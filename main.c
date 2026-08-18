/*
 * Smartmach_v1_0.c
 *
 * Created: 08/11/2021 20:16:34
 * Author : André
 * 
 * Versão 1.3 - 08/11/2021
 *				Criação do arquivo
 *
 * Versão 1.31 - 02/01/2024
 *				Firmware compilado para ATMega328PB
 *				Configuração da SPI corrigida para esse modelo
 *				Escrita/Leitura da SPI só depois de ligar as interrupções
 *				Configuração e níveis lógicos dos pinos da SPI e controle do 74HC165 e 74HC595
 *				Leitura do 74HC165 realizada na borda de descida (CPOL = 1)
 *
 * Versão 1.32 - 10/01/2024
 *				Alterado tempo para responder ao mini pc de 165ms para 170ms (Margem de segurança) "TempoRespostaIntegridade_ms"
 * 
 *				15/02/2024
 *				Realiza comunicação via ModBus RTU com controladores de temperatura modelo:
 *				- TC900ELog v04 (Congelados)
 *				- MT512ELog v10r02 (Resfriados)
 *
 * Versão 1.33 - 06/07/2024
 *				Tempo de acionamento da gancheira corrigido "TempoPulsoInicial_Gancheira_ms" e "TempoEsperaFinalizar_Gancheira_ms"
 *				Alterações na leitura do sensor magnético da esteira
 *
 * ATENÇÃO: COMPILAR SEMPRE COM OTIMIZAÇÃO -Os (OPTIMIZE FOR SIZE)
 */

//FUSES para ATMega328PB-U
//L = 0xFF = 0b11111111
//H = 0xDF = 0b11011111
//E = 0xF4 = 0b11110100
//LB = 0xD4 = 0b11010100

//LOW FUSE -> 0xFF
//CKDIV8 = 1 (DIVISÃO DO CLOCK PRINCIPAL POR 8 DESLIGADO)
//CKOUT = 1 (SAÍDA DO CLOCK EXTERNO NO PORTB0 DESLIGADO)
//SUT1 = 1
//SUT0 = 1 (DELAY DE 14 CICLOS DE CLOCK + 65ms)
//CKSEL3 = 1
//CKSEL2 = 1
//CKSEL1 = 1 (CRISTAL OSCILADOR)
//CKSEL0 = 1 (INICIALIZAÇÃO LENTA 19 CICLOS DE CLOCK + 65ms)

//HIGH FUSE -> 0xDF
//RSTDISBL = 1 (RESET EXTERNO NO PINO C6 LIGADO)
//DWEN = 1 (DEBUGWIRE DESLIGADO)
//SPIEN = 0 (PROGRAMAÇÃO VIA SPI LIGADA)
//WDTON = 1 (WDT LIGADO)
//ESAVE = 1 (NÃO PRESERVA CONTEÚDO DA EEPROM DURANTE APAGAMENTO DO CHIP)
//BOOTSZ1 = 1
//BOOTSZ0 = 1
//BOOTRST = 1 (VETOR DE RESET NO ENDEREÇO 0x0000)

//EXTENDED FUSE -> 0xF4
//CFD = 0
//BODLEVEL2 = 1
//BODLEVEL1 = 0
//BODLEVEL0 = 0 (BOD MINIMO 4,1V A 4,5 V)

//LOCK BYTE -> 0xD4
//BLB12 = 1
//BLB11 = 0
//BLB02 = 1
//BLB01 = 0
//LB2 = 0
//LB1 = 0 (BLOQUEIA LEITURA MAS PERMITE NOVAS GRAVAÇÕES)	
	
#include "main.h"

//Variáveis

//Identificação da maquina
volatile unsigned char Numero_de_Serie[Comprimento_Numero_de_serie];
volatile unsigned char Modelo[Comprimento_Modelo];
volatile unsigned char Versao_de_Hardware[Comprimento_Versao_de_Hardware];
volatile unsigned char Versao_de_Firmware[Comprimento_Versao_de_Firmware];

//Union com saídas do expansor
union Saida Saida_74HC595;
//Union com entradas do expansor
union Entrada	Entrada_74H165;

//Autoteste
extern AT	StatusAutoTeste;

//Atuadores
extern MaquinaDeEstados MaquinaDeEstados_Atuadores;
extern volatile unsigned char TipoDeAtuador[NumeroMaximoDeAtuadores+1];
//Identificação dos atuadores conectados
volatile unsigned char _atuador = 0;
volatile unsigned char _tipo_de_atuador = 0;
volatile unsigned char Retorno_Atuador = 0;
volatile unsigned char Retorno_Solicita_Compra = 0;

//Temperatura
volatile bool Flag_LeituraNTC = false;
volatile float Temperatura = 0.0;
unsigned char StringTemperatura[10] = "";

//Elevador 
volatile unsigned char	_elevador = 0,
						_gaveta = 0;	//Gaveta a posicionar o elevador
extern MaquinaDeEstados Movimenta_Elevador;
MaquinaDeEstados AUX_AcionaElevador;
MaquinaDeEstados AUX_SolicitaCompra;
volatile unsigned char Retorno_Elevador = 0;
extern volatile bool LigaDesliga_Elevador;
extern volatile unsigned char SensoresPosicaoElevador_Validos[NumeroDeGavetas+1];

//Leitura de corrente
volatile unsigned int Canal0Media, Canal7Media;

//Maquina de estados principal
volatile MaquinaDeEstados Maquina_Principal;

extern volatile unsigned char FilaCompras[TamanhoFilaCompras];
extern volatile unsigned char FilaStatusCompras[TamanhoFilaStatusCompras];
volatile unsigned char ItensEntregues = 0;	//Conta itens entregues
volatile bool Flag_StatusHardware = false;
volatile bool Flag_StatusCompras = false;

//USART0
extern volatile bool	Flag_PacoteRecebido,	//1 = Sinaliza recebimento do pacote
						Flag_USART0_Ocupada;
//Fila envio USART0
extern struct f Fila_MiniPC[TamanhoFilaUSART0];

extern volatile unsigned char msg[Tamanho_BufferUSART0];
extern volatile bool Flag_EnviaInformacao_MiniPC;
extern volatile bool Flag_FimDeCurso;

//Union auxiliar para manipular bytes
union {
	unsigned char bytes[4];
	unsigned int u_int;
	unsigned long u_long_int;
	float f;
}ManipulaBytes;

//Porta principal
volatile bool Flag_Le_PortaPrincipal = false;
volatile signed int TimeOut_PortaPrincipal = 0;
volatile bool Trava_PortaPrincipal = false;

//Status da placa
volatile unsigned int ContadorStatus_Led_On = CiclosLedStatusPlaca_On;
volatile unsigned int ContadorStatus_Led_Off = CiclosLedStatusPlaca_Off;

//delay de uso geral
volatile signed long int DelayTimer0_ms = -1;
volatile bool Flag_Delay = false;

//Autoteste
extern MaquinaDeEstados	MaquinaDeEstados_AutoTeste;
extern volatile unsigned char EnviaResultadoDoAutotesteAoFinalizar;

//Identificação dos atuadores
extern volatile unsigned char AtuadorIdentificado[NumeroMaximoDeAtuadores+1];

//Controlador de temperatura
extern volatile unsigned char ControlaComunicacao_Controlador_de_temperatura;
extern volatile bool Flag_EnviaInformacao_ControladorDeTemperatura;
extern struct f_CT Fila_CT[TamanhoFilaModBus];

//Sensor de queda
extern volatile bool Flag_Status_SensorDeQueda;
extern volatile bool LigaDesliga_SensorDeQueda;

//Sensor de queda
extern volatile bool Flag_Le_EntradasComFiltro;
extern volatile bool Flag_TravaDasGavetas;

int main(void) {
	//Configura WatchDog Timer para 4 segundos
	wdt_enable(WDTO_4S);
	//Reinicia contador do WatchDog
	wdt_reset();

//Espera fonte estabilizar
	for(unsigned int Delay_Config_MCU = 0 ; Delay_Config_MCU < 50000 ; Delay_Config_MCU++);
	for(unsigned int Delay_Config_MCU = 0 ; Delay_Config_MCU < 50000 ; Delay_Config_MCU++);
					
	ConfiguraIOs();	
	ConfiguraUSART0(BaudRate_USART0);
	InicializaFila_MiniPc(TamanhoFilaUSART0);
	InicializaFila_ControladorDeTemperatura(TamanhoFilaModBus);
	ConfiguraADC(ReferenciaADC_PinoAREF);
	InicializaTimer0(BaseTempo_TMR0);	//100us
			
	ConfiguraElevador();
	ConfiguraLeituraSensorDeQueda();	
	ConfiguraLeituraEsteira();	
	ConfiguraBUZZER();
	InicializaEEPROM();
	
	//Inicializa Maquinas de estado
	InicializaMaquinaDeEstados_AUX_Autoteste(M_AT_TestaFusivel_Atuadores);
	InicializaMaquinaDeEstados_Principal(M_AutoTeste);
	InicializaMaquinaDeEstados_Atuadores(MA_AtuadorDisponivel);
	InicializaMaquinaDeEstados_PosicionaElevador(MP_ElevadorDisponivel);
	InicializaMaquinaDeEstados_MovimentaElevador(MM_ElevadorDisponivel);
	InicializaMaquinaDeEstados_AUX_AcionaElevador(M_AE_Inicio);
	InicializaMaquinaDeEstados_AUX_SolicitaCompra(M_SC_Verifica_FilaDeCompra);
	InicializaMaquinaDeEstados_ControladorDeTemperatura(Estado_CT_Inicia_MODBUS);
	
	sei();

//Espera fonte de alimentação estabilizar e trocar a alimentação USB para fonte principal
	delayms(1000);
	ConfiguraSPI();

	//Coloca nível lógico 0 nas saídas dos 74HC595
	Saida_74HC595.Expansor = 0;
	Escreve74HC595(Saida_74HC595.Expansor);
	delayms(1);
	//Habilita saída do 74HC595
	Port_OE_74HC595	|= (1<<Pino_OE_74HC595);
		
	ManipulaBytes.u_int = Le74HC165();
		
    while (1) {
		//Reinicia contador do WatchDog
		wdt_reset();
		
		if(ContadorStatus_Led_On > 1) {
			ContadorStatus_Led_On--;
			SET_BIT(PORTD, 6);
		} else {
			if(ContadorStatus_Led_Off > 1) {
				ContadorStatus_Led_Off--;
				CLEAR_BIT(PORTD, 6);
			} else {
				ContadorStatus_Led_On = CiclosLedStatusPlaca_On;
				ContadorStatus_Led_Off = CiclosLedStatusPlaca_Off;			
			}
		}

		//Maquina de estados principal
		switch(Maquina_Principal.Estado) {
			
			case M_ComunicandoCom_ControladorDeTemperatura:
				//Realizando alguma tarefa enviada pelo MiniPC que não seja trata na máquina de estados principal
			break;
			
			case M_AutoTeste:
				if(RealizaAutoteste() == Status_AT_AutoTesteFinalizado) {
					
					InicializaMaquinaDeEstados_AUX_Autoteste(M_AT_TestaFusivel_Atuadores);		
					InicializaMaquinaDeEstados_Principal(M_Ocioso);

					if(EnviaResultadoDoAutotesteAoFinalizar == 1) {
						EnviaResultadoDoAutotesteAoFinalizar = 0;
						
						Retorna_StatusDoAutoteste();					
					}																		
				}
			break;
					
			case M_AcionaAtuador:
				Retorno_Atuador = AcionaAtuador(_atuador,_tipo_de_atuador);
				
				//Terminou o acionamento do atuador?				
				if(Retorno_Atuador != Status_MA_AcionandoAtuador) {					
					
					LimpaVetor(msg,Tamanho_BufferUSART0);
					//Responde para MiniPC sobre o acionamento do atuador
					
					msg[0] = Caractere_Inicio;
					msg[1] = ProtocoloVersao;	
					msg[2] = ComandoAcionaAtuador;
					msg[3] = TamanhoRX_ComandoAcionaAtuador;
					msg[4] = Retorno_Atuador;	
					
					//Calcula CRC
					ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);	
					
					//Acrescenta CRC no pacote
					msg[5] = ManipulaBytes.bytes[0]; //LSB
					msg[6] = ManipulaBytes.bytes[1]; //MSB
												
					msg[7] = Caractere_Fim;			//fim do pacote	
					
					//Coloca informação na FILA
					AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
					
					//Retorna maquina da estado principal para ocioso
					_atuador = 0;				
					
					InicializaMaquinaDeEstados_Principal(M_Ocioso);
					InicializaMaquinaDeEstados_Atuadores(MA_AtuadorDisponivel);	
				} 
					

			break;
			
			case M_AcionaElevador:
				//Movimenta elevador para próxima posição
				switch(AUX_AcionaElevador.Estado) {
	
					case M_AE_Inicio:
						
						Retorno_Elevador = MovimentaElevador(_elevador);
						
						//Terminou a movimentação do elevador?
						if(Retorno_Elevador != Status_MM_AcionandoElevador) {
							//Sim
							//Responde para MiniPC	
							LimpaVetor(msg,Tamanho_BufferUSART0);
							msg[0] = Caractere_Inicio;
							msg[1] = ProtocoloVersao;
							msg[2] = ComandoMovimentaElevador;
							msg[3] = TamanhoTX_ComandoMovimentaElevador;
							msg[4] = Retorno_Elevador;
							
							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
						
							//Acrescenta CRC no pacote
							msg[5] = ManipulaBytes.bytes[0]; //LSB
							msg[6] = ManipulaBytes.bytes[1]; //MSB
						
							msg[7] = Caractere_Fim;			//fim do pacote
						
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);							
							
							//Coloca maquina de estado principal no estado OCIOSO
							InicializaMaquinaDeEstados_Principal(M_Ocioso);
							//Reinicia maquina de estado auxiliar
							AUX_AcionaElevador.Estado = M_AE_Inicio;					
						}
					break;
				}
				
			break;
			
			case M_SolicitaCompra:
				switch(AUX_SolicitaCompra.Estado) {

					case M_SC_Verifica_FilaDeCompra:						
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim
							
							//Tem itens na fila de compra?
							if(ItensEntregues < FilaCompras[0]) {
								//Sim
								//Contabiliza a tentativa de entrega de 1 item
								ItensEntregues++; 

								//Prepara para entregar item da fila
								_atuador = FilaCompras[ItensEntregues];
								//Identifica o tipo do atuador
								_tipo_de_atuador = TipoDeAtuador[_atuador];

								//Verifica o se hardware necessário está OK
								Flag_StatusHardware = false;

								AUX_SolicitaCompra.Estado = M_SC_ValidaHardware_Elevador;
								AUX_SolicitaCompra.TempoProximoEstado = 0;														
							} else {
								//Não tem mais produtos na fila

								DesligaSensorDeQueda();	

								AUX_SolicitaCompra.Estado = M_SC_Avalia_Compra;
								AUX_SolicitaCompra.TempoProximoEstado = 0;											
							}							
						}
					break; 

					case M_SC_ValidaHardware_Elevador:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {								
							//Sim	

							if(LigaDesliga_Elevador == true) {
								//Elevador HABILITADO

								//Lê entradas
								Entrada_74H165.Expansor = Le74HC165();
								
								//Lê trava das gavetas OK
								if(Entrada_74H165.TRAVA_GAVETAS == 0) {
									//Trava da GAVETA FECHADA

									if(!Entrada_74H165.PORTINHOLA) {
										//PORTINHOLA FECHADA

										if(Flag_FimDeCurso == false) {
											//Fim de curso OK

											if(SensoresPosicaoElevador_Validos[PosicaoGaveta_Entrega] == Resultado_SensorDePosicaoDoElevador_OK) {
												//Sensor de entrega OK

												//Identifica gaveta do atuador
												_gaveta = (_atuador / NumeroAtuadoresNaGaveta) + 1;
												if((_atuador % NumeroAtuadoresNaGaveta) == 0) _gaveta--;		

												//Sensor da gaveta OK?
												if(SensoresPosicaoElevador_Validos[_gaveta - 1] == Resultado_SensorDePosicaoDoElevador_OK)	{
													//Sim

													AUX_SolicitaCompra.Estado = M_SC_ValidaHardware_Atuador;
													AUX_SolicitaCompra.TempoProximoEstado = 0;	
												} else {
													//FALHA no sensor da gaveta do atuador
													FilaStatusCompras[ItensEntregues-1] = Status_MM_PosicaoDoElevadorSolicitadaInvalida + Offset_Elevador_Solicita_Compra;
													Flag_StatusHardware = false;	

													AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
													AUX_SolicitaCompra.TempoProximoEstado = 0;													
												}
											} else {
												//FALHA no sensor de entrega
												FilaStatusCompras[ItensEntregues-1] = Status_MM_SensorDeEntregaInvalido + Offset_Elevador_Solicita_Compra;
												Flag_StatusHardware = false;	

												AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
												AUX_SolicitaCompra.TempoProximoEstado = 0;													
											}
										} else {
											//FALHA no Fim de curso
											FilaStatusCompras[ItensEntregues-1] = Status_MM_FimDeCurso + Offset_Elevador_Solicita_Compra;
											Flag_StatusHardware = false;	

											AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
											AUX_SolicitaCompra.TempoProximoEstado = 0;	
										}										

									} else {
										//PORTINHOLA ABERTA
										FilaStatusCompras[ItensEntregues-1] = Status_MM_PortinholaAbertaOuFusivelRompido + Offset_Elevador_Solicita_Compra;
										Flag_StatusHardware = false;	

										AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
										AUX_SolicitaCompra.TempoProximoEstado = 0;																					
									}
									
								} else {
									//Trava da GAVETA ABERTA
									FilaStatusCompras[ItensEntregues-1] = Status_MM_TravaDasGavetasAberta + Offset_Elevador_Solicita_Compra;
									Flag_StatusHardware = false;

									AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
									AUX_SolicitaCompra.TempoProximoEstado = 0;										
								}
							} else {
								//Elevador DESABILITADO

								Flag_StatusHardware = true;
								AUX_SolicitaCompra.Estado = M_SC_ValidaHardware_Atuador;
								AUX_SolicitaCompra.TempoProximoEstado = 0;									
							}
						}
					break;

					case M_SC_ValidaHardware_Atuador:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {								
							//Sim					

							Entrada_74H165.Expansor = Le74HC165();
							//Verifica Fusível do atuador
							if(Entrada_74H165.FUSIVEL_ATUADORES == 0) {
								//Fusível OK

								//Garante que atuador a ser acionado está entre 1 e 72
								if((_atuador >= NumeroMinimoDeAtuadores) && (_atuador <= NumeroMaximoDeAtuadores)) {				
			
									if(AtuadorIdentificado[_atuador] == Atuador_Conectado) {
										//Atuador conectado
									
										if(_tipo_de_atuador == Esteira) {
											//Lê sensor magnético da esteira

											//Garante matriz desligada
											Desliga_MatrizDosAtuadores();	

											//Conecta fonte de +24V nos atuadores
											Saida_74HC595.ID_ATUADORES = 0;
										
											//Seleciona atuador
											//Sendo 0 = Leitura da esteira (Abre +24V lendo sensor magnético)
											//      1 = Acionamento do Motor Mola ou Gancheira (Fecha +24V alimentando atuador)
										
											Saida_74HC595.SEL_ATUADOR = 0;
											Escreve74HC595(Saida_74HC595.Expansor);

											AUX_SolicitaCompra.Estado = M_SC_Valida_SensorMagnetico_Esteira;
											AUX_SolicitaCompra.TempoProximoEstado = 250;						

										} else {
											//Atuador OK
											
											Flag_StatusHardware = true;

											AUX_SolicitaCompra.Estado = M_SC_ValidaHardware_SensorDeQueda;
											AUX_SolicitaCompra.TempoProximoEstado = 0;	
										}
									} else {
										//Se atuador não esta CONECTADO
										//Só pode estar em CURTO ou DESCONECTADO
										if(AtuadorIdentificado[_atuador] == Atuador_Em_Curto) {
											FilaStatusCompras[ItensEntregues-1] = Status_MA_Erro_AtuadorEmCurto;
										} else {
											FilaStatusCompras[ItensEntregues-1] = Status_MA_Erro_AtuadorDesconectado;
										}
									
										Flag_StatusHardware = false;

										AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
										AUX_SolicitaCompra.TempoProximoEstado = 0;	
									}
							
								} else {
									//Se atuador fora do range FINALIZA e não aciona elevador					

									FilaStatusCompras[ItensEntregues-1] = Status_MA_Erro_AtuadorForaDoRange;

									Flag_StatusHardware = false;

									AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
									AUX_SolicitaCompra.TempoProximoEstado = 0;								
								}
							} else {
								//Fusível rompido
								
								FilaStatusCompras[ItensEntregues-1] = Status_MA_Erro_FusivelDosAtuadoresRompido;

								Flag_StatusHardware = false;

								AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
								AUX_SolicitaCompra.TempoProximoEstado = 0;					
							}								
						}
					break;

					case M_SC_Valida_SensorMagnetico_Esteira:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim
											
							if(!(PortLeitura_LeituraEsteira & (1<<Pino_LeituraEsteira))) {
								//Se sensor acionado indica erro						
								FilaStatusCompras[ItensEntregues-1] = Status_MA_Erro_SensorEsteiraAcionadoAntesDeIniciarMovimentacao;

								Flag_StatusHardware = false;

								AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
								AUX_SolicitaCompra.TempoProximoEstado = 0;	
							
							} else {
								//Atuador OK
							
								Flag_StatusHardware = true;

								AUX_SolicitaCompra.Estado = M_SC_ValidaHardware_SensorDeQueda;
								AUX_SolicitaCompra.TempoProximoEstado = 0;	
							}
						}
					break;

					case M_SC_ValidaHardware_SensorDeQueda:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim

							//Verifica se Sensor de Queda está habilitado ou desabilitado												
							if(LigaDesliga_SensorDeQueda == false) {
								//Sensor de Queda DESABILITADO

								Flag_StatusHardware = true;
								AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
								AUX_SolicitaCompra.TempoProximoEstado = 0;		
							} else {
								//Sensor de Queda HABILITADO

								//Energiza sensor de queda
								LigaSensorDeQueda();		

								AUX_SolicitaCompra.Estado = M_SC_ValidaHardware_Le_SensorDeQueda;
								AUX_SolicitaCompra.TempoProximoEstado = TempoEsperaParaValidar_SensorDeQueda_ms; 	//ms

								Flag_Status_SensorDeQueda = false;								
							}
						}
					break;

					case M_SC_ValidaHardware_Le_SensorDeQueda:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim
							//Esperou o tempo necessário para validar o sensor de queda
/*
#warning SENSOR DE QUEDA SEMPRE VALIDO PARA CLEIDSON TESTAR NA BANCADA	
							//Sensor de queda OK
							Flag_StatusHardware = true;
							AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
							AUX_SolicitaCompra.TempoProximoEstado = 0;		
												
*/	
//CÓDIGO DE VALIDAÇÃO DO SENSOR DE QUEDA						
							if(Flag_Status_SensorDeQueda == true) {
								//sensor de queda acionado CANCELA COMPRA
								
								//Alguma coisa errada não está certa

								FilaStatusCompras[ItensEntregues-1] = Offset_SensorDeQueda_Solicita_Compra; //0x38

								Flag_StatusHardware = false;
								AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
								AUX_SolicitaCompra.TempoProximoEstado = 0;										
								
								DesligaSensorDeQueda();
							} else {
								//Sensor de queda OK

								Flag_StatusHardware = true;
								AUX_SolicitaCompra.Estado = M_SC_Avalia_Hardware;
								AUX_SolicitaCompra.TempoProximoEstado = 0;											
							}	
													
						}
					break;

					case M_SC_Avalia_Hardware:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim

							if(Flag_StatusHardware != true) {
								//ERRO no hardware minimo pra realizar a compra do item

								//Verifica se tem mais produtos na fila para compra
								AUX_SolicitaCompra.Estado = M_SC_Verifica_FilaDeCompra;
								AUX_SolicitaCompra.TempoProximoEstado = 0;		
							} else {
								//Hardware minimo pra realizar a compra do item OK

								AUX_SolicitaCompra.Estado = M_SC_MovimentaElevadorNaGaveta;
								AUX_SolicitaCompra.TempoProximoEstado = 0;	
							}
						}					

					break;

					case M_SC_MovimentaElevadorNaGaveta:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim

							if(LigaDesliga_Elevador == true) {
								//Elevador Habilitado MOVIMENTA para gaveta
			
								Retorno_Elevador = MovimentaElevador(_gaveta);
								
								//Terminou a movimentação do elevador?
								if(Retorno_Elevador == Status_MM_AcionamentoFinalizado) {
									//Sim							
									//Passa para próximo estado da maquina auxiliar
									AUX_SolicitaCompra.Estado = M_SC_Aciona_Atuador;
									AUX_SolicitaCompra.TempoProximoEstado = 0;								
																	
									//Garante que leitura anterior do sensor de queda não afete a leitura atual
									Flag_Status_SensorDeQueda = false;	

								} else if(Retorno_Elevador > Status_MM_AcionamentoFinalizado) {
									//Erro na movimentação do elevador			
																										
									FilaStatusCompras[ItensEntregues-1] = Retorno_Elevador + Offset_Elevador_Solicita_Compra;

									AUX_SolicitaCompra.Estado = M_SC_Verifica_FilaDeCompra;
									AUX_SolicitaCompra.TempoProximoEstado = 0;								
						
								}		
							} else {
								//Elevador desabilitado aciona atuador

								AUX_SolicitaCompra.Estado = M_SC_Aciona_Atuador;
								AUX_SolicitaCompra.TempoProximoEstado = 0;		

								//Garante que leitura anterior do sensor de queda não afete a leitura atual
								Flag_Status_SensorDeQueda = false;	
							}
						}
					break;

					case M_SC_Aciona_Atuador:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim

							Retorno_Atuador = AcionaAtuador(_atuador,_tipo_de_atuador);
													
							//Atuador finalizou o acionamento do atuador corretamente?
							if(Retorno_Atuador == Status_MA_AcionamentoFinalizado) {
								//Sim
								//Passa para próximo estado da maquina auxiliar
								
								AUX_SolicitaCompra.Estado = M_SC_Le_SensorDeQueda;	
								AUX_SolicitaCompra.TempoProximoEstado = TimeOutSensorDeQueda_ms; 
								
							} else if(Retorno_Atuador > Status_MA_AcionamentoFinalizado) {
								//Aconteceu algum ERRO 
									
								FilaStatusCompras[ItensEntregues-1] = Retorno_Atuador + Offset_Atuador_Solicita_Compra;

								AUX_SolicitaCompra.Estado = M_SC_Verifica_FilaDeCompra;
								AUX_SolicitaCompra.TempoProximoEstado = 0;									
							}
						}
					break;

					case M_SC_Le_SensorDeQueda:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado > 0) {
							//Sim

							//Se chegou aqui é porque atuador OK apenas avalia o sensor de queda
							if(LigaDesliga_SensorDeQueda == true) {
								//Sensor de queda HABILITADO
								
								if(Flag_Status_SensorDeQueda == true) {
									//Sensor de queda foi acionado compra ENTREGUE
									FilaStatusCompras[ItensEntregues-1] = SensorDeQueda_IdentificouAEntregaDoProduto;		

									AUX_SolicitaCompra.Estado = M_SC_Verifica_FilaDeCompra;
									AUX_SolicitaCompra.TempoProximoEstado = TempoSensorDeQuedaFicaDesligadoEntreCompras_ms;	

									DesligaSensorDeQueda();												
								} 	
							} else {
								//Sensor de queda DESLIGADO 

								//Sempre comfirma a entrega
								FilaStatusCompras[ItensEntregues-1] = SensorDeQueda_IdentificouAEntregaDoProduto;

								AUX_SolicitaCompra.Estado = M_SC_Verifica_FilaDeCompra;
								AUX_SolicitaCompra.TempoProximoEstado = 0;	
							}											
						} else {
							//Deu timeout no sensor de queda

							//Sensor de queda não identificou o produto 
							//Alguma coisa errada não está certa
									
							FilaStatusCompras[ItensEntregues-1] = SensorDeQueda_Nao_IdentificouAEntregaDoProduto;		

							AUX_SolicitaCompra.Estado = M_SC_Verifica_FilaDeCompra;
							AUX_SolicitaCompra.TempoProximoEstado = 0;	

							DesligaSensorDeQueda();	
						}
					break;

					case M_SC_Avalia_Compra:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim

							//Avalia a compra
							Flag_StatusCompras = false;
							for(unsigned char i = 0 ; i < ItensEntregues ; i++) {
								//Varre vetor FilaStatusCompra para verificar se pelo menos uma compra foi realizada com sucesso
								if(FilaStatusCompras[i] == SensorDeQueda_IdentificouAEntregaDoProduto) Flag_StatusCompras = true;
							}								

							if(Flag_StatusCompras == true) {
								//A compra de ALGUM produto foi realizada com sucesso

								AUX_SolicitaCompra.Estado = M_SC_MovimentaElevadorNaEntrega;
								AUX_SolicitaCompra.TempoProximoEstado = 0;	
							} else {
								//A compra de NENHUM produto foi realizada com sucesso
								//Se houve algum erro apenas responde pela serial sem MOVIMENTAR O ELEVADOR

								AUX_SolicitaCompra.Estado = M_SC_Responde_StatusDaCompra;
								AUX_SolicitaCompra.TempoProximoEstado = 0;			
							}
						}
					break;

					case M_SC_MovimentaElevadorNaEntrega:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim

							if(LigaDesliga_Elevador == true) {
								//Elevador Habilitado MOVIMENTA para gaveta
																
								if(Flag_StatusCompras == true) {
									//Se compra ocorreu normalmente MOVIMENTA O ELEVADOR
									Retorno_Elevador = MovimentaElevador(7);
									
									//Terminou a movimentação do elevador?
									if(Retorno_Elevador == Status_MM_AcionamentoFinalizado) {
										//Sim							
										//Passa para próximo estado da maquina auxiliar
										AUX_SolicitaCompra.Estado = M_SC_Responde_StatusDaCompra;
										AUX_SolicitaCompra.TempoProximoEstado = 0;								
																																											
									} else if(Retorno_Elevador > Status_MM_AcionamentoFinalizado) {
										//Erro na movimentação do elevador			

										//Se retorno elevador igual a fim de curso OU SEJA SENSOR DE ENTREGA FALHOU retorna status e não aciona solenoide

 										//Coloca o erro no status da última compra																	
										FilaStatusCompras[ItensEntregues-1] = Retorno_Elevador + Offset_Elevador_Solicita_Compra;

										//ATUALIZA STATUS DA COMPRA
										Flag_StatusCompras = false;
										for(unsigned char i = 0 ; i < ItensEntregues ; i++) {
											//Varre vetor FilaStatusCompra para verificar se pelo menos uma compra foi realizada com sucesso
											if(FilaStatusCompras[i] == SensorDeQueda_IdentificouAEntregaDoProduto) Flag_StatusCompras = true;
										}	

										if(Retorno_Elevador == Status_MM_FimDeCurso) {
											//Elevador CHEGOU no FIM DE CURSO

											//Sensor de posição da entrega falhou 											
											SensoresPosicaoElevador_Validos[6] = 0;
											StatusAutoTeste.SensorDeEntrega_OK = false;										

											//Retorna para posição de repouso e cancela a compra
											Flag_StatusCompras = false;
										}

										//Passa para próximo estado da maquina auxiliar
										AUX_SolicitaCompra.Estado = M_SC_Responde_StatusDaCompra;
										AUX_SolicitaCompra.TempoProximoEstado = 0;		
																
									}
								} else {
									//Se houve algum erro apenas responde pela serial sem MOVIMENTAR O ELEVADOR

									AUX_SolicitaCompra.Estado = M_SC_Responde_StatusDaCompra;
									AUX_SolicitaCompra.TempoProximoEstado = 0;		
								}
		
							} else {
								//Elevador desabilitado 

								//Passa para próximo estado da maquina auxiliar
								AUX_SolicitaCompra.Estado = M_SC_Responde_StatusDaCompra;
								AUX_SolicitaCompra.TempoProximoEstado = 0;	
							}
						}
					break;


					case M_SC_Responde_StatusDaCompra:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim
							LimpaVetor(msg,Tamanho_BufferUSART0);
							//Coloca informação a ser enviada
							msg[0] = Caractere_Inicio;
							msg[1] = ProtocoloVersao;
							msg[2] = ComandoRequisitaCompraDeProduto;
							msg[3] = TamanhoRX_ComandoRequisitaCompraDeProdutoMaximo;
							msg[4] = FilaStatusCompras[0];	//Status da compra do 1° produto
							msg[5] = FilaStatusCompras[1];	//Status da compra do 2° produto
							msg[6] = FilaStatusCompras[2];	//Status da compra do 3° produto
							msg[7] = FilaStatusCompras[3];	//Status da compra do 4° produto
							msg[8] = FilaStatusCompras[4];	//Status da compra do 5° produto
							msg[9] = FilaStatusCompras[5];	//Status da compra do 6° produto
							msg[10] = FilaStatusCompras[6];	//Status da compra do 7° produto

							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,11);
							//Acrescenta CRC no pacote
							msg[11] = ManipulaBytes.bytes[0]; //LSB
							msg[12] =	ManipulaBytes.bytes[1]; //MSB
							msg[13] = Caractere_Fim;	//fim

							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,14,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

							LimpaVetor(FilaStatusCompras, TamanhoFilaStatusCompras);

							if(Flag_StatusCompras == false) {
								//Desliga solenoide
								AcionaSolenoide(0);	

								AUX_SolicitaCompra.Estado = M_SC_MovimentaElevadorParaRepouso;
								AUX_SolicitaCompra.TempoProximoEstado = 0;
							
							} else {
								//Espera com solenoide ligada
								AUX_SolicitaCompra.Estado = M_SC_Aciona_Solenoide;
								AUX_SolicitaCompra.TempoProximoEstado = 0;
							}
						}
					break;

					case M_SC_Espera_Solenoide_Acionada:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim
							//Desliga solenoide
							AcionaSolenoide(0);	

							AUX_SolicitaCompra.Estado = M_SC_MovimentaElevadorParaRepouso;
							AUX_SolicitaCompra.TempoProximoEstado = 0;
						}
					break;

					case M_SC_Aciona_Solenoide:
						//Liga solenoide
						AcionaSolenoide(1);	
//#warning TEMPO SOLENOIDE LIGADA 3s PARA CLEIDSON TESTAR 
						AUX_SolicitaCompra.Estado = M_SC_Espera_Solenoide_Acionada;
						AUX_SolicitaCompra.TempoProximoEstado = EsperaSolenoideLigada_ms;
					break;

					case M_SC_MovimentaElevadorParaRepouso:
						//Esperou o tempo necessário?
						if(AUX_SolicitaCompra.TempoProximoEstado <= 0) {
							//Sim

							if(LigaDesliga_Elevador == true) {
								//Elevador Habilitado MOVIMENTA para gaveta
			
								Retorno_Elevador = MovimentaElevador(6);
								
								//Terminou a movimentação do elevador?
								if(Retorno_Elevador == Status_MM_AcionamentoFinalizado) {
									//Sim										

									AUX_SolicitaCompra.TempoProximoEstado = 0;	
									//Finaliza Compra
									Maquina_Principal.Estado = M_Ocioso;																					
								} else if(Retorno_Elevador > Status_MM_AcionamentoFinalizado) {
									//Erro na movimentação do elevador			
//#warning LIMITACAO ERRO NA MOVIMENTACAO DEPOIS DA COMPRA NAO RETORNA ERRO																									
									AUX_SolicitaCompra.TempoProximoEstado = 0;	

									//Finaliza Compra
									Maquina_Principal.Estado = M_Ocioso;							
								}		
							} else {
								//Elevador desabilitado aciona atuador

								AUX_SolicitaCompra.TempoProximoEstado = 0;	

								//Finaliza Compra
								Maquina_Principal.Estado = M_Ocioso;	
							}
						}
					break;


				}				
			break;
			
		}
		
		//Deve ler a temperatura a cada 1s?
		if(Flag_LeituraNTC == true) {
			//Sim 
			
			//Sinaliza que não deve mais ler a temperatura
			Flag_LeituraNTC = false;
			Temperatura = CalculaTemperatura();		

		}
		
		//Recebeu informações pela serial?
		if(Flag_PacoteRecebido == true) {
			//Sim
			//Sinaliza que tratou o ultimo pacote
			Flag_PacoteRecebido = false;		
			TrataUSART0();						
		}	
		
		
		//Verifica se deve responder para MiniPC e USART0 não está em uso pelo ModBus
		if((Flag_EnviaInformacao_MiniPC == true) && (Flag_USART0_Ocupada == false)) {
			Flag_EnviaInformacao_MiniPC = false;
			//Envia mensagem da posição 0 da fila
			EnviaStringUSART0(Fila_MiniPC[0].msg,Fila_MiniPC[0].Tamanho_msg_bytes,0,_USART);
			//Sinaliza que já enviou resposta para o MiniPC
			Fila_MiniPC[0].Flag_JaEnviouResposta = true;
		}				

		//Deve se comunicar com controlador de temperatura?
		if(ControlaComunicacao_Controlador_de_temperatura == 1) {
			Comunicacao_ControladorDeTemperatura();	
		}
	
		//Verifica se deve enviar para Controlador de temperatura 
		if((Flag_EnviaInformacao_ControladorDeTemperatura == true) && (Flag_USART0_Ocupada == false)) {
			Flag_EnviaInformacao_ControladorDeTemperatura = false;
			
			EnviaStringUSART0(Fila_CT[0].msg,Fila_CT[0].Tamanho_msg_ModBus_Bytes,0,_ModBus);
			
		}

		//Porta principal está aberta?
		if(Flag_Le_PortaPrincipal == true) {
			Flag_Le_PortaPrincipal = false;
			
			

			if(VerificaPortaPrincipal() == 1) {
				//Sim
				if (Trava_PortaPrincipal == false) {
					Trava_PortaPrincipal = true;
					//Inicia contagem para ligar alarme de porta aberta
					TimeOut_PortaPrincipal = EsperaAlarmePortaAberta_s;
				}
			} else {
				//Porta fechada
				Trava_PortaPrincipal = false;
				//Desliga BUZZER
				CLEAR_BIT(Port_BUZZER,Pino_BUZZER);
				//Desliga temporizador para ligar alarme de porta aberta
				TimeOut_PortaPrincipal = 0;
			}	
		}

		if(Flag_Le_EntradasComFiltro == true) {
			Flag_Le_EntradasComFiltro = false;			
			Le_Entradas_Com_Filtro(TempoParaConsiderar_FimDeCurso_Acionado_ms);
		} 
    }
}

void ConfiguraIOs() {	
	DDRB = 0b11101111;		//PORTB PB4 (MISO) entrada restante como saída
	DDRC = 0b11110110;		//PORTC PC0 e PC3 como entrada para ADC demais pinos como saída
	DDRD = 0b11111110;		//PORTD PD0 (RX) como entrada demais pinos como saída
	
	MCUCR |= (1 << PUD);	//Desliga pull-ups
	
	//Mantem CS do 74HC595 alto e demais pinos em 0V
	//CS do 74HC165 em nível baixo para ignorar pulsos de CLK
	PORTB |= (1<<_CS_74HC595);
	PORTC = 0x00;
	PORTD = 0x00;			//Zera todas as portas
}


/* Inicializa maquina de estados principal
 * Parâmetros de entrada:
 * Parâmetro 1 = Estado inicial da maquina de estados
 *
 * Parâmetro de saída:
 * Não tem
 */
void InicializaMaquinaDeEstados_Principal(unsigned char EstadoInicial) {
	Maquina_Principal.Estado = EstadoInicial;
	Maquina_Principal.TempoProximoEstado = 0;
}

/* Inicializa maquina de estados AUXILIAR para acionar elevador
 * Parâmetros de entrada:
 * Parâmetro 1 = Estado inicial da maquina de estados
 *
 * Parâmetro de saída:
 * Não tem
 */
void InicializaMaquinaDeEstados_AUX_AcionaElevador(unsigned char EstadoInicial) {
	AUX_AcionaElevador.Estado = EstadoInicial;
	AUX_AcionaElevador.TempoProximoEstado = 0;
}

/* Inicializa maquina de estados AUXILIAR para comando SOLICITA COMPRA
 * Parâmetros de entrada:
 * Parâmetro 1 = Estado inicial da maquina de estados
 *
 * Parâmetro de saída:
 * Não tem
 */
void InicializaMaquinaDeEstados_AUX_SolicitaCompra(unsigned char EstadoInicial) {
	AUX_SolicitaCompra.Estado = EstadoInicial;
	AUX_SolicitaCompra.TempoProximoEstado = 0;
}

/* Inicializa maquina de estados AUXILIAR para comando AUTOTESTE
 * Parâmetros de entrada:
 * Parâmetro 1 = Estado inicial da maquina de estados
 *
 * Parâmetro de saída:
 * Não tem
 */
void InicializaMaquinaDeEstados_AUX_Autoteste(unsigned char EstadoInicial) {
	MaquinaDeEstados_AutoTeste.Estado = EstadoInicial;
	MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
}

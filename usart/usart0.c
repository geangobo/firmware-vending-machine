/*
 usart0.c - Código fonte
 Escrito por André N. da Cruz, Dezembro 2021
 SmartMach.

 Versão: 01
 Dezembro/2021 - André Cruz
  - Vetor de interrupção do periférico USART0 (Leitura de informações)   
  - Envio de informações em fila FIFO com temporização
 */ 

//Include
#include "usart0.h"

volatile unsigned char  Buffer_USART0[Tamanho_BufferUSART0], //Buffer de recepção serial
						Pacote_USART0[Tamanho_BufferUSART0]; //Buffer com pacote recebido
						
volatile unsigned int	CRCCalculado = 0, //Armazena CRC calculado do pacote recebido
						CRCRecebido = 0; //Armazena CRC recebido no pacote


//Fila envio USART0 MiniPc
volatile unsigned char Num_msg_fila_MiniPC = 0; //Número de mensagens na fila da USART para serem enviadas
volatile bool Flag_EnviaInformacao_MiniPC = false; //true = Envia mensagem para Mini Pc

//Declara vetor com de structs f para Fila da USART
struct f Fila_MiniPC[TamanhoFilaUSART0];

//Buffer para armazenar a mensagem a ser enviada para o Mini Pc
volatile unsigned char msg[Tamanho_BufferUSART0] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
extern volatile unsigned char TipoDeAtuador[NumeroMaximoDeAtuadores+1];

//Identificação da maquina
extern volatile unsigned char Numero_de_Serie[Comprimento_Numero_de_serie];
extern volatile unsigned char Modelo[Comprimento_Modelo];
extern volatile unsigned char Versao_de_Hardware[Comprimento_Versao_de_Hardware];
extern volatile unsigned char Versao_de_Firmware[Comprimento_Versao_de_Firmware];

//Controle de temperatura
extern volatile unsigned char Buffer_ControladorDeTemperatura_Conversao[Tamanho_Buffer_ControladorDeTemperatura_Conversao];

extern volatile signed char TempoEsperaPulsoDoDegelo_ControladorDeTemperatura_s;

extern volatile unsigned char IdentificaComando_ControladorDeTemperatura;
extern volatile unsigned char Identifica_Parametro_ControladorDeTemperatura;

extern volatile unsigned char Buffer_Modbus[Tamanho_BufferModbus];
extern volatile unsigned char Pacote_Modbus[Tamanho_BufferModbus];
extern volatile unsigned char ContadorBytesRx_ModBus;
extern volatile unsigned char TamanhoDoPacote_ModBus_Bytes;
extern volatile signed int TempoEsperaProximoByte_ModBus;
extern volatile bool Flag_PacoteRecebido_ModBus;
extern volatile bool Flag_JaRecebeuEndereco_ControladorDeTemperatura;

//Novo algoritmo fila

volatile signed char	TempoEsperaProximoPacote = 0,			//Tempo para validar o recebimento do proximo pacote (50ms)
						TempoEsperaFimDoPacote = 0,				//Tempo para identifica o fim do recebimento de um pacote				
						TempoEsperaFimTransmissaoModBus = -1;
							
volatile unsigned char	ContadorBytesRxUSART0 = 0, //Contador de bytes recebidos do pacote
						TamanhoDoPacote = 0; //Quantidade de bytes presente no pacote

volatile bool	Flag_JaRecebeuInicioDoPacote = false, //true = Indica que recebeu o caractere de inicio de um pacote (0x10)
				Flag_PacoteRecebido = false, //true = Indica que recebeu um pacote 	
				Flag_USART0_Ocupada = false;

//Guarda quantidade de produtos na fila e atuador correspondente
volatile unsigned char FilaCompras[TamanhoFilaCompras] = {0,0,0,0,0,0,0,0,0};
volatile unsigned char FilaStatusCompras[TamanhoFilaStatusCompras] = {0,0,0,0,0,0,0,0,0};

extern volatile unsigned char ItensEntregues;	//Conta itens entregues

volatile bool	RespondeNACK = false; //true = Responde NACK para Mini PC

volatile unsigned char QuemEstaUsandoSerial = _USART;	//UART ou MODBUS

extern volatile MaquinaDeEstados Maquina_Principal;

extern unsigned char StringTemperatura[10];
extern volatile float Temperatura;

//Union com saídas do expansor
extern union Saida Saida_74HC595;
//Union com entradas do expansor
union Entrada	Entrada_74H165;

extern volatile unsigned char _atuador;
extern volatile unsigned char _tipo_de_atuador;

extern volatile unsigned char _elevador;

extern volatile bool LigaDesliga_Elevador;
extern volatile bool LigaDesliga_SensorDeQueda;

extern AT	StatusAutoTeste;

//Union auxiliar para manipular bytes
union {
	unsigned char bytes[4];
	unsigned int u_int;
	int _int;
	unsigned long u_long_int;
	float f;
}ManipulaBytes;

//Identificação dos atuadores
extern volatile unsigned char AtuadorIdentificado[NumeroMaximoDeAtuadores+1];
//Identificação dos sensores de posicao validos
volatile unsigned char SensoresPosicaoElevador_Validos[NumeroDeGavetas+1];
//Resultado do autoteste
extern volatile unsigned char EnviaResultadoDoAutotesteAoFinalizar;

extern volatile unsigned char UltimaPosicaoValidaDoElevador;

//Interrupção de recepção da UART
ISR(USART0_RX_vect) {
	//Limpa flag da interrupção
	UCSR0A &= ~(1 << RXC0);
	
	if(QuemEstaUsandoSerial == _USART) {
		//Coloca informação recebida no buffer e limpa flag da interrupção
		Buffer_USART0[ContadorBytesRxUSART0] = UDR0;	
					
		//Já esperou o tempo necessário para receber o próximo pacote?
		if(TempoEsperaProximoPacote == 0) {
			//Sim

			//Inicializa temporizador para identificar o fim do pacote					
			//Espera 45ms entre pacotes	
			TempoEsperaFimDoPacote = TempoEsperaFimDoPacote_ms;						
							
			//Ainda não recebeu o caractere de inicio (0x10) do pacote?
			if(Flag_JaRecebeuInicioDoPacote == false) {
				//Não
					
				//Informação recebida é o caractere de inicio (0x06) do pacote?
				if(Buffer_USART0[ContadorBytesRxUSART0] == Caractere_Inicio) {
					//Sim				
		
					//Sinaliza que já recebeu o caractere de inicio do pacote
					Flag_JaRecebeuInicioDoPacote = true;
					//Contabiliza byte de informação recebida
					ContadorBytesRxUSART0++;
				
				} 
			} else {
				//Sim, Já recebeu o caractere de inicio
			
				//Já preencheu todo buffer de recepção?
				if(ContadorBytesRxUSART0 < (Tamanho_BufferUSART0-1)) {
					//Não 
				
					//Contabiliza byte de informação recebida
					ContadorBytesRxUSART0++;
				} else {
					//Sim
					//Preencheu todo buffer
					
					//Zera temporizador para identificar o fim do pacote
					TempoEsperaFimDoPacote = 0;
				
					//Sinaliza que não recebeu o inicio do pacote
					Flag_JaRecebeuInicioDoPacote = false;
		
					//Sinaliza que recebeu pacote
					Flag_PacoteRecebido = true;
		
					//Prepara para próxima recepção
					//Copia informação recebida pela serial
					CopiaVetor(Buffer_USART0,Pacote_USART0,ContadorBytesRxUSART0);
					TamanhoDoPacote = ContadorBytesRxUSART0;
					ContadorBytesRxUSART0 = 0;
		
					LimpaVetor(Buffer_USART0,Tamanho_BufferUSART0);		
				}
			}
		} else {
			//Recebeu informação durante periodo que deve ignorar comando
		
		}


		if(Buffer_USART0[ContadorBytesRxUSART0] == ACK) {  //Recebeu um ACK do Mini Pc		
		
		   if (Fila_MiniPC[0].EsperaRespostaMiniPc==true) {	//Está esperando resposta do Mini Pc
				//Zera temporizador para identificar o fim do pacote
				TempoEsperaFimDoPacote = 0;		
			
				if((Fila_MiniPC[0].Flag_JaEnviouResposta == true)&& //Já enviou resposta para o Mini Pc
				(Flag_JaRecebeuInicioDoPacote == false)) { //Não está recebendo um pacote
						RemoveMsgFila_MiniPc(0x01);
				}
		   } else {
			   //Não está esperando resposta do Mini Pc
			   if(Flag_JaRecebeuInicioDoPacote == false) { //Está recebendo um pacote
					//Zera temporizador para identificar o fim do pacote
					TempoEsperaFimDoPacote = 0;		
				
					//Limpa BUFFER para evitar 
					ContadorBytesRxUSART0 = 0;
				
					LimpaVetor(Buffer_USART0,Tamanho_BufferUSART0);						   
			   }
		   }
		}
	
	} else {
		//ModBus usando a serial
		
		//ESTE ALGORITMO CONSIDERA FINALIZADA A RECEPÇÃO DE UM PACOTE QUANDO DA TIMEOUT NA RECEPÇÃO DE UM BYTE
		
		//Coloca byte recebido no buffer 
		Buffer_Modbus[ContadorBytesRx_ModBus] = UDR0;			

		//Reinicia a contagem do tempo máximo para receber o próximo byte
		TempoEsperaProximoByte_ModBus = TempoEsperaParaReceberProximoByte_ModBus_ms;
		
		if(Flag_JaRecebeuEndereco_ControladorDeTemperatura == false) {
			//Ainda não recebeu Endereço do controlador de temperatura
			
			if(Buffer_Modbus[ContadorBytesRx_ModBus] == EnderecoModBus_ControladorDeTemperatura) {
				//Recebeu o ENDEREÇO ModBus do controlador de temperatura
				
				Flag_JaRecebeuEndereco_ControladorDeTemperatura = true;
				//Contabiliza byte recebido
				ContadorBytesRx_ModBus++;
			}
			
		} else {
			//Já recebeu ENDEREÇO
			
			//Verifica o Buffer
			if(ContadorBytesRx_ModBus < (Tamanho_BufferModbus-1)) {
				//Ainda não encheu o buffer
			
				//Contabiliza byte recebido
				ContadorBytesRx_ModBus++;
			} else {
				//Encheu o buffer

				//Prepara para próxima recepção
				//Copia informação recebida 
				CopiaVetor(Buffer_Modbus,Pacote_Modbus,ContadorBytesRx_ModBus);
				TamanhoDoPacote_ModBus_Bytes = ContadorBytesRx_ModBus;
				ContadorBytesRx_ModBus = 0;
				
				LimpaVetor(Buffer_Modbus,Tamanho_BufferModbus);
			
				//Prepara para receber o próximo byte
				TempoEsperaProximoByte_ModBus = -1;			
			
				//Sinaliza que recebeu informação pelo ModBus
				Flag_PacoteRecebido_ModBus = true;
				
				Flag_JaRecebeuEndereco_ControladorDeTemperatura = false;
			}
		}
	}
}

/* Configura e inicializa USART0 para:
 * Start bit: 1
 * 8 bits de dados
 * Sem paridade
 * Stop bit: 1
 * Parâmetro de entrada:
 * Parâmetro 1 - Baudrate velocidade de transmissão (4800bps a 57600bps).
 *	
 * Parâmetro de saída:
 * Não tem.
*/
void ConfiguraUSART0(unsigned long int Baudrate){
	unsigned long int CalculaBaudrate = 0;
	
	if(Baudrate < 4800) Baudrate = 4800;
	else if(Baudrate > 57600) Baudrate = 57600;
	
	CalculaBaudrate = (((F_CPU / (Baudrate * 16UL))) - 1);
	
	//Configura baudrate
	UBRR0H = (unsigned char) CalculaBaudrate >> 8;
	UBRR0L = (unsigned char) CalculaBaudrate;
	
	//Envio e recepção ligados gerando interrupção de rx
	UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
	
	UCSR0C = (3<<UCSZ00);
	
	//Configura pino de seleção entre USART e MODBUS como saída
	Port_Selecao_USART_MODBUS |= (1 << Pino_Selecao_USART);
	Port_Selecao_USART_MODBUS |= (1 << Pino_Selecao_MODBUS);
	
	//Configura pino de envio e recepção de dados ModBus como saída
	Port_TX_RX_MODBUS |= (1 << Pino_TX_RX_MODBUS);
	
	//Direciona USART do uC para USART
	CLEAR_BIT(Port_Selecao_USART_MODBUS, Pino_Selecao_USART);
	//Desliga comunicação com dispositivo MODBUS
	SET_BIT(Port_Selecao_USART_MODBUS, Pino_Selecao_MODBUS);
	
	//Deixa ModBus aguardando para recepção de informações
	CLEAR_BIT(Port_TX_RX_MODBUS,Pino_TX_RX_MODBUS);
}

/*
Envia string pela USART0.

Parâmetro de entrada:
1 -	str
	Ponteiro para a string a ser enviada.
2 - NumeroBytes
	Quantidade de bytes ou posições do vetor enviado. [0 a 255]
3 - c
	Controla o caracter a ser enviado ao finalizar envio. (1 envia \n 0 não envia nada a mais)
4 - SelecionaUSART_MODBUS
	0 Para enviar informação pela USART
	1 Para enviar informação pelo ModBus
Parâmetro de saída:
	Não tem.
*/
void EnviaStringUSART0(volatile unsigned char *str, unsigned char NumeroBytes, char c, bool SelecionaUSART_MODBUS) {
	unsigned int i;
	
	Flag_USART0_Ocupada = true;
	
	//Habilita envio pelo ModBus
	if(SelecionaUSART_MODBUS == _ModBus) {

		//Habilita envio de dados pelo ModBus
		SET_BIT(Port_TX_RX_MODBUS,Pino_TX_RX_MODBUS);	
		
		//Como 9600bits por segundo = +/- 1,04ms para enviar cada byte
		
		//Mantém pino ligado o tempo suficiente para finalizar a transmissão
		TempoEsperaFimTransmissaoModBus = NumeroBytes*2;
	}
	
	
	//Sinaliza que USART0 está enviando
	for(i = 0; i<NumeroBytes ; i++) {
		//Espera buffer da USART ficar vazio
		while(!(UCSR0A & (1 << UDRE0)));
		//Envia 1 byte
		UDR0 = str[i];
	}
	
	if(c == 1){
		//Espera buffer da USART ficar vazio
		while(!(UCSR0A & (1 << UDRE0)));
		//Envia \r no fim da string
		UDR0 = '\r';
	}
	
	//Espera para finalizar o envio da informação pelo ModBus
	if(SelecionaUSART_MODBUS == _ModBus) {
		//Inicia temporizador 3 bytes antes de finalizar 
		
		TempoEsperaFimTransmissaoModBus = 3;
	}
	else Flag_USART0_Ocupada = false;	//Senão for envio ModBus já libera a USART
}

/* Calcula CRC com algoritmo CRC-16/XMODEM da mensagem recebida do mestre 
 * ATENÇÃO:PARA REALIZAR O CALCULO DO CRC OS DOIS BYTES DO CRC DEVE SER REMOVIDO DO PACOTE. A ROTINA REALIZA ESTÁ TAREFA.
 * Parâmetro de entrada:
 * Parâmetro 1 - Ponteiro para string a ser calculada o CRC.
 * Parâmetro 2 - Quantidade de bytes ou posições do vetor para calculo do CRC. [0 a 255]
 * Parâmetro de saída:
 * Valor do CRC-16/XMODEM da string str.
*/
unsigned int crc16_XMODEM_RX(volatile unsigned char *str, volatile unsigned int NumeroBytes) {
	unsigned int Crc = 0;
	unsigned char i;
	
	//Remove CRC do vetor
	str[NumeroBytes - 3] = str[NumeroBytes - 1];		//Copia último byte
	str[NumeroBytes - 2] = 0;							//Limpa ante penultimo byte
	str[NumeroBytes - 1] = 0;							//Limpa último byte
	
	//Remove os 2 bytes do CRC do comprimento do vetor
	NumeroBytes-=2;

	while (NumeroBytes-- > 0) {
		Crc = Crc ^ (unsigned int)*str++ << 8;
		for (i = 0; i < 8; ++i) {
			if (Crc & 0x8000)
			Crc = Crc << 1 ^ 0x1021;
			else
			Crc = Crc << 1;
		}
	}
	return (Crc & 0xFFFF);
}

/* Calcula CRC com algoritmo CRC-16/XMODEM da mensagem a ser transmitida ao mestre 
 * ATENÇÃO:PARA REALIZAR O CALCULO DO CRC O CARACTERE DE FIM DO PACOTE DE SER ADICIONADO A STRING. A ROTINA REALIZA ESTÁ TAREFA.
 * Parâmetro de entrada:
 * Parâmetro 1 - Ponteiro para string a ser calculada o CRC.
 * Parâmetro 2 - Quantidade de bytes ou posições do vetor para calculo do CRC. [0 a 255]
 * Parâmetro de saída:
 * Valor do CRC-16/XMODEM da string str.
*/
unsigned int crc16_XMODEM_TX(volatile unsigned char *str, volatile unsigned int NumeroBytes) {
	unsigned int Crc = 0;
	unsigned char i;

	//Adiciona fim do protocolo na string
	str[NumeroBytes] = 0x1E;
	//Acrescenta no comprimento do vetor
	NumeroBytes++;	

	while (NumeroBytes-- > 0) {
		Crc = Crc ^ (unsigned int)*str++ << 8;
		for (i = 0; i < 8; ++i) {
			if (Crc & 0x8000)
			Crc = Crc << 1 ^ 0x1021;
			else
			Crc = Crc << 1;
		}
	}
	return (Crc & 0xFFFF);
}

/* Verifica se os dados recebidos pela USART0 estão seguindo o PROTOCOLO SERIAL versão 2 de JANEIRO/2023
 * Parâmetros de entrada:
 * Não tem.
 * Parâmetros de saída:
 * Não tem.
*/	
void TrataUSART0 (void) {
	unsigned char Tamanho_String = 0, i = 0, r = 0, x = 0, _Valida_Atuadores = 0, NumProdutosOK = 0;
	signed char p_Elevador = 0;
	float Aux_f = 0.0;
			
	//Só trata dados recebidos se maquina de estados principal estiver no modo Ocioso
	if(Maquina_Principal.Estado == M_Ocioso) {	
																	
		//Verifica se versão do protocolo está correta
		//Verifica quantidade de bytes com informação está dentro do range esperado
		//Verifica os comandos estão dentro do range esperado
		//Verifica caractere final está correto
		if( (Pacote_USART0[1] == ProtocoloVersao)  && 																
			((Pacote_USART0[3] >= TamanhoRX_ParametroMINIMO) && (Pacote_USART0[3] <= TamanhoRX_ParametroMAXIMO)) &&	
			((Pacote_USART0[2]>= ComandoAcionaAtuador) && (Pacote_USART0[2]<= ComandoLeConfiguracaoDoControladorDeTemperatura)) && 	
			(Pacote_USART0[TamanhoDoPacote-1] == Caractere_Fim)) {													

			//Pacote recebido dentro dos padrões do protocolo

			//Lê CRC recebido
			ManipulaBytes.bytes[0] = Pacote_USART0[TamanhoDoPacote-3]; //LSB
			ManipulaBytes.bytes[1] = Pacote_USART0[TamanhoDoPacote-2]; //MSB
			CRCRecebido = ManipulaBytes.u_int;
	
			//Calcula CRC
			CRCCalculado = crc16_XMODEM_RX(Pacote_USART0,TamanhoDoPacote);

			//Verifica integridade da mensagem
			if(CRCCalculado == CRCRecebido) {
				//CRC Correto responde ACK					
			
				//Limpa fila
				RemoveMsgFila_MiniPc(0xFF);
			
				//Coloca ACK na fila 
				LimpaVetor(msg,Tamanho_BufferUSART0);
				msg[0] = ACK;
				AdicionaMsgFila_MiniPc(msg,TempoRespostaIntegridade_ms,1,false,0,0);			
			
				switch(Pacote_USART0[2]){
					case ComandoAcionaAtuador:	//0x11
									
						//Quantidade de bytes com informação correta e Atuador dentro do range de acionamento da matriz? 
						if((Pacote_USART0[3] == TamanhoRX_ComandoAcionaAtuador) && //0x01
						   (Pacote_USART0[4] >= Parametro_AtuadorMinimo) && (Pacote_USART0[4] <= Parametro_AtuadorMaximo)) { //0x01 a 0x48
							//Sim
							//Comando completo recebido OK
						
							//Aciona atuador sem verificar se está conectado	
							//Copia atuador
							_atuador = Pacote_USART0[4]; 
						
							//Lê o tipo de atuador
							_tipo_de_atuador = TipoDeAtuador[_atuador];
						
							InicializaMaquinaDeEstados_Atuadores(MA_AtuadorDisponivel);
						
							//Prepara maquina de estados principal
							Maquina_Principal.Estado = M_AcionaAtuador;
													
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;			

						} else {
							//Verifica se atuador está fora do range
							if((Pacote_USART0[4] < Parametro_AtuadorMinimo) || (Pacote_USART0[4] > Parametro_AtuadorMaximo)) {
								//Coloca resposta na fila
								LimpaVetor(msg,Tamanho_BufferUSART0);
								//Prepara parte inicial da resposta						
								msg[0] = Pacote_USART0[0];
								msg[1] = Pacote_USART0[1];
								msg[2] = Pacote_USART0[2];
								msg[3] = TamanhoRX_ComandoAcionaAtuador;	//Quantidade de bytes da resposta																					
								msg[4] = Status_MA_Erro_AtuadorForaDoRange;	//Indica que atuador está fora do range													
							
								//Calcula CRC
								ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
								
								//Acrescenta CRC no pacote
								msg[5] = ManipulaBytes.bytes[0]; //LSB
								msg[6] = ManipulaBytes.bytes[1]; //MSB
								
								msg[7] = Caractere_Fim;			//fim do pacote
																					
								//Coloca informação na FILA
								AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao); 
							} else {
								//Tamanho do parâmetro inválido

								//Comando inválido envia NACK para MiniPC
								RespondeNACK = true;
							}
						}

					break;
			
					case ComandoAcionaPortaPrincipal:	//0x12
						//Quantidade de bytes com informação correta e acionamento da fechadura dentro do range?
						if((Pacote_USART0[3] == TamanhoRX_ComandoAcionaPortaPrincipal) && //0x01
						  ((Pacote_USART0[4] >= Parametro_FechaPortaPrincipal) && (Pacote_USART0[4] <= Parametro_AbrePortaPrincipal))) { //0x01 a 0x02
							//Comando completo recebido OK
						
							if(Pacote_USART0[4] == 1) {
								//Liga trava magnética
								AcionaTravaMagnetica(1);
							}else if(Pacote_USART0[4] == 2) {
								//Desliga trava magnética
								AcionaTravaMagnetica(0);
							}
						
							//Prepara maquina de estados principal
							InicializaMaquinaDeEstados_Principal(M_Ocioso);
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;						
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
			
					case ComandoMovimentaElevador:	//0x13
						//Quantidade de bytes com informação correta e motor dentro do range?
						if((Pacote_USART0[3] == TamanhoRX_ComandoMovimentaElevador) && //0x01
						  ((Pacote_USART0[4] >= Parametro_SobeElevador) && (Pacote_USART0[4] <= Parametro_DesceElevador))) { //0x01 a 0x02
							//Comando completo recebido OK
												
							//Copia comando
							_elevador = Pacote_USART0[4];
			
							//Lê posição atual do elevador
							p_Elevador = PosicaoAtualElevador();
						
							if((p_Elevador >= 1) && (p_Elevador <= 7)) {
								//Se elevador em alguma posição válida

								//Elevador deve subir?
								if(_elevador == 1) p_Elevador--;
								else if(_elevador == 2) p_Elevador++;		
										
							} else {
								//Elevador não posicionado em nem um sensor
							
								//Se ultima posição válida != 0 usa ela
								if(UltimaPosicaoValidaDoElevador == 0) {
									//Ultima posição inválida
									p_Elevador = 0;
								} else {
									//Ultima posição do sensor válida
//#warning USA ULTIMA POSICAO VALIDA PARA MOVIMENTAR ELEVADOR 
									p_Elevador = UltimaPosicaoValidaDoElevador;
									
									//Elevador deve subir?
									if(_elevador == 1) p_Elevador--;
									else if(_elevador == 2) p_Elevador++;		
								}									
							}
					
							_elevador = p_Elevador;
								
							InicializaMaquinaDeEstados_MovimentaElevador(MM_ElevadorDisponivel);
							InicializaMaquinaDeEstados_AUX_AcionaElevador(M_AE_Inicio);
							
							//Prepara maquina de estados principal
							Maquina_Principal.Estado = M_AcionaElevador;													

							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;						
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
			
					case ComandoVerificaTravaDasGavetas:	//0x14
						if((Pacote_USART0[3] == TamanhoRX_ComandoVerificaTravaDasGavetas) && //0x01
						   (Pacote_USART0[4] == Parametro_VerificaGavetas)) {	//0x01					   
							//Comando completo OK
						   
							//Lê trava da gaveta
							Entrada_74H165.Expansor = Le74HC165();
						
							//Coloca resposta na fila
							LimpaVetor(msg,Tamanho_BufferUSART0);
							//Prepara parte inicial da resposta						
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];
							msg[3] = TamanhoRX_ComandoVerificaTravaDasGavetas;	//Quantidade de bytes da resposta
						
							if(Entrada_74H165.TRAVA_GAVETAS == 1) {
								//Trava aberta 														
								msg[4] = Resposta_TravaDasGavetas_Aberta;	//Indica que trava esta aberta													
							} else {
								//Trava fechada							  
								msg[4] = Resposta_TravaDasGavetas_Fechada;	//Inidica que trava das gavetas está fechada				
							}
						
							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
							
							//Acrescenta CRC no pacote
							msg[5] = ManipulaBytes.bytes[0]; //LSB
							msg[6] = ManipulaBytes.bytes[1]; //MSB
							
							msg[7] = Caractere_Fim;			//fim do pacote
																				
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);  
						 
							InicializaMaquinaDeEstados_Principal(M_Ocioso);

							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;						   
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
			
					case ComandoVerificaPosicaoElevador:	//0x15
						//Quantidade de bytes com informação correta?
						if((Pacote_USART0[3] == TamanhoRX_ComandoVerificaPosicaoElevador) && 
						   (Pacote_USART0[4] == VerificaPosicaoElevador)) {
							//Sim
						
							//Comando completo recebido OK
						
							LimpaVetor(msg,Tamanho_BufferUSART0);
						
							//Coloca informação a ser enviada
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];
							msg[3] = TamanhoTX_ComandoVerificaPosicaoElevador;						
						
							//Se elevador desabilitado
							if(LigaDesliga_Elevador == false) {
								msg[4] = 0x0A + Offset_Retorno_PosicaoElevador; //0x2F
							//Se elevador não estiver em nem um sensor	
							} else if(PosicaoAtualElevador() == 0) {
								Entrada_74H165.Expansor = Le74HC165();
							
								if((Entrada_74H165.FIM_CURSO_INFERIOR == 1) || (Entrada_74H165.FIM_CURSO_SUPERIOR == 1) || (Entrada_74H165.FIM_CURSO_RESERVA == 1)) {
									msg[4] = 0x08 + Offset_Retorno_PosicaoElevador; //0x2D
								} else {
									//Senão está em nem um fim de curso
									msg[4] = 0x09 + Offset_Retorno_PosicaoElevador;	//0x2E
								}	
							} else {
								msg[4] = PosicaoAtualElevador() + Offset_Retorno_PosicaoElevador;	//POSIÇÃO DO ELEVADOR	
							}
											
							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
						
							//Acrescenta CRC no pacote
							msg[5] = ManipulaBytes.bytes[0]; //LSB
							msg[6] =	ManipulaBytes.bytes[1]; //MSB
						
							msg[7] = Caractere_Fim;			//fim do pacote
						
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
						
							InicializaMaquinaDeEstados_Principal(M_Ocioso);
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;						
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
			
					case ComandoDetectaProdutoNaBandeija:	//0x16 NÃO USADO
			
					break;
			
					case ComandoLeTemperatura:	//0x17
						//Quantidade de bytes com informação correta?
						if((Pacote_USART0[3] == TamanhoRX_ComandoLeTemperatura) &&
						   (Pacote_USART0[4] == Parametro_LeTemperatura)) {				
							//Sim
						
							//Comando completo recebido OK						
							LimpaVetor(msg,Tamanho_BufferUSART0);
												
							LimpaVetor(StringTemperatura,10);
							FloatToStr(Temperatura, StringTemperatura,3,1);	//3 digitos parte inteira e 1 digito parte decimal
																
							//Coloca informação a ser enviada
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];						
						
							if(Temperatura <= TemperaturaMinima_C) {
								//Sensor de temperatura desconectado
								msg[3] = TamanhoTX_ComandoLeTemperatura_SensorDesconectado; //Resposta com apenas 1 byte
								msg[4] = TX_SensorDesconectado; //Sensor de temperatura desconectado
							
								//Calcula CRC
								ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
							
								//Acrescenta CRC no pacote
								msg[5] = ManipulaBytes.bytes[0];	//LSB
								msg[6] = ManipulaBytes.bytes[1];	//MSB
							
								msg[7] = Caractere_Fim;				//fim do pacote		

								//Coloca informação na FILA
								AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

							} else {
								msg[3] = TamanhoTX_ComandoLeTemperatura; 
								//Sensor de temperatura conectado
								msg[4] = TX_SensorConectado; //Sinaliza OK
													
								msg[5] = StringTemperatura[0];	//Sinal
								msg[6] = StringTemperatura[1];
								msg[7] = StringTemperatura[2];
								msg[8] = StringTemperatura[3];
								msg[9] = StringTemperatura[4];
								msg[10] = StringTemperatura[5];
								
								//Calcula CRC
								ManipulaBytes.u_int = crc16_XMODEM_TX(msg,11);
						
								//Acrescenta CRC no pacote
								msg[11] = ManipulaBytes.bytes[0];		//LSB
								msg[12] = ManipulaBytes.bytes[1];	//MSB
						
								msg[13] = Caractere_Fim;				//fim do pacote			

								//Coloca informação na FILA
								AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,14,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
				
							}		

							InicializaMaquinaDeEstados_Principal(M_Ocioso);
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;						
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
			
					case ComandoRequisitaCompraDeProduto:	//0x18

						//Multi carrinho
						if(((LigaDesliga_SensorDeQueda == false) && (LigaDesliga_Elevador == true)) ||
							((LigaDesliga_SensorDeQueda == true) && (LigaDesliga_Elevador == false)) ||
							((LigaDesliga_SensorDeQueda == false) && (LigaDesliga_Elevador == false))) {
							//MULTICARRINHO HABILITADO

							//Quantidade de bytes com informação correta de compra dentro do range?
							if((Pacote_USART0[3] >= TamanhoRX_ComandoRequisitaCompraDeProdutoMinimo) && (Pacote_USART0[3] <= TamanhoRX_ComandoRequisitaCompraDeProdutoMaximo)) {
								//Verifica se tamanho ou numero de compras está correspondente com os produtos enviados
								//Observação CRC foi removido do vetor "Pacote_USART0"

								if(Pacote_USART0[(Pacote_USART0[3] + 4)] == 0x1E) {
									//Recebido OK

									//Valida se todos produtos solicitados estão dentro do range de atuadores (1 a 72)
									for(i = 0 ; i < Pacote_USART0[3] ; i++) {
										if((Pacote_USART0[i+4] >= RequisitaProdutoMinimo) && (Pacote_USART0[i+4] <= RequisitaProdutoMaximo)) NumProdutosOK++;							
									}

									//Se todas as compras enviadas estão dentro do range dos atuadores
									if(Pacote_USART0[3] == NumProdutosOK) {
										//Comando completo recebido OK

										//Limpa fila de produtos
										LimpaVetor(FilaCompras, TamanhoFilaCompras);

										//Limpa fila de status das compras
										for(unsigned char i = 1 ; i < TamanhoFilaCompras ; i++) FilaStatusCompras[i] = ProdutoNaoSolicitado;

										//Coloca no inicio da fila a quantidade de itens da fila							
										FilaCompras[0] = Pacote_USART0[3];	

										//Copia atuadores a serem acionados
										for(i = 0 ; i < Pacote_USART0[3] ; i++) FilaCompras[i+1] = Pacote_USART0[i+4];
																
										//Garante que não entregou nem um item
										ItensEntregues = 0;
										Maquina_Principal.Estado = M_SolicitaCompra;

										InicializaMaquinaDeEstados_AUX_SolicitaCompra(M_SC_Verifica_FilaDeCompra);

										//Sinaliza que recebeu corretamente um comando
										PORTD^=0b10000000;																								
									} else {
										//Algum produto fora do range
										//Comando inválido
										RespondeNACK = true;	
									}
								} else {
									//Fim do pacote fora de posição
									NumProdutosOK = 0xFF;
									//Comando inválido
									RespondeNACK = true;									
								}
							} else {
								//Comando inválido
								RespondeNACK = true;
							}	
						} else {
							//MULTICARRINHO DESABILITADO
							if(Pacote_USART0[3] == TamanhoRX_ComandoRequisitaCompraDeProdutoMinimo) {

								//Limpa fila de produtos
								LimpaVetor(FilaCompras, TamanhoFilaCompras);

								//Limpa fila de status das compras
								for(unsigned char i = 1 ; i < TamanhoFilaCompras ; i++) FilaStatusCompras[i] = ProdutoNaoSolicitado;	

								//Coloca no inicio da fila a quantidade de itens da fila							
								FilaCompras[0] = Pacote_USART0[3];	

								//Copia atuador a ser acionado
								FilaCompras[1] = Pacote_USART0[4];
																
								//Garante que não entregou nem um item
								ItensEntregues = 0;
								Maquina_Principal.Estado = M_SolicitaCompra;

								InicializaMaquinaDeEstados_AUX_SolicitaCompra(M_SC_Verifica_FilaDeCompra);

								//Sinaliza que recebeu corretamente um comando
								PORTD^=0b10000000;	
							} else {
								//Envio de compra multicarrinho com multicarrinho desabilitado

								//Limpa fila de status das compras
								for(unsigned char i = 1 ; i < TamanhoFilaCompras ; i++) FilaStatusCompras[i] = ProdutoNaoSolicitado;	

								LimpaVetor(msg,Tamanho_BufferUSART0);
								//Coloca informação a ser enviada
								msg[0] = Caractere_Inicio;
								msg[1] = ProtocoloVersao;
								msg[2] = ComandoRequisitaCompraDeProduto;
								msg[3] = TamanhoRX_ComandoRequisitaCompraDeProdutoMaximo;
								msg[4] = Multicarrinho_Desabilitado;	//Status da compra do 1° produto
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
							}								
						}

					break;
			
					case ComandoAutoTeste:	//0x19 
						//Quantidade de bytes com informação correta e parâmetro OK?
						if((Pacote_USART0[3] == TamanhoRX_ComandoAutoTeste) &&
						   ((Pacote_USART0[4] >= Parametro_RealizaAutoTeste) && (Pacote_USART0[4] <= Parametro_RespondeResultadoDoUltimoAutoTeste))) {

							//Comando completo recebido OK															
						
							if(Pacote_USART0[4] == Parametro_RealizaAutoTeste) {
								//Habilita a resposta do autoteste
								EnviaResultadoDoAutotesteAoFinalizar = 1;
														
								InicializaMaquinaDeEstados_AUX_Autoteste(M_AT_TestaFusivel_Atuadores);
							
								Maquina_Principal.Estado = M_AutoTeste;
							
							} else if(Pacote_USART0[4] == Parametro_RespondeResultadoDoUltimoAutoTeste) {
								Retorna_StatusDoAutoteste();						
							}
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
				
				
					case ComandoConfiguraTipoDosAtuadoresDaGaveta:	//0x1A
						
						//Quantidade de bytes com informação correta e parâmetro OK?
						if(Pacote_USART0[3] == TamanhoRX_ComandoConfiguraTipoDosAtuadoresDaGaveta) {
						   if((Pacote_USART0[4] >= Gaveta_Inicial) && (Pacote_USART0[4] <= Gaveta_Final)) {  

								//Conta o número de atuadores dentro do range permitido
								for(i = 5 ; i <= (NumeroAtuadoresNaGaveta + 5) ; i++) {
									if((Pacote_USART0[i] >= Motor_Mola_Simples) && (Pacote_USART0[i] <= Esteira)) {
										_Valida_Atuadores++;
									}
								}

								LimpaVetor(msg,Tamanho_BufferUSART0);
								//Coloca informação a ser enviada
								msg[0] = Pacote_USART0[0];
								msg[1] = Pacote_USART0[1];
								msg[2] = Pacote_USART0[2];
								msg[3] = TamanhoTX_ComandoConfiguraTipoDosAtuadoresDaGaveta;

								//Se os 12 atuadores estiverem dentro do range 
								if(_Valida_Atuadores == NumeroAtuadoresNaGaveta) {
									//Salva os atuadores de 1 a 12 da gaveta na eeprom
									for(i = 1 ; i <= NumeroAtuadoresNaGaveta ; i++) {
										Escreve_uchar_eeprom((End_EEPROM_Tipo_De_Atuador + ((Pacote_USART0[4] - Gaveta_Inicial) * NumeroAtuadoresNaGaveta) + i), Pacote_USART0[4 + i]);
									}	

									Atualiza_TipoDeAtuador();
									
									//Responde que configurou os atuadores da gaveta
									msg[4] = ConfiguraTipoDosAtuadoresDaGaveta_Ok;								
						
								} else {
									//Responde que houve falha na configuração dos atuadores da gaveta
									msg[4] = ConfiguraTipoDosAtuadoresDaGaveta_Falha;	
								}

								//Calcula CRC
								ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);					
								
								//Acrescenta CRC no pacote
								msg[5] = ManipulaBytes.bytes[0];		//LSB
								msg[6] = ManipulaBytes.bytes[1];	//MSB							
								msg[7] = Caractere_Fim;				//fim do pacote		

								//Coloca informação na FILA
								AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

								//Sinaliza que recebeu corretamente um comando
								PORTD^=0b10000000;
						   }
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
				
					case ComandoLeConfiguracaoDosAtuadoresDaGaveta: //0x1B
						//Quantidade de bytes com informação correta e parâmetro OK?
						if((Pacote_USART0[3] == TamanhoRX_ComandoLeConfiguracaoDosAtuadoresDaGaveta) &&
						  ((Pacote_USART0[4] >= Gaveta_Inicial) && (Pacote_USART0[4] <= Gaveta_Final))) {

							LimpaVetor(msg,Tamanho_BufferUSART0);
												
							//Coloca informação a ser enviada
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];
							msg[3] = TamanhoTX_ComandoLeConfiguracaoDosAtuadoresDaGaveta; //bytes	
							msg[4] = Pacote_USART0[4]; //Gaveta que está sendo lida
							for(i = 1 ; i <= NumeroAtuadoresNaGaveta ; i++) {
								msg[i + 4] = Le_uchar_eeprom((End_EEPROM_Tipo_De_Atuador + ((Pacote_USART0[4] - Gaveta_Inicial) * NumeroAtuadoresNaGaveta) + i));//Atuador da Gaveta
							}

							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,17);					
							
							//Acrescenta CRC no pacote
							msg[17] = ManipulaBytes.bytes[0];		//LSB
							msg[18] = ManipulaBytes.bytes[1];	//MSB							
							msg[19] = Caractere_Fim;				//fim do pacote
						
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,20,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

							InicializaMaquinaDeEstados_Principal(M_Ocioso);
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
				
					case ComandoAcionaPortinhola: //0x1C
						//Quantidade de bytes com informação correta e parâmetro OK?
						if((Pacote_USART0[3] == TamanhoRX_ComandoAcionaPortinhola) &&
						  ((Pacote_USART0[4] >= AbrePortinhola) && (Pacote_USART0[4] <= FechaPortinhola))) {
						
							if(Pacote_USART0[4] == AbrePortinhola) AcionaSolenoide(true);
							else if(Pacote_USART0[4] == FechaPortinhola) AcionaSolenoide(false);
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
			
					case ComandoLeInformacoesDeIdentificacaoDaMaquina:	//0x1D
						//Quantidade de bytes com informação correta e parâmetro OK?
						if((Pacote_USART0[3] == TamanhoRX_ComandoLeInformacoesDeIdentificacaoDaMaquina) &&
						  (Pacote_USART0[4] == Parametro_LeInformacoesDeIdentificacaoDaMaquina)) {

							//Comando completo recebido OK
						
							LimpaVetor(msg,Tamanho_BufferUSART0);
						
							//Coloca informação a ser enviada
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];

							//Lê Número de série da EEPROM						
							for(i = 0 ; i < Comprimento_Numero_de_serie ; i++) {
								Numero_de_Serie[i] = Le_uchar_eeprom(End_EEPROM_Numero_de_serie + i);
							}

							//Modelo
							for(i = 0 ; i < Comprimento_Modelo ; i++) {
								Modelo[i] = Le_uchar_eeprom(End_EEPROM_Modelo + i);
							}
							//Versão de Hardware
							for(i = 0 ; i < Comprimento_Versao_de_Hardware ; i++) {
								Versao_de_Hardware[i] = Le_uchar_eeprom(End_EEPROM_Versao_de_Hardware + i);
							}
							//Versão de Firmware
							for(i = 0 ; i < Comprimento_Versao_de_Firmware ; i++) {
								Versao_de_Firmware[i] = Le_uchar_eeprom(End_EEPROM_Versao_de_Firmware + i);
							}
								
							Tamanho_String = VerificaTamanho(Numero_de_Serie);
							msg[3] = Tamanho_String + 2; //bytes soma 0x02 e 0x03
						
							r = 4;	//indice inicia a escrita do numero de série

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!						
							msg[r] = CaracterInicio_Informacao; //inicio do numero de série
							r++;
							//Copia número de série para a resposta
							for(i = 0 ; i < Tamanho_String ; i++) {							
								msg[r] = Numero_de_Serie[i];
								r++; //conta bytes do numero de série
							}

							msg[r] = CaracterFim_Informacao;	//Fim do número de série									
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!				
							Tamanho_String = VerificaTamanho(Modelo);
							msg[3] += Tamanho_String + 2;	//Soma tamanho do pacote
						
							r++;
							msg[r] = CaracterInicio_Informacao;	//Inicio do modelo
							r++;
							//Copia modelo para a resposta
							for(unsigned char i = 0 ; i < Tamanho_String ; i++) {							
								msg[r] = Modelo[i];
								r++; //conta bytes do modelo
							}
							msg[r] = CaracterFim_Informacao;	//Fim do modelo				
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!																				
							Tamanho_String = VerificaTamanho(Versao_de_Hardware);
							msg[3] += Tamanho_String + 2;	//Soma tamanho do pacote

							r++;
							msg[r] = CaracterInicio_Informacao; //Inicio da versão de hardware
							r++;
							//Copia modelo para a resposta
							for(unsigned char i = 0 ; i < Tamanho_String ; i++) {							
								msg[r] = Versao_de_Hardware[i];
								r++; //conta bytes da versão de hardware
							}						
							msg[r] = CaracterFim_Informacao;	//Fim da versão de hardware
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!	
							Tamanho_String = VerificaTamanho(Versao_de_Firmware);
							msg[3] += Tamanho_String + 2;	//Soma tamanho do pacote

							r++;
							msg[r] = CaracterInicio_Informacao;	//Inicio da versão de firmware
							r++;
							//Copia modelo para a resposta
							for(unsigned char i = 0 ; i < Tamanho_String ; i++) {							
								msg[r] = Versao_de_Firmware[i];
								r++; //conta bytes da versão de firmware
							}						
							msg[r] = CaracterFim_Informacao;	//Fim da versão de firmware						
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!							
							r++;						
							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,r);
						
							//Acrescenta CRC no pacote
							msg[r] = ManipulaBytes.bytes[0];		//LSB
							msg[r+1] = ManipulaBytes.bytes[1];	//MSB
							msg[r+2] = Caractere_Fim;				//fim do pacote
						
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,(r+3),true,TempoEsperaResposta_ms,1);
						  
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
				
					case ComandoConfiguraInformacoesDeIdentificacaoDaMaquina: //0x1E
								
						//Quantidade de bytes com informação correta e motor dentro do range?
						if((Pacote_USART0[3] >= TamanhoRX_Minimo_ComandoConfiguraInformacoesDeIdentificacaoDaMaquina) && (Pacote_USART0[3] <= TamanhoRX_Maximo_ComandoConfiguraInformacoesDeIdentificacaoDaMaquina)) {
						
							//Comando completo recebido OK
						
							if(Pacote_USART0[4] == CaracterInicio_Informacao) {
								//Inicio do número de série OK
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!	
								LimpaVetor(Numero_de_Serie, Comprimento_Numero_de_serie);							
								r = 5;	//indice inicial de leitura das informações da máquina
								x = 0; //indica que não recebeu o fim do numero de série
							
								for(i = 0 ; i < Comprimento_Numero_de_serie ; i++) {
									if(Pacote_USART0[r] != CaracterFim_Informacao) {
										Numero_de_Serie[i] = Pacote_USART0[r];	//Copia novo numero de série para o vetor							
										r++; //Passa para o próximo byte									
									} else {
										//Recebeu todo o numero de série 
										x++; // indica que recebeu o fim do numero de série									
										r++; //Passa para o próximo byte
										break; //força a saída do FOR									
									}
								}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!	
								LimpaVetor(Modelo, Comprimento_Modelo);							
								if(Pacote_USART0[r] == CaracterInicio_Informacao) {
									//inicio do modelo OK

									r++;									
									for(i = 0 ; i < Comprimento_Modelo ; i++) {
										if(Pacote_USART0[r] != CaracterFim_Informacao) {
											Modelo[i] = Pacote_USART0[r];	//Copia novo modelo para vetor
											r++; //Passa para o próximo byte
										} else {
											//Recebeu todo modelo										
											x++; // indica que recebeu o fim do modelo
											r++; //Passa para o próximo byte
											break;	//força a saída do FOR
										}
									}								
								}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!	
								LimpaVetor(Versao_de_Hardware, Comprimento_Versao_de_Hardware);
								if(Pacote_USART0[r] == CaracterInicio_Informacao) {
									//inicio do versão de hardware

									r++;
									for(i = 0 ; i < Comprimento_Versao_de_Hardware ; i++) {
										if(Pacote_USART0[r] != CaracterFim_Informacao) {
											Versao_de_Hardware[i] = Pacote_USART0[r];	//Copia nova versão de hardware para vetor
											r++; //Passa para o próximo byte
											} else {
											//Recebeu toda versão de hardware
											x++; // indica que recebeu o fim da versão de hardware
											r++; //Passa para o próximo byte
											break;	//força a saída do FOR
										}
									}
								}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!								
								LimpaVetor(Versao_de_Firmware, Comprimento_Versao_de_Firmware);
								if(Pacote_USART0[r] == CaracterInicio_Informacao) {
									//inicio do versão de firmware

									r++;
									for(i = 0 ; i < Comprimento_Versao_de_Firmware ; i++) {
										if(Pacote_USART0[r] != CaracterFim_Informacao) {
											Versao_de_Firmware[i] = Pacote_USART0[r];	//Copia nova versão de firmware para vetor
											r++; //Passa para o próximo byte
										} else {
											//Recebeu toda versão de firmware

											x++; // indica que recebeu o fim da versão de firmware
											break;	//força a saída do FOR
										}
									}
								}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!					
								//Verifica se recebeu todas as informações corretamente							
								if(x == 4) {
									//Sim								
									x = 0;
								
									//Salva as informações da EEPROM
									for(i = 0 ; i < Comprimento_Numero_de_serie ; i++) {
											Escreve_uchar_eeprom(End_EEPROM_Numero_de_serie + i, Numero_de_Serie[i]);
									}
								
									for(i = 0 ; i < Comprimento_Modelo ; i++) {
										Escreve_uchar_eeprom(End_EEPROM_Modelo + i, Modelo[i]);
									}
								
									for(i = 0 ; i < Comprimento_Versao_de_Hardware ; i++) {
										Escreve_uchar_eeprom(End_EEPROM_Versao_de_Hardware + i, Versao_de_Hardware[i]);
									}
								
									for(i = 0 ; i < Comprimento_Versao_de_Firmware ; i++) {
										Escreve_uchar_eeprom(End_EEPROM_Versao_de_Firmware + i, Versao_de_Firmware[i]);
									}
								} 							
							}

							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;						

						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;				
				
					case ComandoConfiguraParametroDoControladorDeTemperatura://0x1F
					
						//Quantidade de bytes com informação correta?
						if((Pacote_USART0[3] == Tamanho_Comando_Temperatura_ParametroDoControladorDeTemperatura) ||
						   (Pacote_USART0[3] == Tamanho_Comando_Tempo_ParametroDoControladorDeTemperatura) ||
						   (Pacote_USART0[3] == Tamanho_Comando_Buzzer_ParametroDoControladorDeTemperatura)) {

							if((Pacote_USART0[4] >= Parametro_SetPoint_C_ControladorDeTemperatura) && (Pacote_USART0[4] <= Parametro_Buzzer_ControladorDeTemperatura)) {
								//Parâmetro dentro do range

								//Comando completo recebido OK						
								//Sinaliza que recebeu corretamente um comando
								PORTD^=0b10000000;

								ManipulaBytes.u_int = SelecionaRegistradorModbusDoControladorDeTemperatura(Pacote_USART0[4]);
								//Atribui o registrador correspondente ao parâmetro do controlador de temperatura

								if(ManipulaBytes.u_int != 0xFFFF) {
									//Comando válido para o controlador de temperatura

									//Lê valor da configuração
									LimpaVetor(Buffer_ControladorDeTemperatura_Conversao, Tamanho_Buffer_ControladorDeTemperatura_Conversao);
						
									Maquina_Principal.Estado = M_ComunicandoCom_ControladorDeTemperatura;

									//Coloca informação na fila do controlador de temperatura
									//Inicia montagem do buffer para enviar para controlador de temperatura								 

									IdentificaComando_ControladorDeTemperatura = Configura_ControladorDeTemperatura;
									Identifica_Parametro_ControladorDeTemperatura = Pacote_USART0[4];


								
									if((Pacote_USART0[4] >= Parametro_SetPoint_C_ControladorDeTemperatura) && (Pacote_USART0[4] <= Parametro_AlarmeTemperaturaAmbienteAlta_C_ControladorDeTemperatura)) {						
										
										Buffer_ControladorDeTemperatura_Conversao[0] = Pacote_USART0[5];
										Buffer_ControladorDeTemperatura_Conversao[1] = Pacote_USART0[6];
										Buffer_ControladorDeTemperatura_Conversao[2] = Pacote_USART0[7];
										Buffer_ControladorDeTemperatura_Conversao[3] = Pacote_USART0[8];
										Buffer_ControladorDeTemperatura_Conversao[4] = Pacote_USART0[9];
										Buffer_ControladorDeTemperatura_Conversao[5] = Pacote_USART0[10];

										LimpaVetor(msg,Tamanho_BufferUSART0);
										msg[0] = Modbus_Endereco_ControladorDeTemperatura;
										msg[1] = Modbus_EscritaUnicoRegistrador;																
										msg[2] = ManipulaBytes.bytes[1];
										msg[3] = ManipulaBytes.bytes[0];

										//Converte STRING para FLOAT
										Aux_f = StrToFloat(Buffer_ControladorDeTemperatura_Conversao, 1);
										
										//Multiplica por 10 
										Aux_f *= 10.0;
										//Remove parte decimal
										ManipulaBytes._int = (int) Aux_f;
						
										msg[4] = ManipulaBytes.bytes[1]; //MSB
										msg[5] = ManipulaBytes.bytes[0]; //LSB

										//Calcula CRC16 MODBUS
										ManipulaBytes.u_int = Calcula_CRC16_MODBUS(msg,6);
										msg[6] = ManipulaBytes.bytes[0];	//MSB
										msg[7] = ManipulaBytes.bytes[1];	//LSB
										AdicionaMsgFila_ControladorDeTemperatura(msg, TempoEsperaParaEnviarPacote_ControladorDeTemperatura_ms, 8, TempoEsperaParaReceberResposta_ControladorDeTemperatura_ms, NumeroDeTentativas_EnvioComando_ControladorDeTemperatura);
									} else if((Pacote_USART0[4] >= Parametro_IntervaloEntreDegelos_Minutos_ControladorDeTemperatura) && (Pacote_USART0[4] <= Parametro_TempoDeIndicacaoDeTemperaturaTravada_Minutos_ControladorDeTemperatura)) {
									
										Buffer_ControladorDeTemperatura_Conversao[0] = Pacote_USART0[5];
										Buffer_ControladorDeTemperatura_Conversao[1] = Pacote_USART0[6];
										Buffer_ControladorDeTemperatura_Conversao[2] = Pacote_USART0[7];
										Buffer_ControladorDeTemperatura_Conversao[3] = Pacote_USART0[8];
#warning ADICIONADA LEITURA DO 4 DIGITO NA CONFIGURACAO
										LimpaVetor(msg,Tamanho_BufferUSART0);
										msg[0] = Modbus_Endereco_ControladorDeTemperatura;
										msg[1] = Modbus_EscritaUnicoRegistrador;													
										msg[2] = ManipulaBytes.bytes[1];
										msg[3] = ManipulaBytes.bytes[0];
	
										//Converte STRING para FLOAT
										Aux_f = StrToFloat(Buffer_ControladorDeTemperatura_Conversao, 0);
										ManipulaBytes._int = (int) Aux_f;
						
										msg[4] = ManipulaBytes.bytes[1]; //MSB
										msg[5] = ManipulaBytes.bytes[0]; //LSB

										//Calcula CRC16 MODBUS
										ManipulaBytes.u_int = Calcula_CRC16_MODBUS(msg,6);
										msg[6] = ManipulaBytes.bytes[0];	//MSB
										msg[7] = ManipulaBytes.bytes[1];	//LSB
										AdicionaMsgFila_ControladorDeTemperatura(msg, TempoEsperaParaEnviarPacote_ControladorDeTemperatura_ms, 8, TempoEsperaParaReceberResposta_ControladorDeTemperatura_ms, NumeroDeTentativas_EnvioComando_ControladorDeTemperatura);
									} else if(Pacote_USART0[4] == Parametro_Buzzer_ControladorDeTemperatura) {
										Buffer_ControladorDeTemperatura_Conversao[0] = Pacote_USART0[5];

										LimpaVetor(msg,Tamanho_BufferUSART0);
										msg[0] = Modbus_Endereco_ControladorDeTemperatura;
										msg[1] = Modbus_EscritaUnicoRegistrador;																
										msg[2] = ManipulaBytes.bytes[1];
										msg[3] = ManipulaBytes.bytes[0];

										//Converte STRING para FLOAT
										Aux_f = StrToFloat(Buffer_ControladorDeTemperatura_Conversao, 0);
										ManipulaBytes._int = (int) Aux_f;
						
										msg[4] = ManipulaBytes.bytes[1]; //MSB
										msg[5] = ManipulaBytes.bytes[0]; //LSB

										//Calcula CRC16 MODBUS
										ManipulaBytes.u_int = Calcula_CRC16_MODBUS(msg,6);
										msg[6] = ManipulaBytes.bytes[0];	//MSB
										msg[7] = ManipulaBytes.bytes[1];	//LSB
										AdicionaMsgFila_ControladorDeTemperatura(msg, TempoEsperaParaEnviarPacote_ControladorDeTemperatura_ms, 8, TempoEsperaParaReceberResposta_ControladorDeTemperatura_ms, NumeroDeTentativas_EnvioComando_ControladorDeTemperatura);
									}							
								} else {
									//Comando inválido responde pela USART 
									
									LimpaVetor(msg,Tamanho_BufferUSART0);
									//Coloca informação a ser enviada
									msg[0] = Pacote_USART0[0];
									msg[1] = Pacote_USART0[1];
									msg[2] = Pacote_USART0[2];
									msg[3] = 0x01;
									msg[4] = Resposta_ControladorDeTemperatura_ComandoInvalido;
							
									//Calcula CRC
									ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
									//Acrescenta CRC no pacote
									msg[5] = ManipulaBytes.bytes[0]; //LSB
									msg[6] =	ManipulaBytes.bytes[1]; //MSB
							
									msg[7] = Caractere_Fim;	//fim
							
									//Coloca informação na FILA
									AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);								
								}
							}
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
				
					case ComandoResetPlacaControladora:	//0x20
						//Quantidade de bytes com informação correta e motor dentro do range?
						if((Pacote_USART0[3] == TamanhoRX_ComandoResetPlacaControladora) &&
						   (Pacote_USART0[4] == ComandoReset)) {
							//Comando completo recebido OK
						
							wdt_enable(WDTO_250MS);
							//Espera momento de responder ACK
							while(1) {
								//Verifica se deve responder para MiniPC e USART0 não está em uso pelo ModBus
								if((Flag_EnviaInformacao_MiniPC == true) && (Flag_USART0_Ocupada == false)) {
									Flag_EnviaInformacao_MiniPC = false;
									//Envia mensagem da posição 0 da fila
									EnviaStringUSART0(Fila_MiniPC[0].msg,VerificaTamanho(Fila_MiniPC[0].msg),0,_USART);
									//Sinaliza que já enviou resposta para o MiniPC
									Fila_MiniPC[0].Flag_JaEnviouResposta = true;
								}
							}
						
						} else {
							//Comando inválido
							RespondeNACK = true;
						}					

					break;
			
					case ComandoVerificaStatusDaPlacaControladora:	//0x21
						//Quantidade de bytes com informação correta?
						if((Pacote_USART0[3] == TamanhoRX_ComandoStatusPlacaControladora) &&
						   (Pacote_USART0[4] == Parametro_VerificaStatusDaPlacaControladora)){
							//Sim
						
							//Comando completo recebido OK
												
							LimpaVetor(msg,Tamanho_BufferUSART0);
							//Coloca informação a ser enviada
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];
							msg[3] = TamanhoTX_ComandoStatusPlacaControladora;
							msg[4] = OK;
					
							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
							//Acrescenta CRC no pacote
							msg[5] = ManipulaBytes.bytes[0]; //LSB
							msg[6] =	ManipulaBytes.bytes[1]; //MSB
					
							msg[7] = Caractere_Fim;	//fim
				
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

							//InicializaMaquinaDeEstados_Principal(M_Ocioso);
							#warning REMOVER DE TODAS AS VERSOES
							
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;																
						} else {
							//Comando inválido
							RespondeNACK = true;			
						}					
					break;	
				
					case ComandoLeStatusDosAtuadoresDaGaveta:	//0x22
						//Quantidade de bytes com informação correta e parâmetro OK?
						if((Pacote_USART0[3] == TamanhoRX_ComandoLeStatusDosAtuadoresDaGaveta) &&
						  ((Pacote_USART0[4] >= Gaveta_Inicial) && (Pacote_USART0[4] <= Gaveta_Final))) {

							LimpaVetor(msg,Tamanho_BufferUSART0);
												
							//Coloca informação a ser enviada
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];
							msg[3] = TamanhoTX_ComandoLeStatusDosAtuadoresDaGaveta; //bytes	
							msg[4] = Pacote_USART0[4]; //Gaveta que está sendo lida
							for(i = 1 ; i <= NumeroAtuadoresNaGaveta ; i++) {
								msg[i + 4] = AtuadorIdentificado[(((Pacote_USART0[4] - Gaveta_Inicial) * NumeroAtuadoresNaGaveta) + i)];//Atuador da Gaveta
							}

							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,17);					
							
							//Acrescenta CRC no pacote
							msg[17] = ManipulaBytes.bytes[0];		//LSB
							msg[18] = ManipulaBytes.bytes[1];		//MSB							
							msg[19] = Caractere_Fim;				//fim do pacote
						
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,20,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

							InicializaMaquinaDeEstados_Principal(M_Ocioso);
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
				
					case ComandoLeStatusDaPortaPrincipal: //0x23
						//Quantidade de bytes com informação correta e parâmetro OK?
						if((Pacote_USART0[3] == TamanhoRX_ComandoLeStatusDaPortaPrincipal) &&
						   (Pacote_USART0[4] == Parametro_LeStatusDaPortaPrincipal)) {
						   
							 //Comando completo recebido OK
						   						
							 LimpaVetor(msg,Tamanho_BufferUSART0);
							 //Coloca informação a ser enviada
							 msg[0] = Pacote_USART0[0];
							 msg[1] = Pacote_USART0[1];
							 msg[2] = Pacote_USART0[2];
							 msg[3] = TamanhoRX_ComandoLeStatusDaPortaPrincipal;
						 
							 if(VerificaPortaPrincipal() == 1) {
								//Porta aberta
								msg[4] = PortaPrincipalAberta;
							 } else {
								 //Porta fechada
								 msg[4] = PortaPrincipalFechada;
							 }
						 
							 //Calcula CRC
							 ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
							 //Acrescenta CRC no pacote
							 msg[5] = ManipulaBytes.bytes[0]; //LSB
							 msg[6] =	ManipulaBytes.bytes[1]; //MSB
						
							 msg[7] = Caractere_Fim;	//fim
						
							 //Coloca informação na FILA
							 AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
							
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
				
					case ComandoRealizaDegeloImediato: //0x24
						//Quantidade de bytes com informação correta e parâmetro OK?
						if((Pacote_USART0[3] == TamanhoRX_ComandoRealizaDegeloImediato) &&
						(Pacote_USART0[4] == Parametro_RealizaDegeloImediato)) {

							Maquina_Principal.Estado = M_ComunicandoCom_ControladorDeTemperatura;
						
							//Inicia temporizador
							TempoEsperaPulsoDoDegelo_ControladorDeTemperatura_s = DuracaoDoPulsoParaRealizarDegelo_ControladorDeTemperatura_s;
												
							//Inicia pulso do Degelo
							Saida_74HC595.DEGELO = 1;
							Escreve74HC595(Saida_74HC595.Expansor);
												
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						} else {
							//Comando inválido
							RespondeNACK = true;
						}						
					break;
//Comando 0x25 não usar
					case ComandoConfiguraElevador:	//0x26
						//Quantidade de bytes com informação correta?
						if((Pacote_USART0[3] == TamanhoRX_ComandoLigaDesligaElevador) &&
						((Pacote_USART0[4] >= Parametro_DesabilitaElevador) && (Pacote_USART0[4] <= Parametro_HabilitaElevador)) ) {
										
							if (Pacote_USART0[4] == Parametro_HabilitaElevador) LigaDesliga_Elevador = true;
							else if(Pacote_USART0[4] == Parametro_DesabilitaElevador) LigaDesliga_Elevador = false;
							Escreve_uchar_eeprom(End_EEPROM_Elevador_Ligado_Ou_Desligado, LigaDesliga_Elevador);
											
							//Desabilita a resposta do autoteste
							EnviaResultadoDoAutotesteAoFinalizar = 0;	
						
							InicializaMaquinaDeEstados_AUX_Autoteste(M_AT_TestaFusivel_Atuadores);						
							Maquina_Principal.Estado = M_AutoTeste;						
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
										
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;
					
					case ComandoLeConfiguracaoDoElevador:	//0x27
						//Quantidade de bytes com informação correta?
						if((Pacote_USART0[3] == TamanhoRX_ComandoLeConfiguracaoDoElevador) &&
						   (Pacote_USART0[4] == Parametro_LeConfiguracaoDoElevador)) {

							//Comando completo recebido OK
	
							LimpaVetor(msg,Tamanho_BufferUSART0);
							//Coloca informação a ser enviada
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];
							msg[3] = TamanhoRX_ComandoLeConfiguracaoDoElevador;
							msg[4] = LigaDesliga_Elevador+1; //1 Desligado e 2 Ligado
	
							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
							//Acrescenta CRC no pacote
							msg[5] = ManipulaBytes.bytes[0]; //LSB
							msg[6] = ManipulaBytes.bytes[1]; //MSB
	
							msg[7] = Caractere_Fim;	//fim
	
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;

						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;	

					case ComandoConfigura_SensorDeQueda:	//0x28
						//Quantidade de bytes com informação correta?
						if((Pacote_USART0[3] == TamanhoRX_ComandoConfigura_SensorDeQueda) &&
							((Pacote_USART0[4] >= Parametro_DesabilitaSensorDeQueda) && (Pacote_USART0[4] <= Parametro_HabilitaSensorDeQueda)) ) {
										
							if (Pacote_USART0[4] == Parametro_HabilitaSensorDeQueda) LigaDesliga_SensorDeQueda = true;
							else if(Pacote_USART0[4] == Parametro_DesabilitaSensorDeQueda) LigaDesliga_SensorDeQueda = false;

							//Salva configuração do Sensor de queda LIGADO
							Escreve_uchar_eeprom(End_EEPROM_SensorDeQueda_Ligado_Ou_Desligado, LigaDesliga_SensorDeQueda);				
						
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
										
						} else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;

					case ComandoLeConfiguracao_SensorDeQueda:	//0x29
						//Quantidade de bytes com informação correta?
						if((Pacote_USART0[3] == TamanhoRX_ComandoLeConfiguracao_SensorDeQueda) &&
						   (Pacote_USART0[4] == Parametro_LeConfiguracao_SensorDeQueda)) {

							//Comando completo recebido OK
	
							LimpaVetor(msg,Tamanho_BufferUSART0);
							//Coloca informação a ser enviada
							msg[0] = Pacote_USART0[0];
							msg[1] = Pacote_USART0[1];
							msg[2] = Pacote_USART0[2];
							msg[3] = TamanhoRX_ComandoLeConfiguracaoDoElevador;
							msg[4] = LigaDesliga_SensorDeQueda+1; //1 Desligado e 2 Ligado
	
							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
							//Acrescenta CRC no pacote
							msg[5] = ManipulaBytes.bytes[0]; //LSB
							msg[6] = ManipulaBytes.bytes[1]; //MSB
	
							msg[7] = Caractere_Fim;	//fim
	
							//Coloca informação na FILA
							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						}  else {
							//Comando inválido
							RespondeNACK = true;
						}
					break;

					case ComandoLeConfiguracaoDoControladorDeTemperatura: //0x2A
						//Quantidade de bytes com informação correta?
						if(Pacote_USART0[3] == TamanhoRX_ComandoLeConfiguracaoDoControladorDeTemperatura) {

							if((Pacote_USART0[4] >= Parametro_SetPoint_C_ControladorDeTemperatura) && (Pacote_USART0[4] <= Parametro_Buzzer_ControladorDeTemperatura)) {
								//Parâmetro dentro do range

								ManipulaBytes.u_int = SelecionaRegistradorModbusDoControladorDeTemperatura(Pacote_USART0[4]);
								//Atribui o registrador correspondente ao parâmetro do controlador de temperatura
							
								if(ManipulaBytes.u_int != 0xFFFF) {
									//Comando válido para o controlador de temperatura

									//Comando completo recebido OK								
									Maquina_Principal.Estado = M_ComunicandoCom_ControladorDeTemperatura;

									//Coloca informação na fila do controlador de temperatura
									//Inicia montgem do buffer para enviar para controlador de temperatura								 

									IdentificaComando_ControladorDeTemperatura = LeConfiguracao_ControladorDeTemperatura;
									Identifica_Parametro_ControladorDeTemperatura = Pacote_USART0[4];
	
									LimpaVetor(msg,Tamanho_BufferUSART0);
									msg[0] = Modbus_Endereco_ControladorDeTemperatura;
									msg[1] = Modbus_LeituraUnicoRegistrador;									
									msg[2] = ManipulaBytes.bytes[1];
									msg[3] = ManipulaBytes.bytes[0];
									msg[4] = 0x00;
									msg[5] = 0x01;
									//Calcula CRC16 MODBUS
									ManipulaBytes.u_int = Calcula_CRC16_MODBUS(msg,6);
									msg[6] = ManipulaBytes.bytes[0];	//MSB
									msg[7] = ManipulaBytes.bytes[1];	//LSB
									AdicionaMsgFila_ControladorDeTemperatura(msg, TempoEsperaParaEnviarPacote_ControladorDeTemperatura_ms, 8, TempoEsperaParaReceberResposta_ControladorDeTemperatura_ms, NumeroDeTentativas_EnvioComando_ControladorDeTemperatura);
													
						
									//Inicia maquina de estados da comunicação com o controlador de temperatura
									InicializaMaquinaDeEstados_ControladorDeTemperatura(Estado_CT_Inicia_MODBUS);	
								}else {
									//Comando inválido responde pela USART
								
									LimpaVetor(msg,Tamanho_BufferUSART0);
									//Coloca informação a ser enviada
									msg[0] = Pacote_USART0[0];
									msg[1] = Pacote_USART0[1];
									msg[2] = Pacote_USART0[2];
									msg[3] = 0x01;
									msg[4] = Resposta_ControladorDeTemperatura_ComandoInvalido;
								
									//Calcula CRC
									ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
									//Acrescenta CRC no pacote
									msg[5] = ManipulaBytes.bytes[0]; //LSB
									msg[6] =	ManipulaBytes.bytes[1]; //MSB
								
									msg[7] = Caractere_Fim;	//fim
								
									//Coloca informação na FILA
									AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
								}
							} 
							//Sinaliza que recebeu corretamente um comando
							PORTD^=0b10000000;
						
						} else {
							//Comando inválido
							RespondeNACK = true;
						}				
					break;					
				}	
							
			} else {
				//CRC Inválido responde NACK			
				RespondeNACK = true;
			}
		
		} 
	
	} else {
		//Maquina de estado principal não está no modo ocioso
		
		//Se estiver executando o auto teste responde apenas o COMANDO DE VERIFICAÇÃO DA PLACA CONTROLADORA 0x21
		//Caso placa demore realizando o auto teste quando o MiniPc verificar o status a placa sempre responderá
		if(Maquina_Principal.Estado == M_AutoTeste) {

			//Verifica se versão do protocolo está correta
			//Verifica quantidade de bytes com informação está dentro do range esperado
			//Verifica os comandos estão dentro do range esperado
			//Verifica caractere final está correto
			
			if( (Pacote_USART0[1] == ProtocoloVersao)  &&
			((Pacote_USART0[3] >= TamanhoRX_ParametroMINIMO) && (Pacote_USART0[3] <= TamanhoRX_ParametroMAXIMO)) &&	//0x01 a 0x02
			((Pacote_USART0[2]>= ComandoAcionaAtuador) && (Pacote_USART0[2]<= ComandoLeConfiguracaoDoElevador)) && //0x11 a 0x29
			(Pacote_USART0[TamanhoDoPacote-1] == Caractere_Fim)) {	//0x1E

				//Pacote recebido dentro dos padrões do protocolo

				//Lê CRC recebido
				ManipulaBytes.bytes[0] = Pacote_USART0[TamanhoDoPacote-3]; //LSB
				ManipulaBytes.bytes[1] = Pacote_USART0[TamanhoDoPacote-2]; //MSB
				CRCRecebido = ManipulaBytes.u_int;
		
				//Calcula CRC
				CRCCalculado = crc16_XMODEM_RX(Pacote_USART0,TamanhoDoPacote);

				//Verifica integridade da mensagem
				if(CRCCalculado == CRCRecebido) {
					//CRC Correto responde ACK
			
					//Limpa fila
					RemoveMsgFila_MiniPc(0xFF);		

					//Coloca ACK na fila
					LimpaVetor(msg,Tamanho_BufferUSART0);
					msg[0] = ACK;
					AdicionaMsgFila_MiniPc(msg,TempoRespostaIntegridade_ms,1,false,0,0);
			
					//Quantidade de bytes com informação correta?
					if((Pacote_USART0[2] == ComandoVerificaStatusDaPlacaControladora) && 
					   (Pacote_USART0[3] == TamanhoRX_ComandoStatusPlacaControladora) &&
					   (Pacote_USART0[4] == Parametro_VerificaStatusDaPlacaControladora)) {
						//Sim
					
						//Comando completo recebido OK
					
						LimpaVetor(msg,Tamanho_BufferUSART0);
						//Coloca informação a ser enviada
						msg[0] = Pacote_USART0[0];
						msg[1] = Pacote_USART0[1];
						msg[2] = Pacote_USART0[2];
						msg[3] = TamanhoTX_ComandoStatusPlacaControladora;
						msg[4] = OK;
					
						//Calcula CRC
						ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
						//Acrescenta CRC no pacote
						msg[5] = ManipulaBytes.bytes[0]; //LSB
						msg[6] =	ManipulaBytes.bytes[1]; //MSB
					
						msg[7] = Caractere_Fim;	//fim
					
						//Coloca informação na FILA
						AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);

						InicializaMaquinaDeEstados_Principal(M_Ocioso);
					
						//Sinaliza que recebeu corretamente um comando
						PORTD^=0b10000000;
					} else {
						//Comando inválido
						RespondeNACK = true;
					}					
				}//if(CRCCalculado == CRCRecebido) {
			}
		}//if(Maquina_Principal.Estado == M_AutoTeste) {
	}	
	
	//Verifica se deve responder NACK
	if(RespondeNACK == true) {
		RespondeNACK = false;
					
		//Limpa toda fila
		RemoveMsgFila_MiniPc(0xFF);
		
		//Limpa vetor para colocar a mensagem
		LimpaVetor(msg,Tamanho_BufferUSART0);
		//Coloca mensagem no vetor
		msg[0] = NACK;
		AdicionaMsgFila_MiniPc(msg,TempoRespostaIntegridade_ms,1,false,0,0);		
	}
			
	//Prepara para receber próximo pacote
	TamanhoDoPacote = 0;
	LimpaVetor(Pacote_USART0, Tamanho_BufferUSART0);	
}


/*Inicializa a fila da comunicação serial com o MiniPc
 * Parâmetros de entrada:
 * Parâmetro 1 - Número máximo de mensagens possível na fila
 *
 * Parâmetro de saída:
 * Não tem.
 */
void InicializaFila_MiniPc(volatile unsigned char TamanhoMaxFila) {
	volatile unsigned char i,x;
	
	//Inicializa a fila
	for(i = 0 ; i < TamanhoMaxFila ; i++) {
		//limpa tamanho da mensagem
		Fila_MiniPC[i].Tamanho_msg_bytes = 0;

		//limpa tempo de espera para enviar próxima mensagem
		Fila_MiniPC[i].TempoEsperaEnvioMsg = 0;
	
		//Limpa todo vetor da mensagem
		for(x = 0 ; x < Tamanho_BufferUSART0 ; x++) Fila_MiniPC[i].msg[x] = 0;
	
		//Limpa flag de status da resposta do Mini Pc
		Fila_MiniPC[i].EsperaRespostaMiniPc = false;
		
		//Limpa envio de resposta para Mini Pc
		Fila_MiniPC[i].TempoEsperaRespostaMiniPc = 0;
		Fila_MiniPC[i].TentativaEnvioRespostaMiniPc = 0;
	
		//Limpa flag indicando que não enviou resposta ao Mini Pc
		Fila_MiniPC[i].Flag_JaEnviouResposta = false;
	}	
	Num_msg_fila_MiniPC = 0;	//Inicia sem mensagens na fila da USART
} 

/* Adiciona mensagem na fila da comunicação serial com o MiniPc
 * Parâmetros de entrada:
 * Parâmetro 1 - Informações a serem enviadas 
 * Parâmetro 2 - Tempo em milisegundos para esperar para realizar o envio da mensagem
 * Parâmetro 3 - True deve esperar PACOTE de resposta do mestre e false não deve esperar PACOTE de resposta do mestre
 * Parâmetro 4 - Tempo em milisegundos para esperar retorno do mestre (ACK ou NACK)
 * Parâmetro 5 - Número de tentativas de envio da mensagem para o mestre
 *
 * Parâmetro de saída:
 * Não tem.
*/
void AdicionaMsgFila_MiniPc(volatile unsigned char *_msg,
volatile unsigned char _tempo_espera_envio_msg,
volatile unsigned char Tamanho,
volatile bool _espera_resposta_mini_pc,
volatile char _tempo_espera_resposta_mini_pc,
volatile char _tentativa_envio_resposta_mini_pc) {

	int i;
			
	//A fila não está cheia ?
	if(Num_msg_fila_MiniPC < TamanhoFilaUSART0) {//5
		//Não, ainda tem espaço na Fila
											
		//Limpa vetor que receberá a mensagem a ser adicionada na fila
		for(i = 0 ; i < Tamanho_BufferUSART0 ; i++) Fila_MiniPC[Num_msg_fila_MiniPC].msg[i] = (unsigned char) 0;
		
		//Adiciona o tamanho da mensagem
		Fila_MiniPC[Num_msg_fila_MiniPC].Tamanho_msg_bytes = Tamanho;
		
		//Adiciona mensagem 
		//for(i = 0 ; i < VerificaTamanho(_msg) ; i++) Fila_MiniPC[Num_msg_fila_MiniPC].msg[i] = _msg[i]; 
		for(i = 0 ; i < Fila_MiniPC[Num_msg_fila_MiniPC].Tamanho_msg_bytes ; i++) Fila_MiniPC[Num_msg_fila_MiniPC].msg[i] = _msg[i]; 
		
		//Adiciona tempo de espera para enviar a mensagem 
		Fila_MiniPC[Num_msg_fila_MiniPC].TempoEsperaEnvioMsg = _tempo_espera_envio_msg; //* copia valor
		
		//Adiciona flag de resposta do Mini Pc
		Fila_MiniPC[Num_msg_fila_MiniPC].EsperaRespostaMiniPc = _espera_resposta_mini_pc;
		
		if(_espera_resposta_mini_pc == true) {
			//Se tiver resposta prepara para enviar resposta
			
			//Adiciona tempo para esperar a resposta do Mini Pc
			Fila_MiniPC[Num_msg_fila_MiniPC].TempoEsperaRespostaMiniPc = _tempo_espera_resposta_mini_pc;
			//Adiciona o número de tentativas de envio da resposta para o Mini Pc
			Fila_MiniPC[Num_msg_fila_MiniPC].TentativaEnvioRespostaMiniPc = _tentativa_envio_resposta_mini_pc;
		}
		
		//Adiciona flag indicando que a placa controladora não enviou resposta para o Mini Pc
		Fila_MiniPC[Num_msg_fila_MiniPC].Flag_JaEnviouResposta = false;		
		
		//Contabiliza mensagens na fila para envio
		Num_msg_fila_MiniPC++;

	} else {
		//Fila cheia RETORNA ERRO
	}
}

/* Remove mensagem da fila da comunicação serial com o MiniPc
 * Parâmetros de entrada:
 * Parâmetro 1 - Número de mensagens a ser removida da fila
 *
 * Parâmetro de saída:
 * Não tem.
*/
void RemoveMsgFila_MiniPc(unsigned char c) {
	//Salva cópia de Num_msg_fil
	int i,x,
	    CopiaNum_msg_fila = Num_msg_fila_MiniPC, 
		i_controle_vezes = 0, y;

				
	//Verifica se tem mensagem na fila
	if(CopiaNum_msg_fila > 0) {
		//Tem mensagem na fila
						
		//Não permite retirar mais itens do que tem na fila
		if(c > CopiaNum_msg_fila) i_controle_vezes = CopiaNum_msg_fila;
		else i_controle_vezes = c;
			
		//Copia varias posições da fila			
		for(y = 0 ; y < i_controle_vezes ; y++) {				
		
			//Remove mensagem anterior da fila			
			for(i = 0 ; i < CopiaNum_msg_fila ; i++) {											
				//Copia tempo de espera para enviar mensagem anterior para atual
				Fila_MiniPC[i].TempoEsperaEnvioMsg = Fila_MiniPC[i+1].TempoEsperaEnvioMsg;
				//Limpa tempo de espera anterior
				Fila_MiniPC[i+1].TempoEsperaEnvioMsg = 0;					
															
				//Copia tamanho da mensagem anterior para a atual
				Fila_MiniPC[i].Tamanho_msg_bytes = Fila_MiniPC[i+1].Tamanho_msg_bytes;															
															
				//Limpa vetor com a mensagem atual para receber a mensagem anterior
				for(x = 0 ; x < Tamanho_BufferUSART0 ; x++) Fila_MiniPC[i].msg[x] = 0;
					
				//Copia mensagem anterior para atual
				//for(x = 0 ; x < VerificaTamanho(Fila_MiniPC[i+1].msg) ; x++) Fila_MiniPC[i].msg[x] = Fila_MiniPC[i+1].msg[x]; //0 <- 1		
				for(x = 0 ; x < Fila_MiniPC[i].Tamanho_msg_bytes ; x++) Fila_MiniPC[i].msg[x] = Fila_MiniPC[i+1].msg[x]; //0 <- 1		
					
				//Copia status da resposta do Mini Pc anterior para atual
				Fila_MiniPC[i].EsperaRespostaMiniPc = Fila_MiniPC[i+1].EsperaRespostaMiniPc;
				//Limpa resposta do MiniPc anterior
				Fila_MiniPC[i+1].EsperaRespostaMiniPc = false;
					
				//Copia tempo de espera para enviar resposta para o Mini Pc anterior para atual
				Fila_MiniPC[i].TempoEsperaRespostaMiniPc = Fila_MiniPC[i+1].TempoEsperaRespostaMiniPc;
				//Limpa tempo de espera para enviar resposta para o Mini Pc anterior 
				Fila_MiniPC[i+1].TempoEsperaRespostaMiniPc = 0;
					
				//Copia tentativas de envio de resposta para o Mini Pc anterior para atual
				Fila_MiniPC[i].TentativaEnvioRespostaMiniPc = Fila_MiniPC[i+1].TentativaEnvioRespostaMiniPc;
				//Limpa tentativas de envio de resposta para o Mini Pc anterior
				Fila_MiniPC[i+1].TentativaEnvioRespostaMiniPc = 0;
					
				//Copia status da resposta enviada ao Mini Pc anterior para atual
				Fila_MiniPC[i].Flag_JaEnviouResposta = Fila_MiniPC[i+1].Flag_JaEnviouResposta;
				//Limpa status da resposta enviada ao Mini Pc anterior
				Fila_MiniPC[i+1].Flag_JaEnviouResposta = false;								
			}				
		}
			
		//Atualiza contador de mensagens na fila
		Num_msg_fila_MiniPC-= i_controle_vezes;
	}	
}

unsigned int SelecionaRegistradorModbusDoControladorDeTemperatura(unsigned char Parametro_USART) {
	unsigned int Aux_Retorno = 0xFFFF;
	
	if(StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK == ControladorDeTemperatura_TC900Elog_Conectado) {
		if(Parametro_USART == Parametro_SetPoint_C_ControladorDeTemperatura) Aux_Retorno = TC900Elog_RegistradorSetpoint_ControladorDeTemperatura;
		else if(Parametro_USART == Parametro_Histerese_C_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorHisterese_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_SetPointMinimo_C_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorSetpointMinimo_ControladorDeTemperatura;	
		else if(Parametro_USART == Parametro_SetPointMaximo_C_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorSetpointMaximo_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_AlarmeTemperaturaAmbienteBaixa_C_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorAlarmeTemperaturaAmbienteBaixa_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_AlarmeTemperaturaAmbienteAlta_C_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorAlarmeTemperaturaAmbienteAlta_ControladorDeTemperatura;	
		else if(Parametro_USART == Parametro_IntervaloEntreDegelos_Minutos_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorIntervaloEntreDegelos_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_TempoDeDuracaoMaximaDoDegelo_Minutos_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorTempoDeDuracaoMaximaDoDegelo_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_TempoDeIndicacaoDeTemperaturaTravada_Minutos_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorTempoDeIndicacaoDeTemperaturaTravada_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_Buzzer_ControladorDeTemperatura) Aux_Retorno = TC900Elog_ResgistradorControlaBuzzer_ControladorDeTemperatura;
	} else if(StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK == ControladorDeTemperatura_MT512Elog_Conectado) {
		if(Parametro_USART == Parametro_SetPoint_C_ControladorDeTemperatura) Aux_Retorno = MT512Elog_RegistradorSetpoint_ControladorDeTemperatura;
		else if(Parametro_USART == Parametro_Histerese_C_ControladorDeTemperatura) Aux_Retorno = MT512Elog_ResgistradorHisterese_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_SetPointMinimo_C_ControladorDeTemperatura) Aux_Retorno = MT512Elog_ResgistradorSetpointMinimo_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_SetPointMaximo_C_ControladorDeTemperatura) Aux_Retorno = MT512Elog_ResgistradorSetpointMaximo_ControladorDeTemperatura;	
		else if(Parametro_USART == Parametro_AlarmeTemperaturaAmbienteBaixa_C_ControladorDeTemperatura) Aux_Retorno = 0xFFFF;		
		else if(Parametro_USART == Parametro_AlarmeTemperaturaAmbienteAlta_C_ControladorDeTemperatura) Aux_Retorno = 0xFFFF;		
		else if(Parametro_USART == Parametro_IntervaloEntreDegelos_Minutos_ControladorDeTemperatura) Aux_Retorno = MT512Elog_ResgistradorIntervaloEntreDegelos_ControladorDeTemperatura;
		else if(Parametro_USART == Parametro_TempoDeDuracaoMaximaDoDegelo_Minutos_ControladorDeTemperatura) Aux_Retorno = MT512Elog_ResgistradorTempoDeDuracaoMaximaDoDegelo_ControladorDeTemperatura;		
		else if(Parametro_USART == Parametro_TempoDeIndicacaoDeTemperaturaTravada_Minutos_ControladorDeTemperatura) Aux_Retorno = 0xFFFF;		
		else if(Parametro_USART == Parametro_Buzzer_ControladorDeTemperatura) Aux_Retorno =	0xFFFF;	
	}
	
	return(Aux_Retorno);
}
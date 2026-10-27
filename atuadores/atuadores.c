/*
 * atuadores.c
 *
 * Created: 19/12/2021 12:22:37
 *  Author: André
 */ 

#include "atuadores.h"

//Union com saídas do expansor
extern union Saida Saida_74HC595;
//Union com entradas do expansor
extern union Entrada	Entrada_74H165;

volatile bool CurtoAtuador; //0 Não tem curto 1 tem curto
volatile bool CurtoElevador; //0 Não tem curto 1 tem curto

extern volatile unsigned char SensoresPosicaoElevador_Validos[NumeroDeGavetas+1];

//Struct de status do auto teste
extern AT StatusAutoTeste;
//Guarda resultado da identificação dos atuadores
volatile unsigned char AtuadorIdentificado[NumeroMaximoDeAtuadores+1];

volatile unsigned char TipoDeAtuador[NumeroMaximoDeAtuadores+1];

volatile unsigned char UltimaPosicaoValidaDoElevador = 0;

MaquinaDeEstados MaquinaDeEstados_Atuadores;
MaquinaDeEstados Posiciona_Elevador;
MaquinaDeEstados Movimenta_Elevador;

volatile bool LigaDesliga_Elevador;

//Sensor de queda
volatile bool LigaDesliga_SensorDeQueda;
volatile bool Flag_Status_SensorDeQueda;

//Leitura dos fim de curso
extern volatile bool Flag_FimDeCurso;
//Leitura da trava das gavetas
extern volatile bool Flag_TravaDasGavetas;

extern volatile unsigned int Debounce_SensorDePosicaoElevador;
volatile unsigned char ContaBorda_SensorDePosicaoElevador = 0, TRAVA_CONTA_SENSOR = 0;

/* Inicializa maquina de estados para movimentar o elevador
 * Parâmetros de entrada:
 * Parâmetro 1 = Estado inicial da maquina de estados
 *
 * Parâmetro de saída:
 * Não tem
 */
void InicializaMaquinaDeEstados_MovimentaElevador(unsigned char EstadoInicial) {
	//Inicia no estado Atuador DESLIGADO
	Movimenta_Elevador.Estado = EstadoInicial;
	//Desliga temporizador
	Movimenta_Elevador.TempoProximoEstado = -1;
}

/* Inicializa maquina de estados para posicionar o elevador ao ligar a máquina
 * Parâmetros de entrada:
 * Parâmetro 1 = Estado inicial da maquina de estados
 *
 * Parâmetro de saída:
 * Não tem
 */
void InicializaMaquinaDeEstados_PosicionaElevador(unsigned char EstadoInicial) {
	//Inicia no estado Atuador DESLIGADO
	Posiciona_Elevador.Estado = EstadoInicial;
	//Desliga temporizador
	Posiciona_Elevador.TempoProximoEstado = -1;	
}

/* Inicializa maquina de estados para acionar atuadores (motor mola, gancheira ou esteira)
 * Parâmetros de entrada:
 * Parâmetro 1 = Estado inicial da maquina de estados
 *
 * Parâmetro de saída:
 * Não tem
 */
void InicializaMaquinaDeEstados_Atuadores(unsigned char EstadoInicial) {
	//Inicia no estado Atuador DESLIGADO
	MaquinaDeEstados_Atuadores.Estado = EstadoInicial;
	//Desliga temporizador
	MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
}

/* Aciona atuadores (motor mola, gancheira ou esteira)
 *
 * ATENÇÃO: ESTÁ FUNÇÃO SÓ FUNCIONA CORRETAMENTE DEPOIS DE REALIZADO O AUTO TESTE NA MÁQUINA
 *
 * Parâmetros de entrada:
 * Parâmetro 1 = Motor a ser acionado (1 a 72)
 *					
 * Parâmetro 2 = Tipo do atuador (motor mola, gancheira ou esteira)
 *
 * Parâmetro de saída:	0 = "Status_MA_AcionandoAtuador" Acionamento do atuador em andamento
						0x26 = "Status_MA_AcionamentoFinalizado" Finalizou o acionamento corretamente
						0x27 = "Status_MA_Erro_FusivelDosAtuadoresRompido" 
						0x28 = "Status_MA_Erro_AtuadorDesconectado"
						0x29 = "Status_MA_Erro_AtuadorEmCurto" Atuador em curto
						0x2A = "Status_MA_Erro_TimeoutEsteira" Esteira não identificou a entrega do produto
						0x2B = "Status_MA_Erro_SensorEsteiraAcionadoAntesDeIniciarMovimentacao"
						0x2C = "Status_MA_Erro_AtuadorForaDoRange"
*/
unsigned char AcionaAtuador(unsigned char NumeroAtuador, unsigned char TipoAtuador) {
	static unsigned char Coluna = 0, Linha = 0;
	unsigned char r = Status_MA_AcionandoAtuador,
				  Aux = 0;
	
	//Garante que atuador a ser acionado está entre 1 e 72
	if((NumeroAtuador >= NumeroMinimoDeAtuadores) && (NumeroAtuador <= NumeroMaximoDeAtuadores)) {
		
		switch(MaquinaDeEstados_Atuadores.Estado) {
			case MA_AtuadorDisponivel:
				//Desliga temporizador
				MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
				//Passa para próximo estado
				MaquinaDeEstados_Atuadores. Estado = MA_LigaFonte;
				
				//Garante matriz desligada
				Desliga_MatrizDosAtuadores();				
										
			break;

			case MA_LigaFonte:
				//Liga fonte +24V e espera 100ms
				MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoEsperaParaVerificarFusivel_ms; //100ms
				//Passa para próximo estado
				MaquinaDeEstados_Atuadores.Estado = MA_EsperaFonteLigar;
			
				//Conecta fonte de +24V nos atuadores
				Saida_74HC595.ID_ATUADORES = 0;
			
				//Seleciona atuador
				//Sendo 0 = Leitura da esteira (Abre +24V lendo sensor magnético)
				//      1 = Acionamento do Motor Mola ou Gancheira (Fecha +24V alimentando atuador)
			
				if(TipoAtuador == Esteira) Saida_74HC595.SEL_ATUADOR = 0;
				else if((TipoAtuador == Motor_Mola_Simples) || (TipoAtuador == Motor_Mola_Duplo) || (TipoAtuador == Gancheira_Mola) || (TipoAtuador == Gancheira)) Saida_74HC595.SEL_ATUADOR = 1;
			
				Escreve74HC595(Saida_74HC595.Expansor);
			
			break;
			
			case MA_EsperaFonteLigar:
				if(MaquinaDeEstados_Atuadores.TempoProximoEstado <= 0) {
					//Passa para próximo estado
					MaquinaDeEstados_Atuadores.Estado = MA_VerificaFusivel;
				}
			break;
			
			case MA_VerificaFusivel:
			//Verifica se fusível esta OK
			Entrada_74H165.Expansor = Le74HC165();
			
				if(Entrada_74H165.FUSIVEL_ATUADORES == 0) {
					//Fusível OK
				
					//Passa para próximo estado
					//Desliga temporizador
					MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
					//Passa para próximo estado
					MaquinaDeEstados_Atuadores.Estado = MA_VerificaAtuador;
				
				} else {
					//Fusível ABERTO/ROMPIDO
				
					//Atualiza status do fusível
					StatusAutoTeste.Fusivel_Atuadores_OK = false;
				
					//Retorna que fusível está rompido
					r = Status_MA_Erro_FusivelDosAtuadoresRompido;
				
					//Desliga temporizador
					MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
					MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;
				}
			
			break;
						
			case MA_VerificaAtuador:
				//Verifica se atuador está conectado, desconectado ou em curto
				
				Aux = AtuadorIdentificado[NumeroAtuador];

				if((Aux == Atuador_Conectado) || (Aux == Atuador_Em_Curto)) { //Aciona atuador em curto se ele for substituido funciona normalmente
					
					//Atuador devidamente conectado a máquina e o hardware de acionamento está OK 
					//senão não seria possível identificar o atuador

					//Desliga temporizador
					MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
					//Passa para próximo estado
					MaquinaDeEstados_Atuadores. Estado = MA_IdentificaLinhaEColuna;							
							
				} else {
					//Atuador desconectado ou em curto
					if(Aux == Atuador_Desconectado) {
						//Retorna ERRO se atuador a ser acionado está desconectado
						r = Status_MA_Erro_AtuadorDesconectado;						
					} 
									
					//Desliga temporizador
					MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;					
					MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;										
				}
	
			break;
						
			case MA_IdentificaLinhaEColuna:
				//Identifica linha
				Linha = (NumeroAtuador / NumeroAtuadoresNaGaveta) + 1;
				if((NumeroAtuador % NumeroAtuadoresNaGaveta) == 0) Linha--;
			
				//Identifica coluna
				Coluna = NumeroAtuador - ((Linha -1) * NumeroAtuadoresNaGaveta);
				
				if(TipoAtuador == Esteira) {				
					//Liga temporizador
					MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoEntreEnergizar_Esteira_E_MontorarSensorMagnetico; 
					//Passa para próximo estado
					MaquinaDeEstados_Atuadores.Estado = MA_VerificaSensorMagnetico_Esteira;		
#warning R215 alterado para 10K
#warning ADICIONADO 4 versao 1.34 110324
					Saida_74HC595.ID_ATUADORES = 1; //5V @20mA para não movimentar atuador
					
					//Liga 5V@20mA para verificar o sensor magnético
					ComandaHighSide(Linha, LigaAtuador);
					//Aterra atuador
					ComandaLowSide(Coluna, LigaAtuador);
					
					//Aciona atuador
					Escreve74HC595(Saida_74HC595.Expansor);			
/*								
					//Liga para verificar o sensor magnético
					ComandaHighSide(Linha, LigaAtuador);								
					//Aciona atuador
					Escreve74HC595(Saida_74HC595.Expansor);		
*/								
				} else {
					//Senão for ESTEIRA passa para próximo estado sem testar sensor magnético da esteira
					//Desliga temporizador
					MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
					//Passa para próximo estado
					MaquinaDeEstados_Atuadores.Estado = MA_PulsoInicial;
				}				
			break;
			
			case MA_VerificaSensorMagnetico_Esteira:
				
				if((TipoAtuador == Esteira) && (MaquinaDeEstados_Atuadores.TempoProximoEstado <= 0)) {
					//Lê sensor magnético da esteira somente depois que tempo zerar
					
					if(!(PortLeitura_LeituraEsteira & (1<<Pino_LeituraEsteira))) {
						//Se sensor acionado indica erro
					
						r = Status_MA_Erro_SensorEsteiraAcionadoAntesDeIniciarMovimentacao;
						
						//Desliga temporizador
						MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
						MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;		
								
					} else {
						//Sensor não acionado OK
/*						
						//Desliga temporizador
						MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
						//Passa para próximo estado
						MaquinaDeEstados_Atuadores.Estado = MA_PulsoInicial;	
*/		

#warning ADICIONADO 3 versao 1.34 110324
						MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoEntreEnergizar_Esteira_E_MontorarSensorMagnetico;
						//Passa para próximo estado
						MaquinaDeEstados_Atuadores.Estado = MA_PulsoInicial;																	
					}
					
					//Desliga acionamento da verificação do sensor da esteira
					
					Saida_74HC595.ID_ATUADORES = 0; //5V @20mA para não movimentar atuador
					
					//Liga 5V@20mA para verificar o sensor magnético
					ComandaHighSide(Linha, DesligaAtuador);
					//Aterra atuador
					ComandaLowSide(Coluna, DesligaAtuador);
					
					//Aciona atuador
					Escreve74HC595(Saida_74HC595.Expansor);										
				}		
			break;
			
			case MA_PulsoInicial:				
				//Motor mola
				if((TipoAtuador == Motor_Mola_Simples) || (TipoAtuador == Motor_Mola_Duplo) || (TipoAtuador == Gancheira_Mola)) {

					//Aciona coluna do atuador
					ComandaLowSide(Coluna,LigaAtuador);
					//Aciona Linha do atuador
					ComandaHighSide(Linha,LigaAtuador);
					//Aciona atuador
					Escreve74HC595(Saida_74HC595.Expansor);
					
					//Tempo para dar pulso inicial
					MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoPulsoInicial_Motor_Mola_ms;	//1s
					//Passa para próximo estado
					MaquinaDeEstados_Atuadores.Estado = EsperaPulsoInicial;						
			
				//Gancheira
				} else if(TipoAtuador == Gancheira) {
				
					//Aciona coluna do atuador
					ComandaLowSide(Coluna,LigaAtuador);
					//Aciona Linha do atuador
					ComandaHighSide(Linha,LigaAtuador);
					//Aciona atuador
					Escreve74HC595(Saida_74HC595.Expansor);
				
					//Tempo para dar pulso inicial
					MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoPulsoInicial_Gancheira_ms;	//0,2s
					//Passa para próximo estado
					MaquinaDeEstados_Atuadores.Estado = EsperaPulsoInicial;
				
				//Esteira
				} else if(TipoAtuador == Esteira) {
					//Aciona coluna do atuador
					ComandaLowSide(Coluna,LigaAtuador);
					//Aciona Linha do atuador
					ComandaHighSide(Linha,LigaAtuador);
					//Aciona atuador
					Escreve74HC595(Saida_74HC595.Expansor);
/*				
					//Tempo para dar pulso inicial
					MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoPulsoInicial_Esteira_ms;
					//Passa para próximo estado
					MaquinaDeEstados_Atuadores.Estado = EsperaPulsoInicial;
*/
				#warning ADICIONADO 2 delay para verificacao do sensor da esteira versao 1.34 110324
				//Tempo para Energizar a esteira
				MaquinaDeEstados_Atuadores.TempoProximoEstado = 100;
				//Passa para próximo estado
				MaquinaDeEstados_Atuadores.Estado = EsperaEnergizarEsteira;
				#warning ADICIONADO PARA TESTE					
				}
			break;

			#warning ADICIONADO 1 delay para verificacao do sensor da esteira versao 1.34 110324
			case EsperaEnergizarEsteira:
			if(MaquinaDeEstados_Atuadores.TempoProximoEstado <= 0) {
				//Tempo para dar pulso inicial
				MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoPulsoInicial_Esteira_ms;
				//Passa para próximo estado
				MaquinaDeEstados_Atuadores.Estado = EsperaPulsoInicial;
			}
			break;
			
			case EsperaPulsoInicial:
				if(TipoAtuador == Esteira) {
					//Lê sensor magnético da esteira

					if(!(PortLeitura_LeituraEsteira & (1<<Pino_LeituraEsteira))) {
						//Esteira identificou a entrega do produto 

						//Quando esteira identifica o produto finaliza o acionamento
						r = Status_MA_AcionamentoFinalizado;
						
						//Desliga temporizador
						MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
						MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;		
				
					} else if(MaquinaDeEstados_Atuadores.TempoProximoEstado <= 0) {
						//Deu timeout e a esteira não identificou a entrega do produto
					
						r = Status_MA_Erro_TimeoutEsteira;

						//Desliga temporizador
						MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
						MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;											
					}
				
				} else if((TipoAtuador == Motor_Mola_Simples) || (TipoAtuador == Motor_Mola_Duplo) || (TipoAtuador == Gancheira_Mola) || (TipoAtuador == Gancheira)) {
					//Se for motor mola OU gancheira espera terminar o tempo 
					if(MaquinaDeEstados_Atuadores.TempoProximoEstado <= 0) {
						//Terminou movimentação
					
						//Desliga temporizador
						MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
						MaquinaDeEstados_Atuadores.Estado = FinalizaPulsoInicial;						
					}
				}
				
				if(CurtoAtuador == true) {
					CurtoAtuador = false;
					//Curto no atuador
					
					r = Status_MA_Erro_AtuadorEmCurto;
					
					//Atualiza matriz dos atuadores 
					AtuadorIdentificado[NumeroAtuador] = Atuador_Em_Curto;
					
					//Desliga temporizador
					MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
					MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;
				}
			break;
			
			case FinalizaPulsoInicial:
				//Entra aqui apenas Motor mola e gancheira
							
				//Desliga Linha do atuador
				ComandaHighSide(Linha, DesligaAtuador);
				//Aciona atuador
				Escreve74HC595(Saida_74HC595.Expansor);
			
				if((TipoAtuador == Motor_Mola_Simples) || (TipoAtuador == Motor_Mola_Duplo) || (TipoAtuador == Gancheira_Mola)) {
					//Tempo para finalizar o giro
					MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoEsperaFinalizarGiro_Motor_Mola_ms;	//4s
					
				} else if(TipoAtuador == Gancheira) {
					//Tempo para finalizar o acionamento
					MaquinaDeEstados_Atuadores.TempoProximoEstado = TempoEsperaFinalizar_Gancheira_ms;	//1s
					
				} 
				//Passa para próximo estado
				MaquinaDeEstados_Atuadores.Estado = EsperaFinalizarGiro;
			break;
			
			case EsperaFinalizarGiro:
				if(MaquinaDeEstados_Atuadores.TempoProximoEstado <= 0) {
					//Esperou o tempo necessario finaliza acinamento
					MaquinaDeEstados_Atuadores.Estado = GiroFinalizado;
				}
				
				if(CurtoAtuador == true) {
					CurtoAtuador = false;
					//Curto no atuador
									
					r = Status_MA_Erro_AtuadorEmCurto;
					
					//Atualiza matriz dos atuadores
					AtuadorIdentificado[NumeroAtuador] = Atuador_Em_Curto;
									
					//Desliga temporizador
					MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
					MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;
				}
			break;
			
			case GiroFinalizado:
			
				r = Status_MA_AcionamentoFinalizado;
						
				//Desliga temporizador
				MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
				MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;
			
			break;
		}
		

	} else {
		//Atuador solicitado está fora do range 1 a 72
		r = Status_MA_Erro_AtuadorForaDoRange;
		
		//Desliga temporizador
		MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
		MaquinaDeEstados_Atuadores. Estado = MA_FinalizaAcionamento;		
	}
	
	if(MaquinaDeEstados_Atuadores.Estado == MA_FinalizaAcionamento) {
		//Desliga tensão para manter movimento motor mola e gancheira e seleciona leitura do sensor da esteira
		//Depois que acionou atuador remove tensão
		Saida_74HC595.SEL_ATUADOR = 0;
				
		Desliga_MatrizDosAtuadores();
				
		//Zera variáveis de identificação da linha e coluna
		Coluna = 0;
		Linha = 0;
				
		//Desliga temporizador
		MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;
		//Reinicia maquina de estados passa para primeiro estado
		MaquinaDeEstados_Atuadores.Estado = MA_AtuadorDisponivel;
	}
	
	return r;
}


/* Atualiza struct do high side (Gaveta) dos atuadores (motor mola, gancheira ou esteira)
 * ATENÇÃO: NÃO ATUALIZA AS SAÍDAS DO 74HC595 
 * Parâmetros de entrada:
 * Parâmetro 1 = Gaveta ou linha da matriz a ser acionada (1 a 6)
 * Parâmetro 2 =	0 para nivel lógico BAIXO e 
					1 para nível lógico ALTO
 *
 * Parâmetro de saída:
 * Não tem
 */
void ComandaHighSide(unsigned char Linha, unsigned char NivelLogico) {
	//Garante nível lógico em 0 ou 1
	if(NivelLogico >= 1) NivelLogico = 1;
	else NivelLogico = 0;
	
	//Garante Linha entre 1 e 6
	if(Linha <= 1) Linha = 1;
	if(Linha >= 6) Linha = 6;
	
	switch(Linha) {
		case 1:
			Saida_74HC595.MAIS_MOTOR_1 = NivelLogico;
		break;
					
		case 2:
			Saida_74HC595.MAIS_MOTOR_2 = NivelLogico;
		break;
					
		case 3:
			Saida_74HC595.MAIS_MOTOR_3 = NivelLogico;
		break;
					
		case 4:
			Saida_74HC595.MAIS_MOTOR_4 = NivelLogico;
		break;
					
		case 5:
			Saida_74HC595.MAIS_MOTOR_5 = NivelLogico;
		break;
					
		case 6:
			Saida_74HC595.MAIS_MOTOR_6 = NivelLogico;
		break;
	}	
}

/* Atualiza struct do low side Atuadores (motor mola, gancheira ou esteira)
 * ATENÇÃO: NÃO ATUALIZA AS SAÍDAS DO 74HC595 
 * Parâmetros de entrada:
 * Parâmetro 1 = Motor ou Coluna da matriz a ser acionada (1 a 12)
 * Parâmetro 2 =	0 para nivel lógico BAIXO e 
					1 para nível lógico ALTO
 *
 * Parâmetro de saída:
 * Não tem
 */
void ComandaLowSide(unsigned char Coluna, unsigned char NivelLogico) {

	//Garante nível lógico em 0 ou 1
	if(NivelLogico >= 1) NivelLogico = 1;
	else NivelLogico = 0;
	
	//Garante Coluna entre 1 e 12	
	if(Coluna <= 1) Coluna = 1;
	if(Coluna >= 12) Coluna = 12;
	
	switch(Coluna) {
		case 1:
			Saida_74HC595.MENOS_MOTOR_1 = NivelLogico;
		break;
						
		case 2:
			Saida_74HC595.MENOS_MOTOR_2 = NivelLogico;
		break;
						
		case 3:
			Saida_74HC595.MENOS_MOTOR_3 = NivelLogico;
		break;
					
		case 4:
			Saida_74HC595.MENOS_MOTOR_4 = NivelLogico;
		break;
						
		case 5:
			Saida_74HC595.MENOS_MOTOR_5 = NivelLogico;
		break;
						
		case 6:
			Saida_74HC595.MENOS_MOTOR_6 = NivelLogico;
		break;
					
		case 7:
			Saida_74HC595.MENOS_MOTOR_7 = NivelLogico;
		break;
						
		case 8:
			Saida_74HC595.MENOS_MOTOR_8 = NivelLogico;
		break;
						
		case 9:
			Saida_74HC595.MENOS_MOTOR_9 = NivelLogico;
		break;
					
		case 10:
			Saida_74HC595.MENOS_MOTOR_10 = NivelLogico;
		break;
						
		case 11:
			Saida_74HC595.MENOS_MOTOR_11 = NivelLogico;
		break;
						
		case 12:
			Saida_74HC595.MENOS_MOTOR_12 = NivelLogico;
		break;
	}	
}

/* Aciona trava magnética da porta principal 
 * Parâmetros de entrada:
 * Parâmetro 1 =	0 para ABRIR a trava magnética e 
					1 para FECHAR a trava magnética
 *
 * Parâmetro de saída:
 * Não tem
 */
void AcionaTravaMagnetica (unsigned char NivelLogico) {
	//Garante nível lógico em 0 ou 1
	if(NivelLogico >= 1) NivelLogico = 1;
	
	Saida_74HC595.TRAVA_MAGNETICA = NivelLogico;
	Escreve74HC595(Saida_74HC595.Expansor);	
}

/* Aciona solenoide da portinhola
 * Parâmetros de entrada:
 * Parâmetro 1 =	0 para DESLIGAR a solenoide e 
					1 para LIGAR a solenoide
 *
 * Parâmetro de saída:
 * Não tem
 */
void AcionaSolenoide (unsigned char NivelLogico) {
	//Garante nível lógico em 0 ou 1
	if(NivelLogico >= 1) NivelLogico = 1;
	
	Saida_74HC595.SOLENOIDE = NivelLogico;
	Escreve74HC595(Saida_74HC595.Expansor);
}

/* Aciona relé para aplicar +5V @ 20mA para identificação dos atuadores conectados
 * Parâmetros de entrada:
 * Parâmetro 1 =	0 aplica +24V e 
					1 aplica +5V @ 20mA
 *
 * Parâmetro de saída:
 * Não tem
 */
void AcionaIdentificacaoAtuadores (unsigned char NivelLogico) {
	//Garante nível lógico em 0 ou 1
	if(NivelLogico >= 1) NivelLogico = 1;
	else NivelLogico = 0;
	
	Saida_74HC595.ID_ATUADORES = NivelLogico;
	Escreve74HC595(Saida_74HC595.Expansor);
}

/* Seleciona entre tensão de alimentação para manter o movimento do motor mola e gancheira ou ler o sensor da esteira.
 * Parâmetros de entrada:
 * Parâmetro 1 =	0 Lê o sensor da esteira Tensão para manter o movimento DESLIGADA 
					1 Tensão para manter o movimento LIGADA 
 *
 * Parâmetro de saída:
 * Não tem
 */
void SelecionaAlimentacaoAtuadores (unsigned char NivelLogico) {
	//Garante nível lógico em 0 ou 1
	if(NivelLogico >= 1) NivelLogico = 1;
	else NivelLogico = 0;
	
	Saida_74HC595.SEL_ATUADOR = NivelLogico;
	Escreve74HC595(Saida_74HC595.Expansor);
}


/* Configura pino conectado ao BUZZER como saída e desliga o BUZZER
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void ConfiguraBUZZER() {
	//Configura pino do BUZZER como saída
	SET_BIT(Config_Port_BUZZER,Pino_BUZZER);
	//Garante BUZZER desligado
	CLEAR_BIT(Port_BUZZER,Pino_BUZZER);
}

/* Configura pinos conectados aos relés de acionamento do ELEVADOR como saída e garante ELEVADOR parado
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void ConfiguraElevador() {
	//Configura pinos do Elevador como saídas
	SET_BIT(Config_Port_Elevador,Pino_Sobe_Elevador);
	SET_BIT(Config_Port_Elevador, Pino_Desce_Elevador);
	//Garante Elevador parado
	CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
	CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
}

/* Configura pino de ENABLE dos CIs 74HC595
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void Configura_Pino_Enable_74HC595 (void) {
	//Configura Pino OE do 74HC595 
	SET_BIT(Config_Port_Enable_74HC595,Pino_Enable_74HC595);
	//Habilita a saída dos 74HC595 
	SET_BIT(Port_Enable_74HC595,Pino_Enable_74HC595);
}

/* Movimenta elevador
 * Parâmetros de entrada:
 * Parâmetro 1 = Posição que o elevador deve parar (1 a 7)
 *
 * Retorno = 0 Ainda não chegou na posição solicitada
 *           1 Chegou na posição soliciada
 *
 * Parâmetro de saída:	0 = Status_MM_AcionandoElevador	
 *						1 = Status_MM_AcionamentoFinalizado
 *						2 = Status_MM_SensorDePosicaoDoElevadorInvalido
 *						3 = Status_MM_PosicaoDoElevadorSolicitadaInvalida
 *						4 = Status_MM_SensorDeEntregaInvalido
 *						5 = Status_MM_SensorDeRepousoInvalido
 *						6 = Status_MM_TimeoutElevador
 *						7 = Status_MM_ElevadorEmCurto
 *						8 = Status_MM_ElevadorDesabilitado
 *						9 = Status_MM_TravaDasGavetasAberta
 *						10 = Status_MM_FimDeCurso
 *						11 = Status_MM_PortinholaAbertaOuFusivelRompido
 */
unsigned char MovimentaElevador(unsigned char p) {
	unsigned char P_Elevador = 0,	//Posição atual do elevador
				  r = Status_MM_AcionandoElevador;			//Retorno
	static unsigned char Copia_UltimaPosicaoValidaDoElevador, r_Aux;


	if(LigaDesliga_Elevador == true) {
		//Elevador habilitado
				
		switch(Movimenta_Elevador.Estado) {
			case MM_ElevadorDisponivel:
				
				//Inicia maquina de estados da movimentação
				Movimenta_Elevador.Estado = MM_ValidaPosicao_E_Hardware;
				Movimenta_Elevador.TempoProximoEstado = 0;	
				r_Aux = 0;
			break;
			
			case MM_ValidaPosicao_E_Hardware:
								
				Entrada_74H165.Expansor = Le74HC165();
				
				if(Entrada_74H165.TRAVA_GAVETAS == 0) {
					//Trava das gavetas FECHADA

					if(Entrada_74H165.PORTINHOLA == 0) {
						//Portinhola FECHADA

						//Sensor 6 repouso OK?
						if(StatusAutoTeste.SensorDeRepouso_OK == 1) {
							//Sensor de repouso OK

							//Sensor 7 entrega OK?
							if(StatusAutoTeste.SensorDeEntrega_OK == 1) {
								//Sensor de entrega OK		
																			
								//Verifica se posição solicitada é válida 
								if((p >= 1) && (p <= 7)) {					
									//Posição valida
						
									//Se sensor da posição solitada está OK
									if(SensoresPosicaoElevador_Validos[p-1] == 1) {	
										//Sensor da posição solicitada OK
														
										//Sensores do elevador necessários OK
										Movimenta_Elevador.Estado = MM_VerificaPosicao;
										Movimenta_Elevador.TempoProximoEstado = 0;
									} else {
										//Sensor da posição solicitada inválido
								
										r = Status_MM_SensorDePosicaoDoElevadorInvalido;
									
										Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
										Movimenta_Elevador.TempoProximoEstado = 0;										
							
									}
								} else {									
									//Se posição solicitada é inválida
									
									r = Status_MM_PosicaoDoElevadorSolicitadaInvalida;
						
									Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
									Movimenta_Elevador.TempoProximoEstado = 0;																					
								}
													
							} else {
								//Sensor de entrega inválido
								r = Status_MM_SensorDeEntregaInvalido;
										
								Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
								Movimenta_Elevador.TempoProximoEstado = 0;

							}
				
						
						} else {
							//Sensor de repouso inválido
							r = Status_MM_SensorDeRepousoInvalido;
							Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
							Movimenta_Elevador.TempoProximoEstado = 0;						
						}		
						
					} else {
						//Portinhola aberta ou fusivel rompido
						r = Status_MM_PortinholaAbertaOuFusivelRompido;
								
						Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
						Movimenta_Elevador.TempoProximoEstado = 0;						
					}
				} else {
					//Trava das gavetas aberta
					r = Status_MM_TravaDasGavetasAberta;
						
					Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
					Movimenta_Elevador.TempoProximoEstado = 0;						
				}
			break;

			case MM_VerificaPosicao:			
				//Atualiza posição atual do elevador
				P_Elevador = PosicaoAtualElevador();

				//Atualiza a última posição válida do elevador ANTES DE INICIAR A MOVIMENTAÇÃO
				Copia_UltimaPosicaoValidaDoElevador = UltimaPosicaoValidaDoElevador;

				//Verifica se elevador está na posição diferente da solicitada
				if(P_Elevador != p) {
					//Se posição atual do elevador diferente da posição desejada
				
					//Se elevador não está posicionado em sensor nem um 
					if(P_Elevador == 0) {

						//Se ultima posição válida != 0 usa ela
						if(UltimaPosicaoValidaDoElevador == 0) {
							r = Status_MM_SensorDePosicaoDoElevadorInvalido;
										
							Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
							Movimenta_Elevador.TempoProximoEstado = 0;	
						} else {
							//Ultima posição do sensor válida
//#warning USA ULTIMA POSICAO VALIDA PARA MOVIMENTAR ELEVADOR compra
							if(UltimaPosicaoValidaDoElevador > p) {
								//Sobe se posição atual do elevador MAIOR que posição solicitada
								Movimenta_Elevador.Estado = MM_Sobe;
								Movimenta_Elevador.TempoProximoEstado = 0;
							} else {
								//Desce se posição atual do elevador MENOR que posição solicitada						
								Movimenta_Elevador.Estado = MM_Desce;
								Movimenta_Elevador.TempoProximoEstado = 0;
							}
						}		

					} else {
						if(P_Elevador > p) {
							//Sobe se posição atual do elevador MAIOR que posição solicitada
							Movimenta_Elevador.Estado = MM_Sobe;
							Movimenta_Elevador.TempoProximoEstado = 0;
						} else {
							//Desce se posição atual do elevador MENOR que posição solicitada						
							Movimenta_Elevador.Estado = MM_Desce;
							Movimenta_Elevador.TempoProximoEstado = 0;
						}					
					}
				} else {
					//Se posição igual
					
					r = Status_MM_AcionamentoFinalizado;
					Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
					Movimenta_Elevador.TempoProximoEstado = 0;				
				}
			break;

			case MM_Sobe:
				//Sobe
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
				delayms(TempoEntreAcionamentoDoElevador_ms);
				SET_BIT(Port_Elevador, Pino_Sobe_Elevador);		
			
				Movimenta_Elevador.Estado = MM_Espera_Subir;
				Movimenta_Elevador.TempoProximoEstado = TimeOutMovimentaElevador_ms;
			
				//Garante que não houve curto no elevador anteriormente
				CurtoElevador = false;

				//Inicializa filtro para realizar debouncing do sensor de posição por +/- 440us e realizar a leitura de 2 bordas de descida
				Debounce_SensorDePosicaoElevador = 0;
				ContaBorda_SensorDePosicaoElevador = 0;
				TRAVA_CONTA_SENSOR = 0;
			break;
		
			case MM_Desce:
				//Desce
				CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
				delayms(TempoEntreAcionamentoDoElevador_ms);
				SET_BIT(Port_Elevador, Pino_Desce_Elevador);	
					
				Movimenta_Elevador.Estado = MM_Espera_Descer;
				Movimenta_Elevador.TempoProximoEstado = TimeOutMovimentaElevador_ms;	
				
				//Garante que não houve curto no elevador anteriormente
				CurtoElevador = false;

				//Inicializa filtro para realizar debouncing do sensor de posição por +/- 440us e realizar a leitura de 2 bordas de descida
				Debounce_SensorDePosicaoElevador = 0;
				ContaBorda_SensorDePosicaoElevador = 0;
				TRAVA_CONTA_SENSOR = 0;
			break;

			case MM_Espera_Subir:
				
				Entrada_74H165.Expansor = Le74HC165();
				if(Entrada_74H165.PORTINHOLA == 1) {
					Movimenta_Elevador.Estado = MM_Pausado_Portinhola_Subindo;
					break;
				}

				P_Elevador = PosicaoAtualElevador();					
												 					
				//Verifica se elevador chegou na posição solicitada
				if(P_Elevador == p) {

					if(TRAVA_CONTA_SENSOR == 0) {
						//Filtro para validação da posição
						//Conta +/- 440us em nível lógico BAIXO 

						if(Debounce_SensorDePosicaoElevador < DEBOUNCE_SENSORPOSICAOELEVADOR_CICLOS) {
							Debounce_SensorDePosicaoElevador++;							
						} else {
							//Detectou borda de descida
							TRAVA_CONTA_SENSOR = 1;
							ContaBorda_SensorDePosicaoElevador++;
						}
					}

					if(ContaBorda_SensorDePosicaoElevador >= NUMERO_BORDASDEDESCIDA_SENSORDEPOSICAODOELEVADOR) {
						//Detectou 2 bordas de descidas filtradas
						
						ContaBorda_SensorDePosicaoElevador = 0;
						Debounce_SensorDePosicaoElevador = 0;

						//Finaliza rotina

						//Desliga o elevador
						CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);					
						CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);					
						
						//Indica fim da movimentação OK
						r = Status_MM_AcionamentoFinalizado;
							
						Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
						Movimenta_Elevador.TempoProximoEstado = 0;
					}
				} else if((P_Elevador < p) && (P_Elevador !=0)) {
					//Elevador passou do sensor de posição na subida
					r_Aux = Status_MM_SensorDePosicaoDoElevadorInvalido;

					//Desliga o elevador
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					Movimenta_Elevador.Estado = MM_RetornaUltimaPosicaoValida;
					Movimenta_Elevador.TempoProximoEstado = 0;	

				} else if(Movimenta_Elevador.TempoProximoEstado <= 0) {
					//Deu time out	
					r_Aux = Status_MM_TimeoutElevador;

					//Desliga o elevador
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					Movimenta_Elevador.Estado = MM_RetornaUltimaPosicaoValida;
					Movimenta_Elevador.TempoProximoEstado = 0;		

				} else if(CurtoElevador == true) {
					//Curto no elevador
					CurtoElevador = false;

					r_Aux = Status_MM_ElevadorEmCurto;
					
					//Desliga o elevador
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					Movimenta_Elevador.Estado = MM_RetornaUltimaPosicaoValida;
					Movimenta_Elevador.TempoProximoEstado = 0;									
				} else if(Flag_FimDeCurso == true) {
					//Algum fim de curso acionado	
						
					r_Aux = Status_MM_FimDeCurso;

					//Desliga o elevador
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					//Atualiza a última posição válida do elevador
					Copia_UltimaPosicaoValidaDoElevador = UltimaPosicaoValidaDoElevador;

					Movimenta_Elevador.Estado = MM_DesceElevador_Com_FimDeCurso_Acionado;
					Movimenta_Elevador.TempoProximoEstado = 0;				
				} else {
					TRAVA_CONTA_SENSOR = 0;
					Debounce_SensorDePosicaoElevador = 0;
				}
				
			break;	

			case MM_Espera_Descer:
				
				Entrada_74H165.Expansor = Le74HC165();
				if(Entrada_74H165.PORTINHOLA == 1) {
					Movimenta_Elevador.Estado = MM_Pausado_Portinhola_Descendo;
					break;
				}

				P_Elevador = PosicaoAtualElevador();
										
				//Verifica se elevador chegou na posição solicitada
				if(P_Elevador == p) {					
					if(TRAVA_CONTA_SENSOR == 0) {
						//Filtro para validação da posição
						//Conta +/- 440us em nível lógico BAIXO 
						if(Debounce_SensorDePosicaoElevador < DEBOUNCE_SENSORPOSICAOELEVADOR_CICLOS) {
							Debounce_SensorDePosicaoElevador++;					
						} else {
							//Detectou borda de descida
							TRAVA_CONTA_SENSOR = 1;
							ContaBorda_SensorDePosicaoElevador++;
						}
					}

					if(ContaBorda_SensorDePosicaoElevador >= NUMERO_BORDASDEDESCIDA_SENSORDEPOSICAODOELEVADOR) {
						//Detectou 2 bordas de descidas filtradas
						
						ContaBorda_SensorDePosicaoElevador = 0;
						Debounce_SensorDePosicaoElevador = 0;

						//Finaliza rotina
						//Desliga o elevador
						CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
						CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

						//Indica fim da movimentação OK
						r = Status_MM_AcionamentoFinalizado;
							
						Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
						Movimenta_Elevador.TempoProximoEstado = 0;

					}
				} else if((P_Elevador > p) && (P_Elevador !=0)) {
					//Elevador passou do sensor de posição na subida
					r_Aux = Status_MM_SensorDePosicaoDoElevadorInvalido;

					//Desliga o elevador
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					Movimenta_Elevador.Estado = MM_RetornaUltimaPosicaoValida;
					Movimenta_Elevador.TempoProximoEstado = 0;	

				} else if(Movimenta_Elevador.TempoProximoEstado <= 0) {
					//Deu time out	
					r_Aux = Status_MM_TimeoutElevador;
					//Desliga o elevador
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					Movimenta_Elevador.Estado = MM_RetornaUltimaPosicaoValida;
					Movimenta_Elevador.TempoProximoEstado = 0;		
				} else if(CurtoElevador == true) {
					//Curto no elevador
					CurtoElevador = false;
				
					r_Aux = Status_MM_ElevadorEmCurto;

					//Desliga o elevador
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					Movimenta_Elevador.Estado = MM_RetornaUltimaPosicaoValida;
					Movimenta_Elevador.TempoProximoEstado = 0;					
				} else if(Flag_FimDeCurso == true) {
					//Algum fim de curso acionado	
						
					r_Aux = Status_MM_FimDeCurso;

					//Desliga o elevador
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					//Atualiza a última posição válida do elevador
					Copia_UltimaPosicaoValidaDoElevador = UltimaPosicaoValidaDoElevador;

					Movimenta_Elevador.Estado = MM_SobeElevador_Com_FimDeCurso_Acionado;
					Movimenta_Elevador.TempoProximoEstado = 0;				
				} else {
					Debounce_SensorDePosicaoElevador = 0;
					TRAVA_CONTA_SENSOR = 0;
				}
				
			break;	

			case MM_RetornaUltimaPosicaoValida:
//				Copia_UltimaPosicaoValidaDoElevador = UltimaPosicaoValidaDoElevador;
					
				//Se ultima posição válida do elevador é 0 retorna erro (NAO TEM MAIS O QUE FAZER)
				if(Copia_UltimaPosicaoValidaDoElevador == 0) {
					r = Status_MM_SensorDePosicaoDoElevadorInvalido;
										
					Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
					Movimenta_Elevador.TempoProximoEstado = 0;							
				} else {
					if(Copia_UltimaPosicaoValidaDoElevador < p) {
						//Sobe se posição atual do elevador MAIOR que posição solicitada
						Movimenta_Elevador.Estado = MM_Sobe_Curto;
						Movimenta_Elevador.TempoProximoEstado = 0;
					} else {
						//Desce se posição atual do elevador MENOR que posição solicitada						
						Movimenta_Elevador.Estado = MM_Desce_Curto;
						Movimenta_Elevador.TempoProximoEstado = 0;
					}					
				}				
			break;

			case MM_SobeElevador_Com_FimDeCurso_Acionado:
				//Sobe
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
				delayms(TempoEntreAcionamentoDoElevador_ms);
				SET_BIT(Port_Elevador, Pino_Sobe_Elevador);		
					
				Movimenta_Elevador.Estado = MM_EsperaElevador_SairDo_FimDeCurso;
				//Tempo para sair do fim de curso
				Movimenta_Elevador.TempoProximoEstado = TEMPO_ESPERA_PARA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_ms; 

				//Inicializa filtro para realizar debouncing do sensor de posição por +/- 440us e realizar a leitura de 2 bordas de descida
				Debounce_SensorDePosicaoElevador = 0;
				ContaBorda_SensorDePosicaoElevador = 0;
				TRAVA_CONTA_SENSOR = 0;				
			break;

			case MM_DesceElevador_Com_FimDeCurso_Acionado:
				//Desce
				CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
				delayms(TempoEntreAcionamentoDoElevador_ms);
				SET_BIT(Port_Elevador, Pino_Desce_Elevador);	
					
				Movimenta_Elevador.Estado = MM_EsperaElevador_SairDo_FimDeCurso;
				Movimenta_Elevador.TempoProximoEstado = TEMPO_ESPERA_PARA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_ms;
//#warning TEMPO PARA SAIR DO FIM DE CURSO
				//Inicializa filtro para realizar debouncing do sensor de posição por +/- 440us e realizar a leitura de 2 bordas de descida
				Debounce_SensorDePosicaoElevador = 0;
				ContaBorda_SensorDePosicaoElevador = 0;
				TRAVA_CONTA_SENSOR = 0;

			break;				

			case MM_EsperaElevador_SairDo_FimDeCurso:
				//Ou sai por tempo ou pelo fim de curso	
				if((Movimenta_Elevador.TempoProximoEstado == -1) || (Flag_FimDeCurso == false)) {	
					Movimenta_Elevador.Estado = MM_EsperaChegarNaUltimaPosicaoValida;
					Movimenta_Elevador.TempoProximoEstado = TimeOutMovimentaElevador_ms;		
				}
			break;				

			case MM_Sobe_Curto:
				//Sobe
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
				delayms(TempoEntreAcionamentoDoElevador_ms);
				SET_BIT(Port_Elevador, Pino_Sobe_Elevador);		
			
				Movimenta_Elevador.Estado = MM_EsperaChegarNaUltimaPosicaoValida;
				Movimenta_Elevador.TempoProximoEstado = TimeOutMovimentaElevador_ms; 
				
				//Garante que não houve curto no elevador anteriormente
				CurtoElevador = false;

				//Inicializa filtro para realizar debouncing do sensor de posição por +/- 440us e realizar a leitura de 2 bordas de descida
				Debounce_SensorDePosicaoElevador = 0;
				ContaBorda_SensorDePosicaoElevador = 0;
				TRAVA_CONTA_SENSOR = 0;			
			break;
	
			case MM_Desce_Curto:
				//Desce
				CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
				delayms(TempoEntreAcionamentoDoElevador_ms);
				SET_BIT(Port_Elevador, Pino_Desce_Elevador);	
					
				Movimenta_Elevador.Estado = MM_EsperaChegarNaUltimaPosicaoValida;
				Movimenta_Elevador.TempoProximoEstado = TimeOutMovimentaElevador_ms;	
				
				//Garante que não houve curto no elevador anteriormente
				CurtoElevador = false;

				//Inicializa filtro para realizar debouncing do sensor de posição por +/- 440us e realizar a leitura de 2 bordas de descida
				Debounce_SensorDePosicaoElevador = 0;
				ContaBorda_SensorDePosicaoElevador = 0;
				TRAVA_CONTA_SENSOR = 0;
			break;		

			case MM_EsperaChegarNaUltimaPosicaoValida:
				//Verifica a posição atual do elevador 
				P_Elevador = PosicaoAtualElevador();
									
				//Verifica se elevador chegou na posição solicitada
				if(P_Elevador == Copia_UltimaPosicaoValidaDoElevador) {

					if(TRAVA_CONTA_SENSOR == 0) {
						//Filtro para validação da posição
						//Conta +/- 440us em nível lógico BAIXO 
						if(Debounce_SensorDePosicaoElevador < DEBOUNCE_SENSORPOSICAOELEVADOR_CICLOS) {
							Debounce_SensorDePosicaoElevador++;					
						} else {
							//Detectou borda de descida							
							TRAVA_CONTA_SENSOR = 1;
							ContaBorda_SensorDePosicaoElevador++;
						}
					}

					if(ContaBorda_SensorDePosicaoElevador >= NUMERO_BORDASDEDESCIDA_SENSORDEPOSICAODOELEVADOR) {
						//Detectou 2 bordas de descidas filtradas

						ContaBorda_SensorDePosicaoElevador = 0;
						Debounce_SensorDePosicaoElevador = 0;							
				
						//Finaliza rotina

						//Desliga o elevador
						CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);					
						CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
										
						//Indica fim da movimentação OK				
						Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
						Movimenta_Elevador.TempoProximoEstado = 0;

					}
				} else if(Movimenta_Elevador.TempoProximoEstado <= 0) {
						//Deu time out	
						r_Aux = Status_MM_ElevadorEmCurto;
						Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
						Movimenta_Elevador.TempoProximoEstado = 0;					
				} else {
					Debounce_SensorDePosicaoElevador = 0;
					TRAVA_CONTA_SENSOR = 0;
				}
				
				//NAO VERIFICA CURTO PORQUE ESTÁ RETORNANDO PARA O ÚLTIMO SENSOR DE POSIÇÃO DO ELEVADOR VÁLIDO

				if(Flag_FimDeCurso == true) {
					//Algum fim de curso acionado	

					r_Aux = Status_MM_FimDeCurso;
					
					Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
					Movimenta_Elevador.TempoProximoEstado = 0;						
				}
					
			break;
			
			case MM_Pausado_Portinhola_Subindo:
				// Garante que o motor está desligado no software (para evitar faiscamento ao restaurar energia via fechamento da portinhola)
				CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

				Entrada_74H165.Expansor = Le74HC165();
				if(Entrada_74H165.PORTINHOLA == 0) {
					// Portinhola fechou, retoma o movimento de subida
					Movimenta_Elevador.Estado = MM_Sobe;
				}
			break;

			case MM_Pausado_Portinhola_Descendo:
				// Garante que o motor está desligado no software
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);

				Entrada_74H165.Expansor = Le74HC165();
				if(Entrada_74H165.PORTINHOLA == 0) {
					// Portinhola fechou, retoma o movimento de descida
					Movimenta_Elevador.Estado = MM_Desce;
				}
			break;						
		}
	} else {
		//Elevador desligado
		
		//Indica que elevador está desabilitado
		r = Status_MM_ElevadorDesabilitado;
		
		Movimenta_Elevador.Estado = MM_FinalizaAcionamento;
		Movimenta_Elevador.TempoProximoEstado = 0;		
	}
	
	if(Movimenta_Elevador.Estado == MM_FinalizaAcionamento) {
		//Desliga o elevador
		CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
		delayms(TempoEntreAcionamentoDesligarElevador_ms);
		CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
					
		InicializaMaquinaDeEstados_MovimentaElevador(MM_ElevadorDisponivel);

		//Retorna erro caso tenha curto, timeout ou chegue no fim de curso,
		
		if(r_Aux != 0) {
			r = r_Aux;
			r_Aux = 0;
		}
	}
	return r;	
}

/* Verifica em qual gaveta a atuador passado por parâmetro está
 * Parâmetro de entrada:
 * Parâmetro 1 - Atuador que deseja saber qual gaveta pertence
 * 
 * Parâmetro de saída:
 * Varia de 1 a 6.
 */
unsigned char Verifica_GavetaDoAtuador(unsigned char atuador) {
	unsigned char _g = 0;
	
	_g = (atuador / NumeroAtuadoresNaGaveta) + 1;
	if((atuador % NumeroAtuadoresNaGaveta) == 0) _g--;
	return (_g);
}

/* Desliga a matriz de acionamento dos atuadores
 * Parâmetro de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void Desliga_MatrizDosAtuadores(void) {
	//Desliga toda matriz
	Saida_74HC595.MAIS_MOTOR_1 = 0;
	Saida_74HC595.MAIS_MOTOR_2 = 0;
	Saida_74HC595.MAIS_MOTOR_3 = 0;
	Saida_74HC595.MAIS_MOTOR_4 = 0;
	Saida_74HC595.MAIS_MOTOR_5 = 0;
	Saida_74HC595.MAIS_MOTOR_6 = 0;
							
	Saida_74HC595.MENOS_MOTOR_1 = 0;
	Saida_74HC595.MENOS_MOTOR_2 = 0;
	Saida_74HC595.MENOS_MOTOR_3 = 0;
	Saida_74HC595.MENOS_MOTOR_4 = 0;
	Saida_74HC595.MENOS_MOTOR_5 = 0;
	Saida_74HC595.MENOS_MOTOR_6 = 0;
	Saida_74HC595.MENOS_MOTOR_7 = 0;
	Saida_74HC595.MENOS_MOTOR_8 = 0;
	Saida_74HC595.MENOS_MOTOR_9 = 0;
	Saida_74HC595.MENOS_MOTOR_10 = 0;
	Saida_74HC595.MENOS_MOTOR_11 = 0;
	Saida_74HC595.MENOS_MOTOR_12 = 0;
	Escreve74HC595(Saida_74HC595.Expansor);
}
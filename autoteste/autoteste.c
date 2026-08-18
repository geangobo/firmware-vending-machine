/*
 * autoteste.c
 *
 * Created: 31/05/2022 20:01:25
 *  Author: André
 */ 

#include "autoteste.h"

//Union com saídas do expansor
extern union Saida Saida_74HC595;
//Union com entradas do expansor
extern union Entrada	Entrada_74H165;

//Leitura do ADC
extern volatile unsigned int	Canal0Media,	//I_Elevador
						Canal3Media,	//V_Atuadores
						Canal7Media;	//I_Atuadores

extern volatile int	NumeroAmostrasADCAtuadores_Corrente,		//Número de leituras do ADC para identificar a CORRENTE do atuador
					NumeroAmostrasADCAtuadores_Tensao,			//Número de leituras do ADC para identificar a TENSÃO do atuador
					ValorPicoMedioAtuadores_ADC_Corrente,		//Valor de pico médio da CORRENTE para identificar atuador
					ValorPicoMedioAtuadores_ADC_Tensao;			//Valor de pico médio da TENSÃO para identificar atuador				

extern volatile int ContadorIdentificarCurto_Elevador;
extern volatile bool CurtoElevador;
extern volatile unsigned char UltimaPosicaoValidaDoElevador;

extern volatile unsigned char SensoresPosicaoElevador_Validos[NumeroDeGavetas+1];						

//Guarda resultado da identificação dos atuadores
extern volatile unsigned char AtuadorIdentificado[NumeroMaximoDeAtuadores+1];

//Maquinas de estados do auto teste
MaquinaDeEstados	MaquinaDeEstados_AutoTeste;
volatile unsigned char	TestaFusivelAtuadores_Estados = M_TFA_LIGA_24V_NOS_ATUADORES;
volatile unsigned char	TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_LIGA_24V_E_DESLIGA_MATRIZ;
volatile unsigned char	TestaSensorDeTensaoAtuadores_Estados = M_TSTA_LIGA_24V_NOS_ATUADORES;
volatile unsigned char	Identifica_Atuadores_Estados = M_TA_LIGA_5V_NOS_ATUADORES;
volatile unsigned char	TestaFusivelElevador_Estados = M_TFE_INICIA_TESTE_FUSIVEL_DO_ELEVADOR;
volatile unsigned char  TestaTravaDasGavetas_Estados = M_TTG_INICIA_TESTE_TRAVA_DAS_GAVETAS;
volatile unsigned char  TestaElevador_Estados = M_TE_SOBE_ELEVADOR;
volatile unsigned char	TestaComunicacaoControladorDeTemperatura_Estados = M_TCT_TC900Elog_EnviaMensagem;

extern volatile unsigned char Estado_Comunicacao_ControladorDeTemperatura;

//Define struct de status do auto teste
AT	StatusAutoTeste;

extern volatile bool LigaDesliga_Elevador;

volatile unsigned char EnviaResultadoDoAutotesteAoFinalizar = 0;

extern volatile unsigned char ControlaComunicacao_Controlador_de_temperatura;

//Union auxiliar para manipular bytes
union {
	unsigned char bytes[4];
	unsigned int u_int;
	unsigned long u_long_int;
	float f;
}ManipulaBytes;

extern volatile unsigned char msg[Tamanho_BufferUSART0];

extern volatile unsigned char IdentificaComando_ControladorDeTemperatura;

//Leitura dos fim de curso
extern volatile bool Flag_FimDeCurso;

/* Realiza Autoteste
 * Parâmetro de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * 0 sinaliza Status_AT_AutoTesteFinalizado
 * 1 sinaliza Status_AT_SubTesteFinalizado			
 * 2 sinaliza Status_AT_RealizandoAutoTeste
 */
unsigned char RealizaAutoteste(void) {
	unsigned char 	r = Status_AT_RealizandoAutoTeste;
	
	switch(MaquinaDeEstados_AutoTeste.Estado) {
		case M_AT_TestaFusivel_Atuadores:
			r = TestaFusivel_Atuadores();
			if(r != Status_AT_RealizandoAutoTeste) {
				//Finalizou o subteste
				
				//Passa para próximo estado
				MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaSensorDeCorrente_Atuadores;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
				//Indica que ainda está realizando o autoteste
				r = Status_AT_RealizandoAutoTeste;
			}
		break;
		
		case M_AT_TestaSensorDeCorrente_Atuadores:
			r = TestaSensorDeCorrente_Atuadores();
			if(r != Status_AT_RealizandoAutoTeste) {
				//Finalizou o subteste
			
				//Passa para próximo estado
				MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaSensorDeTensao_Atuadores;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
				//Indica que ainda está realizando o autoteste
				r = Status_AT_RealizandoAutoTeste;
			}
		break;		
		
		case M_AT_TestaSensorDeTensao_Atuadores:
			r = TestaSensorDeTensao_Atuadores();
			if(r != Status_AT_RealizandoAutoTeste) {
				//Finalizou o subteste
			
				//Passa para próximo estado
				MaquinaDeEstados_AutoTeste.Estado = M_AT_Identifica_Atuadores;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
				//Indica que ainda está realizando o autoteste
				r = Status_AT_RealizandoAutoTeste;
			}
		break;		
		
		case M_AT_Identifica_Atuadores:
			if((StatusAutoTeste.Fusivel_Atuadores_OK) && (StatusAutoTeste.SensorDeCorrente_Atuadores_OK) && (StatusAutoTeste.SensorDeTensao_Atuadores_OK)) {
				//Só identifica os atuadores caso fusível e sensores de corrente e tensão dos atuadores OK
				r = Identifica_Atuadores();
				if(r != Status_AT_RealizandoAutoTeste) {
					//Finalizou o subteste
			
					if(LigaDesliga_Elevador == true) {
						//Elevador ligado 
						
						//Passa para próximo estado
						MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaFusivel_Elevador;
						MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;						
					} else {
						//Elevador desligado 
						
						//Passa para próximo estado
						MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaSensorDeTemperatura;
						MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;						
					}			

					//Indica que ainda está realizando o autoteste
					r = Status_AT_RealizandoAutoTeste;
				}
			} else {
				//Erro, vai testar o elevador
				
					if(LigaDesliga_Elevador == true) {
						//Elevador ligado
						
						//Passa para próximo estado
						MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaFusivel_Elevador;
						MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
					} else {
						//Elevador desligado
						
						//Passa para próximo estado
						MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaSensorDeTemperatura;
						MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
					}

					//Indica que ainda está realizando o autoteste
					r = Status_AT_RealizandoAutoTeste;		
			}
		break;	
		
		case M_AT_TestaFusivel_Elevador:
			r = TestaFusivel_Elevador();
			if(r != Status_AT_RealizandoAutoTeste) {
				//Finalizou o subteste
			
				//Passa para próximo estado
				MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaTrava_Gavetas;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
				//Indica que ainda está realizando o autoteste
				r = Status_AT_RealizandoAutoTeste;
			}
		break;	
		
		case M_AT_TestaTrava_Gavetas:
			r = TestaTrava_Gavetas();
			if(r != Status_AT_RealizandoAutoTeste) {
				//Finalizou o subteste
					
				if((StatusAutoTeste.Fusivel_Elevador_OK) && (StatusAutoTeste.TravaDasGavetas_OK)) {
					//Se fusível e trava do elevador OK testa elevador
					
					UltimaPosicaoValidaDoElevador = 0;
					//Garante sensores de posição inválidos
					SensoresPosicaoElevador_Validos[0] = 0;
					SensoresPosicaoElevador_Validos[1] = 0;
					SensoresPosicaoElevador_Validos[2] = 0;
					SensoresPosicaoElevador_Validos[3] = 0;
					SensoresPosicaoElevador_Validos[4] = 0;
					SensoresPosicaoElevador_Validos[5] = 0;
					SensoresPosicaoElevador_Validos[6] = 0;
					
					//Passa para próximo estado
					MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaElevador;
					MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
					
					//Inicializa maquina de estados do teste do elevador
					TestaElevador_Estados = M_TE_TESTA_FIM_DE_CURSO;
					
					//Indica que ainda está realizando o autoteste
					r = Status_AT_RealizandoAutoTeste;					
				} else {
					//Não testa elevador 
					
					//Testa fim de curso
					Entrada_74H165.Expansor = Le74HC165();
			
					if((Entrada_74H165.FIM_CURSO_INFERIOR == 1) || (Entrada_74H165.FIM_CURSO_SUPERIOR == 1) || (Entrada_74H165.FIM_CURSO_RESERVA == 1)) {
						//Algum fim de curso acionado indica erro no fim de curso
				
						//Indica erro
						StatusAutoTeste.FimDeCurso_OK = false;				
					} else {
						//Indica fim de curso OK
						StatusAutoTeste.FimDeCurso_OK = true;				
					}	
																			
					//Sinaliza erro nos sensores de posição do elevador
					StatusAutoTeste.SensoresDePosicao_OK = false;
					StatusAutoTeste.SensorDeEntrega_OK = false;
					StatusAutoTeste.SensorDeRepouso_OK = false;

					UltimaPosicaoValidaDoElevador = 0;
					//Garante sensores de posição inválidos
					SensoresPosicaoElevador_Validos[0] = 0;
					SensoresPosicaoElevador_Validos[1] = 0;
					SensoresPosicaoElevador_Validos[2] = 0;
					SensoresPosicaoElevador_Validos[3] = 0;
					SensoresPosicaoElevador_Validos[4] = 0;
					SensoresPosicaoElevador_Validos[5] = 0;
					SensoresPosicaoElevador_Validos[6] = 0;
								
					//Reinicia máquina de estados do auto teste
					MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaSensorDeTemperatura;
					MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
					
					//Indica que ainda está realizando o autoteste
					r = Status_AT_RealizandoAutoTeste;			
				}
			}
		break;	
		
		case M_AT_TestaElevador:
		r = TestaElevador();
		if(r != Status_AT_RealizandoAutoTeste) {
			//Finalizou o subteste
				
			
			//Reinicia máquina de estados do auto teste
			MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaSensorDeTemperatura;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			//Indica que ainda está realizando o autoteste
			r = Status_AT_RealizandoAutoTeste;
		}
		break;	
		
		case M_AT_TestaSensorDeTemperatura:
			
			if(CalculaTemperatura() < (TemperaturaMinima_C+5.0)) {
				//Temperatura atual < -35.0°C sensor está desconectado
				StatusAutoTeste.SensorDeTemperatura_OK = false;
			} else {
				//Temperatura > -35.0°C sensor conectado
				StatusAutoTeste.SensorDeTemperatura_OK = true;
			}
			
			//Retorna máquina de estados do auto teste para inicio
			MaquinaDeEstados_AutoTeste.Estado = M_AT_ComunicacaoControladorDeTemperatura;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			
			//Inicia maquina de estados do teste da comunicação com o controlador de temperatura TC900E-Log
			TestaComunicacaoControladorDeTemperatura_Estados = M_TCT_TC900Elog_EnviaMensagem;
			
			//Inicia maquina de estados da comunicação com o controlador de temperatura
			InicializaMaquinaDeEstados_ControladorDeTemperatura(Estado_CT_Inicia_MODBUS);
			
			//Indica que ainda está realizando o autoteste
			r = Status_AT_RealizandoAutoTeste;
						
		break;	
		
		case M_AT_ComunicacaoControladorDeTemperatura:
			r = TestaComunicacao_ControladorDeTemperatura();
			if(r != Status_AT_RealizandoAutoTeste) {
				
				//Retorna máquina de estados do auto teste para inicio
				MaquinaDeEstados_AutoTeste.Estado = M_AT_TestaFusivel_Atuadores;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
				//Indica que ainda está realizando o autoteste
				r = Status_AT_AutoTesteFinalizado;				
			}	
		break;
	}
	
	return r;
}

/* Testa comunicação com o controlador de temperatura Full Gauge
 * Parâmetro de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * 1 sinaliza Status_AT_SubTesteFinalizado
 * 2 sinaliza Status_AT_RealizandoAutoTeste
 */
unsigned char TestaComunicacao_ControladorDeTemperatura(void) {
	unsigned char r = Status_AT_RealizandoAutoTeste;
	
	switch(TestaComunicacaoControladorDeTemperatura_Estados) {
		case M_TCT_TC900Elog_EnviaMensagem:
			IdentificaComando_ControladorDeTemperatura = TC900Elog_ValidaComunicacaoCom_ControladorDeTemperatura;
			
			StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK = ControladorDeTemperatura_Desconectado;
						
			//Setpoint
			LimpaVetor(msg,Tamanho_BufferUSART0);
			msg[0] = Modbus_Endereco_ControladorDeTemperatura;
			msg[1] = Modbus_LeituraUnicoRegistrador;
			ManipulaBytes.u_int = TC900Elog_RegistradorID_Fabricante_ControladorDeTemperatura;
			msg[2] = ManipulaBytes.bytes[1];
			msg[3] = ManipulaBytes.bytes[0];
			msg[4] = 0x00;
			msg[5] = 0x01;
			//Calcula CRC16 MODBUS
			ManipulaBytes.u_int = Calcula_CRC16_MODBUS(msg,6);
			msg[6] = ManipulaBytes.bytes[0];	//MSB
			msg[7] = ManipulaBytes.bytes[1];	//LSB
			AdicionaMsgFila_ControladorDeTemperatura(msg, TempoEsperaParaEnviarPacote_ControladorDeTemperatura_ms, 8, TempoEsperaParaReceberResposta_ControladorDeTemperatura_ms, NumeroDeTentativas_EnvioComando_ControladorDeTemperatura);
			
			TestaComunicacaoControladorDeTemperatura_Estados = M_TCT_TC900Elog_EsperaResposta;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TempoEsperaRepostaNoAutoteste_ControladorDeTemperatura_ms; //2s		
															
		break;
		
		case M_TCT_TC900Elog_EsperaResposta:

			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Não recebeu resposta do TC-900Elog envia mensagem para MT-512Elog

				//Configura para próximo envio
				TestaComunicacaoControladorDeTemperatura_Estados = M_TCT_MT512Elog_EnviaMensagem;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
				
			//Deve se comunicar com controlador de temperatura?
			if(ControlaComunicacao_Controlador_de_temperatura == 1) {
				Comunicacao_ControladorDeTemperatura();
					
			} else {
				//Aqui finalizou a comunicação com TC-900Elog	

				if(StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK == ControladorDeTemperatura_TC900Elog_Conectado) {				
					//Retorna que finalizou o subteste
					r = Status_AT_SubTesteFinalizado;
									
					//Configura para próximo envio
					TestaComunicacaoControladorDeTemperatura_Estados = M_TCT_TC900Elog_EnviaMensagem;
					MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
				}							
			}			

		break;
		
		case M_TCT_MT512Elog_EnviaMensagem:									
			IdentificaComando_ControladorDeTemperatura = MT512Elog_ValidaComunicacaoCom_ControladorDeTemperatura;
			
			//Setpoint
			LimpaVetor(msg,Tamanho_BufferUSART0);
			msg[0] = Modbus_Endereco_ControladorDeTemperatura;
			msg[1] = Modbus_LeituraUnicoRegistrador;
			ManipulaBytes.u_int = MT512ELog_RegistradorID_Fabricante_ControladorDeTemperatura;
			msg[2] = ManipulaBytes.bytes[1];
			msg[3] = ManipulaBytes.bytes[0];
			msg[4] = 0x00;
			msg[5] = 0x01;
			//Calcula CRC16 MODBUS
			ManipulaBytes.u_int = Calcula_CRC16_MODBUS(msg,6);
			msg[6] = ManipulaBytes.bytes[0];	//MSB
			msg[7] = ManipulaBytes.bytes[1];	//LSB
			AdicionaMsgFila_ControladorDeTemperatura(msg, TempoEsperaParaEnviarPacote_ControladorDeTemperatura_ms, 8, TempoEsperaParaReceberResposta_ControladorDeTemperatura_ms, NumeroDeTentativas_EnvioComando_ControladorDeTemperatura);
			
			TestaComunicacaoControladorDeTemperatura_Estados = M_TCT_MT512Elog_EsperaResposta;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TempoEsperaRepostaNoAutoteste_ControladorDeTemperatura_ms; //2s		
		break;
		
		case M_TCT_MT512Elog_EsperaResposta:

			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Não recebeu resposta

				//Retorna que finalizou o subteste
				r = Status_AT_SubTesteFinalizado;
			
				//Configura para próximo envio
				TestaComunicacaoControladorDeTemperatura_Estados = M_TCT_TC900Elog_EnviaMensagem;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
		
			//Deve se comunicar com controlador de temperatura?
			if(ControlaComunicacao_Controlador_de_temperatura == 1) {
				Comunicacao_ControladorDeTemperatura();
			} else {
				//Aqui finalizou a comunicação 
			
				if(StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK == ControladorDeTemperatura_MT512Elog_Conectado) {				
					//Retorna que finalizou o subteste
					r = Status_AT_SubTesteFinalizado;	
					
					//Configura para próximo envio
					TestaComunicacaoControladorDeTemperatura_Estados = M_TCT_TC900Elog_EnviaMensagem;
					MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;									
				} 
			}

		break;		
	}
	return (r);
}

/* Teste fusível dos atuadores F3 
 * Parâmetros de entrada:
 * Parâmetro 1 = não tem
 *
 * Parâmetro de saída:
 *					0x01 = Subteste finalizado
 *					0x02 = Realizando autoteste
*/
unsigned char TestaFusivel_Atuadores(void) {
	static unsigned char n_leituras_ok, Contadorleituras;
	unsigned char r = Status_AT_RealizandoAutoTeste;
	
	switch(TestaFusivelAtuadores_Estados) {
		case M_TFA_LIGA_24V_NOS_ATUADORES:
			//Limpa contador de leituras corretas (OK)
			n_leituras_ok = 0;
			//Numero de leituras do fusível
			Contadorleituras = 0;
		
			//Conecta fonte de +24V nos atuadores
			Saida_74HC595.ID_ATUADORES = 0;
			Escreve74HC595(Saida_74HC595.Expansor);
		
			//Espera relé bater e tensão estabilizar
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_RELE_BATER_ms; //100ms
			TestaFusivelAtuadores_Estados = M_TFA_ESPERA_LIGAR_24V_NOS_ATUADORES;
		break;
		
		case M_TFA_ESPERA_LIGAR_24V_NOS_ATUADORES:
		case M_TFA_ESPERA_PARA_TESTAR_FUSIVEL_DOS_ATUADORES:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Acabou de contar o tempo
							
				TestaFusivelAtuadores_Estados = M_TFA_TESTA_FUSIVEL_DOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
		break;
		
		case M_TFA_TESTA_FUSIVEL_DOS_ATUADORES:		
			//Lê fusível
			Entrada_74H165.Expansor = Le74HC165();
			//Conta número de leituras do Fusível OK
			if(!Entrada_74H165.FUSIVEL_ATUADORES) n_leituras_ok++;
		
			if(++Contadorleituras == NUMERO_DE_LEITURAS_DO_FUSIVEL_DOS_ATUADORES) {
				//Terminou as leituras
			
				//REINICIA A MAQUINA DE ESTADOS DO TESTE DO FUSIVEL DOS ATUADORES
				TestaFusivelAtuadores_Estados = M_TFA_LIGA_24V_NOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			
				//Retorna que finalizou o subteste
				r = Status_AT_SubTesteFinalizado;
				
				//Verifica se todas leituras foram corretas e atualiza struct com os resultados do autoteste
				if(n_leituras_ok == NUMERO_DE_LEITURAS_DO_FUSIVEL_DOS_ATUADORES) StatusAutoTeste.Fusivel_Atuadores_OK = true; //OK
				else StatusAutoTeste.Fusivel_Atuadores_OK = false; //ERRO
			} else {
				//Ainda não terminou
			
				//Espera para realizar a próxima leitura
				TestaFusivelAtuadores_Estados = M_TFA_ESPERA_PARA_TESTAR_FUSIVEL_DOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = INTERVALO_DE_ESPERA_ENTRE_LEITURAS_DO_FUSIVEL_DOS_ATUADORES_ms; //10ms
			}
		break;
	}	
	return r;
}

/* Teste sensor de corrente dos atuadores 
 * Parâmetros de entrada:
 * Parâmetro 1 = não tem
 *
 * Parâmetro de saída:
 *					0 - Erro no sensor de corrente dos atuadores
 *					1 - sensor de corrente dos atuadores OK 
*/
unsigned char TestaSensorDeCorrente_Atuadores(void) {
	static unsigned char n_leituras_ok, Contadorleituras;
	unsigned char r = Status_AT_RealizandoAutoTeste;
static unsigned int CorrenteMatrizDesligada[NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_CORRENTE_DOS_ATUADORES],
CorrenteMatrizLigada[NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_CORRENTE_DOS_ATUADORES];	
		
    switch(TestaSensorDeCorrenteAtuadores_Estados) {
	    case M_TSCA_LIGA_24V_E_DESLIGA_MATRIZ:
			//Inicializa vetor que guarda leitura da corrente
			for(unsigned char i = 0 ; i < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_CORRENTE_DOS_ATUADORES ; i++) {
				CorrenteMatrizDesligada[i] = 0;
				CorrenteMatrizLigada[i] = 0;
			}

			//Numero de leituras
			n_leituras_ok = 0;
			Contadorleituras = 0;
	    
			//Conecta fonte de +24V nos atuadores
			Saida_74HC595.ID_ATUADORES = 0;
			//Garante matriz desligada
			Desliga_MatrizDosAtuadores();
	    
			//Espera relé bater e tensão estabilizar
			//Garante matriz desligada
			TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_ESPERA_LIGA_24V_E_DESLIGA_MATRIZ;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_RELE_BATER_ms; //100ms
	    break;

	    case M_TSCA_ESPERA_LIGA_24V_E_DESLIGA_MATRIZ:
	    case M_TSCA_ESPERA_LE_CORRENTE_COM_MATRIZ_DESLIGADA:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) { 
				//Acabou de contar o tempo
		    
				//Volta a testar o fusivel dos atuadores
				TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_LE_CORRENTE_COM_MATRIZ_DESLIGADA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
	    break;

	    case M_TSCA_LE_CORRENTE_COM_MATRIZ_DESLIGADA:
			if(Contadorleituras < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_CORRENTE_DOS_ATUADORES) {
				//Ainda não leu todas as amostras necessárias
				CorrenteMatrizDesligada[Contadorleituras] = Canal7Media;
				Contadorleituras++;

				//Espera próximo momento de ler a corrente
				TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_ESPERA_LE_CORRENTE_COM_MATRIZ_DESLIGADA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = 10; //10ms
			} else {
				//Já leu todas amostras

				Contadorleituras = 0;

				//Liga high side da matriz
				TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_LIGA_HIGH_SIDE_DA_MATRIZ;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
	    break;

	    case M_TSCA_LIGA_HIGH_SIDE_DA_MATRIZ:
			//Liga matriz High Side (Usa leds como carga)
			Saida_74HC595.MAIS_MOTOR_1 = 1;
			Saida_74HC595.MAIS_MOTOR_2 = 1;
			Saida_74HC595.MAIS_MOTOR_3 = 1;
			Saida_74HC595.MAIS_MOTOR_4 = 1;
			Saida_74HC595.MAIS_MOTOR_5 = 1;
			Saida_74HC595.MAIS_MOTOR_6 = 1;
			Escreve74HC595(Saida_74HC595.Expansor);

			//Espera high side da matriz ligar
			TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_ESPERA_LIGA_HIGH_SIDE_DA_MATRIZ;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = 5;  //5ms
	    break;

	    case M_TSCA_ESPERA_LIGA_HIGH_SIDE_DA_MATRIZ:
	    case M_TSCA_ESPERA_LE_CORRENTE_COM_MATRIZ_LIGADA:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Acabou de contar o tempo

				//Volta a ler a corrente
				TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_LE_CORRENTE_COM_MATRIZ_LIGADA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
	    break;

	    case M_TSCA_LE_CORRENTE_COM_MATRIZ_LIGADA:
			if(Contadorleituras < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_CORRENTE_DOS_ATUADORES) {
				//Ainda não leu todas as amostras necessárias
				CorrenteMatrizLigada[Contadorleituras] = Canal7Media;
				Contadorleituras++;

				//Espera próximo momento de ler a corrente
				TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_ESPERA_LE_CORRENTE_COM_MATRIZ_LIGADA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = 10; //10ms
			} else {
				//Já leu todas amostras

				//Desliga High Side
				Saida_74HC595.MAIS_MOTOR_1 = 0;
				Saida_74HC595.MAIS_MOTOR_2 = 0;
				Saida_74HC595.MAIS_MOTOR_3 = 0;
				Saida_74HC595.MAIS_MOTOR_4 = 0;
				Saida_74HC595.MAIS_MOTOR_5 = 0;
				Saida_74HC595.MAIS_MOTOR_6 = 0;
				Escreve74HC595(Saida_74HC595.Expansor);

				//REINICIA MAQUINA DE ESTADOS DO TESTE DO SENSOR DE CORRENTE DOS ATUADORES
				TestaSensorDeCorrenteAtuadores_Estados = M_TSCA_LIGA_24V_E_DESLIGA_MATRIZ;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;

				//Retorna que finalizou o subteste
				r = Status_AT_SubTesteFinalizado;

				//Verifica se sensor de corrente está OK
				n_leituras_ok = 0;
				for(unsigned char i = 0 ; i < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_CORRENTE_DOS_ATUADORES ; i++) {
					if(CorrenteMatrizLigada[i] - CorrenteMatrizDesligada[i] >= VALOR_DO_ADC_SENSOR_DE_CORRENTE_ATUADORES_OK) n_leituras_ok++;
				}

				if(n_leituras_ok == NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_CORRENTE_DOS_ATUADORES) StatusAutoTeste.SensorDeCorrente_Atuadores_OK = true; //OK
				else StatusAutoTeste.SensorDeCorrente_Atuadores_OK = false; //ERRO
			}
	    break;
    }

    return r;	

}

/* Teste sensor de tensão dos atuadores 
 * Parâmetros de entrada:
 * Parâmetro 1 = não tem
 *
 * Parâmetro de saída:
 *					0 - Erro no sensor de tensão dos atuadores
 *					1 - sensor de tensão dos atuadores OK 
*/
unsigned char TestaSensorDeTensao_Atuadores(void) {
	static unsigned char n_leituras_ok, Contadorleituras;
    unsigned char 	r = Status_AT_RealizandoAutoTeste;
    static unsigned int Leitura_ADC_24V[NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES],
    Leitura_ADC_5V[NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES];

    switch(TestaSensorDeTensaoAtuadores_Estados) {
	    case M_TSTA_LIGA_24V_NOS_ATUADORES:
			//Inicializa vetor que guarda leitura da tensão
			for(unsigned char i = 0 ; i < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES ; i++) {
				Leitura_ADC_24V[i] = 0;
				Leitura_ADC_5V[i] = 0;
			}

			//Numero de leituras
			n_leituras_ok = 0;
			Contadorleituras = 0;
	    
			//Conecta fonte de +24V nos atuadores
			Saida_74HC595.ID_ATUADORES = 0;
			Escreve74HC595(Saida_74HC595.Expansor);

			//Espera relé bater e tensão estabilizar
			TestaSensorDeTensaoAtuadores_Estados = M_TSTA_ESPERA_LIGAR_24V_NOS_ATUADORES;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_RELE_BATER_ms; //100ms
	    break;

	    case M_TSTA_ESPERA_LIGAR_24V_NOS_ATUADORES:
	    case M_TSTA_ESPERA_LER_TENSAO_24V_NOS_ATUADORES:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Acabou de contar o tempo

				//Vai ler o 24V
				TestaSensorDeTensaoAtuadores_Estados = M_TSTA_LE_TENSAO_24V_NOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
	    break;

	    case M_TSTA_LE_TENSAO_24V_NOS_ATUADORES:		
			if(Contadorleituras < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES) {
				//Ainda não leu todas as amostras necessárias
				Leitura_ADC_24V[Contadorleituras] = Canal3Media;
				Contadorleituras++;

				//Espera próximo momento de ler a corrente
				TestaSensorDeTensaoAtuadores_Estados = M_TSTA_ESPERA_LER_TENSAO_24V_NOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = 10; //10ms
			} else {
				//Já leu todas amostras

				//Verifica se leitura do 24V está OK
				for(unsigned char i = 0 ; i < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES ; i++) {
					if((Leitura_ADC_24V[i] > 677) && (Leitura_ADC_24V[i] < 1024)) n_leituras_ok++;				
				}	    

				if(n_leituras_ok == NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES) {
					//Leitura do +24V OK

					//Lê o +5V
					TestaSensorDeTensaoAtuadores_Estados = M_TSTA_LIGA_5V_NOS_ATUADORES;
					MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
				} else {
					//ERRO na leitura do +24V

					//Reinicia maquina de estado
					TestaSensorDeTensaoAtuadores_Estados = M_TSTA_LIGA_24V_NOS_ATUADORES;
					MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;

					//Retorna que subteste foi finalizado
					r = Status_AT_SubTesteFinalizado;
					StatusAutoTeste.SensorDeTensao_Atuadores_OK = false; //ERRO
				}
				
				//Reinicia para próxima leitura
				Contadorleituras = 0;
				n_leituras_ok = 0;				
			}
	    break;

	    case M_TSTA_LIGA_5V_NOS_ATUADORES:
			//Conecta fonte de +5V nos atuadores
			Saida_74HC595.ID_ATUADORES = 1;
			Escreve74HC595(Saida_74HC595.Expansor);

			//Espera relé bater e tensão estabilizar
			TestaSensorDeTensaoAtuadores_Estados = M_TSTA_ESPERA_LIGAR_5V_NOS_ATUADORES;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_RELE_BATER_ms; //100ms
	    break;

	    case M_TSTA_ESPERA_LIGAR_5V_NOS_ATUADORES:
	    case M_TSTA_ESPERA_LER_TENSAO_5V_NOS_ATUADORES:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Acabou de contar o tempo

				//Vai ler o 5V
				TestaSensorDeTensaoAtuadores_Estados = M_TSTA_LE_TENSAO_5V_NOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
	    break;

	    case M_TSTA_LE_TENSAO_5V_NOS_ATUADORES:
		
			if(Contadorleituras < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES) {
				//Ainda não leu todas as amostras necessárias
				Leitura_ADC_5V[Contadorleituras] = Canal3Media;
				Contadorleituras++;

				//Espera próximo momento de ler a corrente
				TestaSensorDeTensaoAtuadores_Estados = M_TSTA_ESPERA_LER_TENSAO_5V_NOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = 10; //10ms
			} else {
				//Já leu todas amostras

				//Verifica se leitura do 24V está OK
				for(unsigned char i = 0 ; i < NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES ; i++) {
					if((Leitura_ADC_5V[i] > ValorADCMinimo_ValidaFonteDeCorrente_5V) && (Leitura_ADC_5V[i] < ValorADCMaximo_ValidaFonteDeCorrente_5V)) n_leituras_ok++;
				}

				//Reinicia maquina de estado
				TestaSensorDeTensaoAtuadores_Estados = M_TSTA_LIGA_24V_NOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;

				//Retorna indicando que finalizou o subteste
				r = Status_AT_SubTesteFinalizado;
				if(n_leituras_ok == NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES) StatusAutoTeste.SensorDeTensao_Atuadores_OK = true; //OK
				else StatusAutoTeste.SensorDeTensao_Atuadores_OK = false; //ERRO
				
				Contadorleituras = 0;
				n_leituras_ok = 0;
				
				//Conecta fonte de +24V nos atuadores
				Saida_74HC595.ID_ATUADORES = 0;
				Escreve74HC595(Saida_74HC595.Expansor);				
			}
		break;
	}
	return r;	
}

/* Verifica e identifica cada um dos 72 atuadores se estão conectados ou desconectados a máquina.
 * Liga fonte de corrente de 5V @ 20mA nos atuadores e realiza a leitura da corrente para verificar se atuador está ou não conectado.
 * 
 * A variável AtuadoresIdentificados armazena o "STATUS" de cada um dos 72 atuadores
 *
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 *					0 - Rotina em execução (RETORNO_REALIZANDO_AUTOTESTE)
 *					1 - Subteste finalizado (RETORNO_SUBTESTE_FINALIZADO_OK)
 */
unsigned char Identifica_Atuadores(void) {
	unsigned char 	r = Status_AT_RealizandoAutoTeste;
	static unsigned char Gaveta_atual, Atuador_atual;
	unsigned char StatusAtuador = Atuador_Desconectado;
	
	switch(Identifica_Atuadores_Estados) {
		case M_TA_LIGA_5V_NOS_ATUADORES:
			//Inicializa teste na primeira gaveta e primeiro atuador
			Gaveta_atual = 0;
			Atuador_atual = 0;			
			
			//Seleciona fonte de corrente +5V @ 20mA
			AcionaIdentificacaoAtuadores (Aplica_5V_20mA_Nos_Atuadores);
			
			//Desliga tensão que mantém movimento do motor mola e gancheira
			SelecionaAlimentacaoAtuadores(Ler_Sensor_Da_Esteira);

			//Espera relé bater e tensão estabilizar
			Identifica_Atuadores_Estados = M_TA_ESPERA_LIGAR_5V_NOS_ATUADORES;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_RELE_BATER_ms; //100ms	
				
		break;
		
		case M_TA_ESPERA_LIGAR_5V_NOS_ATUADORES:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Acabou de contar o tempo

				//Seleciona a gaveta a ser testada
				Identifica_Atuadores_Estados = M_TA_SELECIONA_GAVETA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1; 				
			}
		break;
		
		case M_TA_SELECIONA_GAVETA:
			//Verifica se já testou das gavetas
			if(++Gaveta_atual <= NumeroDeGavetas) {						
				
				//Desliga matriz
				Saida_74HC595.MAIS_MOTOR_1 = 0;
				Saida_74HC595.MAIS_MOTOR_2 = 0;
				Saida_74HC595.MAIS_MOTOR_3 = 0;
				Saida_74HC595.MAIS_MOTOR_4 = 0;
				Saida_74HC595.MAIS_MOTOR_5 = 0;
				Saida_74HC595.MAIS_MOTOR_6 = 0;
				
				//Energiza toda gaveta
				ComandaHighSide(Gaveta_atual, 1); 
				Escreve74HC595(Saida_74HC595.Expansor);
							
				Identifica_Atuadores_Estados = M_TA_ESPERA_SELECIONAR_GAVETA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = INTERVALO_DE_ESPERA_PARA_SELECIONAR_A_GAVETA_ms; //5ms
			} else {
				//Já leu todas as gavetas 

				//Desliga matriz
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
				
				//Conecta fonte de +24V nos atuadores
				AcionaIdentificacaoAtuadores (Aplica_24V_Atuadores);
															
				r = Status_AT_SubTesteFinalizado;
				
				//Reinicia maquina de estados
				Identifica_Atuadores_Estados = M_TA_LIGA_5V_NOS_ATUADORES;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1; 
			}	

		break;
		
		case M_TA_ESPERA_SELECIONAR_GAVETA:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Acabou de contar o tempo

				Atuador_atual = 0;
				//Seleciona atuador a ser testado
				Identifica_Atuadores_Estados = M_TA_SELECIONA_ATUADOR_DA_GAVETA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}		
		break;
		
		case M_TA_SELECIONA_ATUADOR_DA_GAVETA:
			
			if(++Atuador_atual <= NumeroAtuadoresNaGaveta) {				

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
								
				//Seleciona atuador							
				ComandaLowSide(Atuador_atual, 1);
				Escreve74HC595(Saida_74HC595.Expansor);
							
				//Le a tensão e corrente do atuador
				Identifica_Atuadores_Estados = M_TA_ESPERA_SELECIONAR_ATUADOR_DA_GAVETA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = 1;

			} else {
				//Terminou a identificação do atuador passa para próxima gaveta
				
				//Desliga matriz
				Desliga_MatrizDosAtuadores();
								
				//Já leu todos atuadores da gaveta
				//Seleciona a gaveta a ser testada
				Identifica_Atuadores_Estados = M_TA_SELECIONA_GAVETA;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;				
			}
			
		break;
		
		case M_TA_ESPERA_SELECIONAR_ATUADOR_DA_GAVETA:
						
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Acabou de contar o tempo

				//Inicia leitura da corrente e tensão na interrupção do ADC
				NumeroAmostrasADCAtuadores_Tensao = 1;
				NumeroAmostrasADCAtuadores_Corrente = 1;
				
				//Seleciona atuador a ser testado
				Identifica_Atuadores_Estados = M_TA_LE_TENSAO_E_CORRENTE_DO_ATUADOR;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;	
			}
		break;
		
		case M_TA_LE_TENSAO_E_CORRENTE_DO_ATUADOR:
			//Espera finalizar as leituras da tensão e corrente

			if((NumeroAmostrasADCAtuadores_Corrente >= 10) && (NumeroAmostrasADCAtuadores_Tensao >= 10)) {
				
				//Pausa leituras
				NumeroAmostrasADCAtuadores_Tensao = 0;
				NumeroAmostrasADCAtuadores_Corrente = 0;
				
				Identifica_Atuadores_Estados = M_TA_IDENTIFICA_ATUADOR;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;	
			}			
		break;
		
		case M_TA_IDENTIFICA_ATUADOR:		
			//Corrente sem atuador 0,004A (Corrente dos leds do HIGH SIDE) = 0,048V no ADC ~13
			//Corrente com atuador 0,016A = 0,192V no ADC ~54
				
			if(ValorPicoMedioAtuadores_ADC_Corrente >= VALOR_DO_ADC_ATUADOR_CONECTADO) {
				StatusAtuador = Atuador_Conectado;					
			} else {
				StatusAtuador = Atuador_Desconectado;
			}
			//Zera leitura do ADC para identificar o próximo atuador
			ValorPicoMedioAtuadores_ADC_Corrente = 0;
								
			//Atualiza status dos atuadores
			AtuadorIdentificado[(Atuador_atual + ((Gaveta_atual-1)*NumeroAtuadoresNaGaveta))] = StatusAtuador;

			Identifica_Atuadores_Estados = M_TA_SELECIONA_ATUADOR_DA_GAVETA;
				
		break;
	}	
	return r;
}

/* Teste fusível do elevador
 * Parâmetros de entrada:
 * Parâmetro 1 = não tem
 *
 * Parâmetro de saída:
 *					0 - Rotina em execução (RETORNO_REALIZANDO_AUTOTESTE)
 *					1 - Subteste finalizado (RETORNO_SUBTESTE_FINALIZADO_OK)
*/
unsigned char TestaFusivel_Elevador(void) {
	static unsigned char n_leituras_ok, Contadorleituras;
	unsigned char 	r = Status_AT_RealizandoAutoTeste;
	
	switch(TestaFusivelElevador_Estados) {
		case M_TFE_INICIA_TESTE_FUSIVEL_DO_ELEVADOR:
			//Limpa contador de leituras corretas (OK)
			n_leituras_ok = 0;
			//Numero de leituras do fusível
			Contadorleituras = 0;
		
			//Inicia teste do fusível do elevador
			TestaFusivelElevador_Estados = M_TFE_TESTA_FUSIVEL_DO_ELEVADOR;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_RELE_BATER_ms; //100ms
		break;
		
		case M_TFE_ESPERA_PARA_TESTAR_FUSIVEL_DO_ELEVADOR:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				//Acabou de contar o tempo
			
				TestaFusivelElevador_Estados = M_TFE_TESTA_FUSIVEL_DO_ELEVADOR;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
		break;
		
		case M_TFE_TESTA_FUSIVEL_DO_ELEVADOR:		
		
			//Lê fusível
			Entrada_74H165.Expansor = Le74HC165();
			//Conta número de leituras do Fusível OK
			if(!Entrada_74H165.PORTINHOLA) n_leituras_ok++;
		
			if(++Contadorleituras == NUMERO_DE_LEITURAS_DO_FUSIVEL_DO_ELEVADOR) {
				//Terminou as leituras
			
				//REINICIA A MAQUINA DE ESTADOS DO TESTE DO FUSIVEL DOS ATUADORES
				TestaFusivelElevador_Estados = M_TFE_INICIA_TESTE_FUSIVEL_DO_ELEVADOR;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			
				//Indica que finalizou o subteste
				r = Status_AT_SubTesteFinalizado;

				//Verifica se todas leituras foram corretas
				if(n_leituras_ok == NUMERO_DE_LEITURAS_DO_FUSIVEL_DO_ELEVADOR) StatusAutoTeste.Fusivel_Elevador_OK = true; //OK
				else StatusAutoTeste.Fusivel_Elevador_OK = false; //ERRO
			} else {
				//Ainda não terminou
			
				//Espera para realizar a próxima leitura
				TestaFusivelElevador_Estados = M_TFE_ESPERA_PARA_TESTAR_FUSIVEL_DO_ELEVADOR;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = INTERVALO_DE_ESPERA_ENTRE_LEITURAS_DO_FUSIVEL_DO_ELEVADOR_ms; //10ms
			}
		break;
	}
	
	return r;
}


/* Verifica a trava das gavetas
 * Parâmetros de entrada:
 * Parâmetro 1 = não tem
 *
 * Parâmetro de saída:
 *					0 - Rotina em execução (RETORNO_REALIZANDO_AUTOTESTE)
 *					1 - Subteste finalizado (RETORNO_SUBTESTE_FINALIZADO_OK)
*/
unsigned char TestaTrava_Gavetas(void) {
	static unsigned char n_leituras_ok, Contadorleituras;
	unsigned char 	r = Status_AT_RealizandoAutoTeste;
	
	switch(TestaTravaDasGavetas_Estados) {
		case M_TTG_INICIA_TESTE_TRAVA_DAS_GAVETAS:
		
			//Limpa contador de leituras corretas (OK)
			n_leituras_ok = 0;
			//Numero de leituras do fusível
			Contadorleituras = 0;
		
			//Inicia teste do fusível do elevador
			TestaTravaDasGavetas_Estados = M_TTG_TESTA_TRAVA_DAS_GAVETAS;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_RELE_BATER_ms; //100ms				
		break;
		
		case M_TTG_ESPERA_PARA_TESTAR_TRAVA_DAS_GAVETAS :
		if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
			//Acabou de contar o tempo
			
			TestaTravaDasGavetas_Estados = M_TTG_TESTA_TRAVA_DAS_GAVETAS;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
		}
		break;
		
		case M_TTG_TESTA_TRAVA_DAS_GAVETAS:
			
			Entrada_74H165.Expansor = Le74HC165();
			//Conta número de leituras da trava das gavetas OK
			if(!Entrada_74H165.TRAVA_GAVETAS) n_leituras_ok++;
		
			if(++Contadorleituras == NUMERO_DE_LEITURAS_DA_TRAVA_DAS_GAVETAS) {
				//Terminou as leituras
			
				//REINICIA A MAQUINA DE ESTADOS DO TESTE DA TRAVA DAS GAVETAS
				TestaTravaDasGavetas_Estados = M_TTG_INICIA_TESTE_TRAVA_DAS_GAVETAS;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			
				r = Status_AT_SubTesteFinalizado;
				
				//Verifica se todas leituras foram corretas
				if(n_leituras_ok == NUMERO_DE_LEITURAS_DA_TRAVA_DAS_GAVETAS) StatusAutoTeste.TravaDasGavetas_OK = true; //OK
				else StatusAutoTeste.TravaDasGavetas_OK = false; //ERRO
				
				n_leituras_ok = 0;
				Contadorleituras = 0;
			} else {
				//Ainda não terminou
			
				//Espera para realizar a próxima leitura
				TestaTravaDasGavetas_Estados = M_TTG_ESPERA_PARA_TESTAR_TRAVA_DAS_GAVETAS;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = INTERVALO_DE_ESPERA_ENTRE_LEITURAS_DA_TRAVA_DAS_GAVETAS_ms; //10ms
			}
		break;
	}
	
	return r;
}

/* Testa elevador
 * Parâmetros de entrada:
 * Não tem.
 * Parâmetro de saída:
 *					0 - Rotina em execução (RETORNO_REALIZANDO_AUTOTESTE)
 *					1 - Subteste finalizado (RETORNO_SUBTESTE_FINALIZADO_OK)
 */
unsigned char TestaElevador(void) {
    unsigned char 	r = Status_AT_RealizandoAutoTeste;
	static volatile char Copia_UltimaPosicaoValidaDoElevador;	

    switch(TestaElevador_Estados) {
        case M_TE_TESTA_FIM_DE_CURSO:
			Entrada_74H165.Expansor = Le74HC165();
			
			//Lê fim de curso sem filtro
			if((Entrada_74H165.FIM_CURSO_INFERIOR == 1) || (Entrada_74H165.FIM_CURSO_SUPERIOR == 1) || (Entrada_74H165.FIM_CURSO_RESERVA == 1)) {
				//Algum fim de curso acionado indica erro no fim de curso				
				//Indica erro
				StatusAutoTeste.FimDeCurso_OK = false;
				
				//Tenta seguir com o teste do elevador
				TestaElevador_Estados = M_TE_SOBE_ELEVADOR_FIM_DE_CURSO_ACIONADO;
			} else {
				//Indica fim de curso OK
				StatusAutoTeste.FimDeCurso_OK = true;
				
				TestaElevador_Estados = M_TE_SOBE_ELEVADOR;
			}			
			
		break;

		case M_TE_SOBE_ELEVADOR_FIM_DE_CURSO_ACIONADO:
			//Sobe
			CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
			delayms(TempoEntreAcionamentoDoElevador_ms);
			SET_BIT(Port_Elevador, Pino_Sobe_Elevador);		
			
			TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_SUBINDO;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_PARA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_ms; 
		break;

		case M_TE_SOBE_ELEVADOR:		
			//Sobe
			CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
			delayms(TempoEntreAcionamentoDoElevador_ms);
			SET_BIT(Port_Elevador, Pino_Sobe_Elevador);		
			
			TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_SUBIR;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TimeOutMovimentaElevador_ms;		
        break;

		case M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_SUBINDO:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_SUBIR;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = TimeOutMovimentaElevador_ms;		
			}
		break;

        case M_TE_ESPERA_ELEVADOR_SUBIR:

            Entrada_74H165.Expansor = Le74HC165();
			
            if(!Entrada_74H165.ELEVADOR_P1) {								 				
			    //Elevador chegou na primeira gaveta 
				
			    TestaElevador_Estados = M_TE_DESCE_ELEVADOR;
			    MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;				
			}       
			
			if((MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) || (CurtoElevador == true)) {
				//Elevador deu timeout ou curto

				ContadorIdentificarCurto_Elevador = TempoADC_IdentificaCurto_Elevador;
				CurtoElevador = false;

				//Desliga o elevador
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
				delayms(TempoEntreAcionamentoDesligarElevador_ms);
				CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
				
				//Indica erro
				StatusAutoTeste.SensoresDePosicao_OK = false; //ERRO
				StatusAutoTeste.SensorDeEntrega_OK = false;
				StatusAutoTeste.SensorDeRepouso_OK = false;
				
				//Desce e tenta terminar o teste
			    TestaElevador_Estados = M_TE_DESCE_ELEVADOR;
			    MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;						
			}
			
			//Se fim de OK e durante o teste aciona um fim de curso indica erro
			if(Flag_FimDeCurso == 1) {
				//Elevador chegou em algum fim de curso					

				//Desliga o elevador
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
				delayms(TempoEntreAcionamentoDesligarElevador_ms);
				CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

				//Indica erro
				StatusAutoTeste.FimDeCurso_OK = false;
				
				//Desce e tenta terminar o teste
				TestaElevador_Estados = M_TE_DESCE_ELEVADOR_FIM_DE_CURSO_ACIONADO;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;			
			}
        break;

        case M_TE_DESCE_ELEVADOR_FIM_DE_CURSO_ACIONADO:
			//Desce
			CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
			delayms(TempoEntreAcionamentoDoElevador_ms);
			SET_BIT(Port_Elevador, Pino_Desce_Elevador);				

			TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_DESCENDO;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_PARA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_ms;		
        break; 

		case M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_DESCENDO:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) {
				TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_DESCER;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = TimeOutMovimentaElevador_ms;		
			}
		break;

        case M_TE_DESCE_ELEVADOR:
			//Desce
			CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
			delayms(TempoEntreAcionamentoDoElevador_ms);
			SET_BIT(Port_Elevador, Pino_Desce_Elevador);				

			TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_DESCER;
			MaquinaDeEstados_AutoTeste.TempoProximoEstado = TimeOutMovimentaElevador_ms;		
        break; 

        case M_TE_ESPERA_ELEVADOR_DESCER:			
			
			Entrada_74H165.Expansor = Le74HC165();

			if((MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) || (CurtoElevador == true)) {
				//Elevador deu timeout ou curto
				
				ContadorIdentificarCurto_Elevador = TempoADC_IdentificaCurto_Elevador;
				CurtoElevador = false;
                //Se deu timeout não chegou no sensor 7

				//Desliga o elevador
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
				delayms(TempoEntreAcionamentoDesligarElevador_ms);
				CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

				//Indica erro
				StatusAutoTeste.SensoresDePosicao_OK = false; //ERRO
				StatusAutoTeste.SensorDeEntrega_OK = false;
				StatusAutoTeste.SensorDeRepouso_OK = false;
				
				//Se possível posicionar elevador no repouso
                TestaElevador_Estados = M_TE_SOBE_ELEVADOR_NO_REPOUSO;
                MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
            } 
			
			//Se fim de OK e durante o teste aciona um fim de curso indica erro
			if(Flag_FimDeCurso == 1) {
				//Elevador chegou em algum fim de curso
				
				//Indica erro			
				StatusAutoTeste.FimDeCurso_OK = false;				

				//Desliga o elevador
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
				delayms(TempoEntreAcionamentoDesligarElevador_ms);
				CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

				//Se possível posicionar elevador no repouso

				//Atualiza erro no sensor de entrega e repouso
				//Testa sensor de entrega
				if(SensoresPosicaoElevador_Validos[6] == 1) StatusAutoTeste.SensorDeEntrega_OK = true;
				else StatusAutoTeste.SensorDeEntrega_OK = false;
				//testa sensor de repouso
				if(SensoresPosicaoElevador_Validos[5] == 1) StatusAutoTeste.SensorDeRepouso_OK = true;				
				else StatusAutoTeste.SensorDeRepouso_OK = false;

				//Verifica se algum sensor de posição é válido
				if((SensoresPosicaoElevador_Validos[6] == 1) ||	(SensoresPosicaoElevador_Validos[5] == 1) ||
					(SensoresPosicaoElevador_Validos[4] == 1) || (SensoresPosicaoElevador_Validos[3] == 1) ||
					(SensoresPosicaoElevador_Validos[2] == 1) || (SensoresPosicaoElevador_Validos[1] == 1) ||
					(SensoresPosicaoElevador_Validos[0] == 1)) {
					//Algum sensores de posição válido

					//Sobe
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					delayms(TempoEntreAcionamentoDoElevador_ms);
					SET_BIT(Port_Elevador, Pino_Sobe_Elevador);		

					Copia_UltimaPosicaoValidaDoElevador = UltimaPosicaoValidaDoElevador;

					TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_CHEGAR_NO_REPOUSO;
					MaquinaDeEstados_AutoTeste.TempoProximoEstado = TEMPO_ESPERA_PARA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_ms;			
							
				} else {
					//Nem um sensor de posição válido finaliza o teste
					//Para elevador e termina subteste
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					delayms(TempoEntreAcionamentoDesligarElevador_ms);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					r = Status_AT_SubTesteFinalizado;
				}
			}

//DURANTE A DESCIDA VÁLIDA OS SENSORES DE POSIÇÃO
            
            if(!Entrada_74H165.ELEVADOR_P1) {
				SensoresPosicaoElevador_Validos[0] = 1;
				UltimaPosicaoValidaDoElevador = 1;
			}
            if(!Entrada_74H165.ELEVADOR_P2) {
				SensoresPosicaoElevador_Validos[1] = 1;
				UltimaPosicaoValidaDoElevador = 2;
			}
            if(!Entrada_74H165.ELEVADOR_P3) {
				SensoresPosicaoElevador_Validos[2] = 1;
				UltimaPosicaoValidaDoElevador = 3;
			}
            if(!Entrada_74H165.ELEVADOR_P4) {
				SensoresPosicaoElevador_Validos[3] = 1;
				UltimaPosicaoValidaDoElevador = 4;
			}
            if(!Entrada_74H165.ELEVADOR_P5) {
				SensoresPosicaoElevador_Validos[4] = 1;
				UltimaPosicaoValidaDoElevador = 5;
			}
            if(!Entrada_74H165.ELEVADOR_P6) {
				SensoresPosicaoElevador_Validos[5] = 1;
				UltimaPosicaoValidaDoElevador = 6;
			}
            if(!Entrada_74H165.ELEVADOR_P7) {
				SensoresPosicaoElevador_Validos[6] = 1;
				UltimaPosicaoValidaDoElevador = 7;
                //Se chegou no sensor 7 volta elevador para a posição de repouso
                TestaElevador_Estados = M_TE_SOBE_ELEVADOR_NO_REPOUSO;
                MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;					
             }
        break;

		case M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_CHEGAR_NO_REPOUSO:
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado <= 0) {
				TestaElevador_Estados = M_TE_POSICIONA_ELEVADOR_NO_ULTIMO_SENSOR_VALIDO;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = TimeOutMovimentaElevador_ms;			
			}
		break;

        case M_TE_SOBE_ELEVADOR_NO_REPOUSO:
            //Volta elevador para repouso
			//AQUI JÁ VÁLIDOU OS SENSORES DE POSIÇÃO DO ELEVADOR
		
			if((SensoresPosicaoElevador_Validos[6] == 1) &&
			   (SensoresPosicaoElevador_Validos[5] == 1) &&
			   (SensoresPosicaoElevador_Validos[4] == 1) &&
			   (SensoresPosicaoElevador_Validos[3] == 1) &&
			   (SensoresPosicaoElevador_Validos[2] == 1) &&
			   (SensoresPosicaoElevador_Validos[1] == 1) &&
			   (SensoresPosicaoElevador_Validos[0] == 1)) {
				//Sensores de posição OK
				StatusAutoTeste.SensoresDePosicao_OK = true;	   
				StatusAutoTeste.SensorDeEntrega_OK = true;
				StatusAutoTeste.SensorDeRepouso_OK = true;
				
				//Sobe
				CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
				delayms(TempoEntreAcionamentoDoElevador_ms);
				SET_BIT(Port_Elevador, Pino_Sobe_Elevador);
            
				TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_CHEGAR_NO_REPOUSO;
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = TimeOutMovimentaElevador_ms;				
			} else {
				//Algum sensor de posição INVÁLIDO
				
				//Indica erro
				StatusAutoTeste.SensoresDePosicao_OK = false; //ERRO

				//Testa sensor de entrega
				if(SensoresPosicaoElevador_Validos[6] == 1) StatusAutoTeste.SensorDeEntrega_OK = true;
				else StatusAutoTeste.SensorDeEntrega_OK = false;
				//testa sensor de repouso
				if(SensoresPosicaoElevador_Validos[5] == 1) StatusAutoTeste.SensorDeRepouso_OK = true;				
				else StatusAutoTeste.SensorDeRepouso_OK = false;
				
				if(StatusAutoTeste.SensorDeRepouso_OK == true) {
					//Se sensor de repouso OK
					
					//Sobe
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					delayms(TempoEntreAcionamentoDoElevador_ms);
					SET_BIT(Port_Elevador, Pino_Sobe_Elevador);
				
					TestaElevador_Estados = M_TE_ESPERA_ELEVADOR_CHEGAR_NO_REPOUSO;
					MaquinaDeEstados_AutoTeste.TempoProximoEstado = TimeOutMovimentaElevador_ms;						
				} else {
					//Se ERRO no sensor de repouso 

					//Verifica se algum sensor de posição é válido
					if((SensoresPosicaoElevador_Validos[6] == 1) ||	(SensoresPosicaoElevador_Validos[5] == 1) ||
			   			(SensoresPosicaoElevador_Validos[4] == 1) || (SensoresPosicaoElevador_Validos[3] == 1) ||
			   			(SensoresPosicaoElevador_Validos[2] == 1) || (SensoresPosicaoElevador_Validos[1] == 1) ||
			   			(SensoresPosicaoElevador_Validos[0] == 1)) {
						//Algum sensores de posição válido

						//Sobe
						CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
						delayms(TempoEntreAcionamentoDoElevador_ms);
						SET_BIT(Port_Elevador, Pino_Sobe_Elevador);		

						Copia_UltimaPosicaoValidaDoElevador = UltimaPosicaoValidaDoElevador;

						TestaElevador_Estados = M_TE_POSICIONA_ELEVADOR_NO_ULTIMO_SENSOR_VALIDO;
						MaquinaDeEstados_AutoTeste.TempoProximoEstado = TimeOutMovimentaElevador_ms;			
						
					} else {
						//Nem um sensor de posição válido finaliza o teste

						//Para elevador e termina subteste
						CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
						delayms(TempoEntreAcionamentoDesligarElevador_ms);
						CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

						r = Status_AT_SubTesteFinalizado;
					}					
				}
            }

        break;

		case M_TE_POSICIONA_ELEVADOR_NO_ULTIMO_SENSOR_VALIDO:
												
				//Verifica se elevador chegou na posição solicitada
				if(PosicaoAtualElevador() == Copia_UltimaPosicaoValidaDoElevador) {
					//Finaliza rotina
					
					//Indica fim da movimentação OK

					//Para elevador e termina subteste
					CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
					delayms(TempoEntreAcionamentoDesligarElevador_ms);
					CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);

					r = Status_AT_SubTesteFinalizado;
				} else {				
					if(MaquinaDeEstados_AutoTeste.TempoProximoEstado <= 0) {
						//Deu time out	
						r = Status_MM_SensorDePosicaoDoElevadorInvalido;

						//Para elevador e termina subteste
						CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
						delayms(TempoEntreAcionamentoDesligarElevador_ms);
						CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);
					}
				}
				
				//NAO VERIFICA CURTO E FIM DE CURSO PORQUE EH ULTIMO SENSOR VALIDO				
		break;

        case M_TE_ESPERA_ELEVADOR_CHEGAR_NO_REPOUSO:
            Entrada_74H165.Expansor = Le74HC165();     

            if(((SensoresPosicaoElevador_Validos[5] == 1) && (!Entrada_74H165.ELEVADOR_P6)) ||
			    (MaquinaDeEstados_AutoTeste.TempoProximoEstado == -1) || (CurtoElevador == true)) {
				//Elevador deu timeout ou curto				
				ContadorIdentificarCurto_Elevador = TempoADC_IdentificaCurto_Elevador;
				CurtoElevador = false;

				UltimaPosicaoValidaDoElevador = 6;

                //Para elevador e termina subteste
                CLEAR_BIT(Port_Elevador, Pino_Desce_Elevador);
                CLEAR_BIT(Port_Elevador, Pino_Sobe_Elevador);	

                r = Status_AT_SubTesteFinalizado;
            } 
        break;
    }   
    return r;
}

/* Retorna para MiniPC o Status do Autoteste
 * Parâmetro de entrada:
 * Não tem 
 *
 * Parâmetro de saída:
 * Não tem
 */
void Retorna_StatusDoAutoteste() {
	
	LimpaVetor(msg,Tamanho_BufferUSART0);
	//Coloca informação a ser enviada
	msg[0] = Caractere_Inicio;
	msg[1] = ProtocoloVersao;
	msg[2] = ComandoAutoTeste;
	msg[3] = TamanhoTX_ComandoAutoTeste;

	//Retorna status do FUSÍVEL DOS ATUADORES						
	if(StatusAutoTeste.Fusivel_Atuadores_OK == true) msg[4] = Resultado_AutoTesteParcial_OK;
	else msg[4] = Resultado_AutoTesteParcial_Falha;
						
	//Retorna status do SENSOR DE CORRENTE DOS ATUADORES						
	if(StatusAutoTeste.SensorDeCorrente_Atuadores_OK == true) msg[5] = Resultado_AutoTesteParcial_OK;
	else msg[5] = Resultado_AutoTesteParcial_Falha;
							
	if(LigaDesliga_Elevador == true) {
		//Elevador habilitado

		//Retorna status do FUSÍVEL DO ELEVADOR						
		if(StatusAutoTeste.Fusivel_Elevador_OK == true) msg[6] = Resultado_AutoTesteParcial_OK;
		else msg[6] = Resultado_AutoTesteParcial_Falha;
								
		//Retorna status da TRAVA DAS GAVETAS								
		if(StatusAutoTeste.TravaDasGavetas_OK == true) msg[7] = Resultado_AutoTesteParcial_OK;
		else msg[7] = Resultado_AutoTesteParcial_Falha;
								
		//Retorna status dos SENSORES DE POSIÇÃO DO ELEVADOR	
		//Gaveta 1						
		if(SensoresPosicaoElevador_Validos[0] == 0) msg[8] = Resultado_SensorDePosicaoDoElevador_Danificado;
		else msg[8] = Resultado_SensorDePosicaoDoElevador_OK;

		//Gaveta 2						
		if(SensoresPosicaoElevador_Validos[1] == 0) msg[9] = Resultado_SensorDePosicaoDoElevador_Danificado;
		else msg[9] = Resultado_SensorDePosicaoDoElevador_OK;

		//Gaveta 3					
		if(SensoresPosicaoElevador_Validos[2] == 0) msg[10] = Resultado_SensorDePosicaoDoElevador_Danificado;
		else msg[10] = Resultado_SensorDePosicaoDoElevador_OK;	

		//Gaveta 4					
		if(SensoresPosicaoElevador_Validos[3] == 0) msg[11] = Resultado_SensorDePosicaoDoElevador_Danificado;
		else msg[11] = Resultado_SensorDePosicaoDoElevador_OK;	

		//Gaveta 5				
		if(SensoresPosicaoElevador_Validos[4] == 0) msg[12] = Resultado_SensorDePosicaoDoElevador_Danificado;
		else msg[12] = Resultado_SensorDePosicaoDoElevador_OK;	

		//Gaveta 6					
		if(SensoresPosicaoElevador_Validos[5] == 0) msg[13] = Resultado_SensorDePosicaoDoElevador_Danificado;
		else msg[13] = Resultado_SensorDePosicaoDoElevador_OK;	

		//Gaveta 7					
		if(SensoresPosicaoElevador_Validos[6] == 0) msg[14] = Resultado_SensorDePosicaoDoElevador_Danificado;
		else msg[14] = Resultado_SensorDePosicaoDoElevador_OK;		
								
		if(StatusAutoTeste.FimDeCurso_OK == true) msg[15] = Resultado_AutoTesteParcial_OK;
			else msg[15] = Resultado_AutoTesteParcial_Falha;
	} else {
			//Elevador desabilitado

			//Retorna status do FUSÍVEL DO ELEVADOR
			msg[6] = Resultado_AutoTesteParcial_ElevadorDesabilitado;	

			//Retorna status da TRAVA DAS GAVETAS	
			msg[7] = Resultado_AutoTesteParcial_ElevadorDesabilitado;	
			
			//Retorna status dos SENSORES DE POSIÇÃO DO ELEVADOR	
			msg[8] = Resultado_SensorDePosicaoDoElevador_Desabilitado; // Gaveta1
			msg[9] = Resultado_SensorDePosicaoDoElevador_Desabilitado; // Gaveta2
			msg[10] = Resultado_SensorDePosicaoDoElevador_Desabilitado; // Gaveta3
			msg[11] = Resultado_SensorDePosicaoDoElevador_Desabilitado; // Gaveta4
			msg[12] = Resultado_SensorDePosicaoDoElevador_Desabilitado; // Gaveta5
			msg[13] = Resultado_SensorDePosicaoDoElevador_Desabilitado; // Gaveta6
			msg[14] = Resultado_SensorDePosicaoDoElevador_Desabilitado; // Gaveta7

			msg[15] = Resultado_AutoTesteParcial_ElevadorDesabilitado;	//fim de curso
	}
							
	if(StatusAutoTeste.SensorDeTemperatura_OK) msg[16] = Resultado_AutoTesteParcial_OK;
	else msg[16] = Resultado_AutoTesteParcial_Falha;
							
	//Resposta do STATUS da comunicação com controlador de temperatura
	msg[17] = StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK;
							
	//Calcula CRC
	ManipulaBytes.u_int = crc16_XMODEM_TX(msg,18);
					
	//Acrescenta CRC no pacote
	msg[18] = ManipulaBytes.bytes[0]; //LSB
	msg[19] =	ManipulaBytes.bytes[1]; //MSB
							
	msg[20] = Caractere_Fim;			//fim do pacote
							
	//Coloca informação na FILA
	AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,21,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);	
}
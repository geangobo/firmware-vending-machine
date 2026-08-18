/*
 * timers.c
 *
 * Created: 05/08/2021 07:29:24
 *  Author: André Cruz
 */ 

#include "timers.h"

//Variáveis do timer
volatile signed int Aux_Conta_1s = BaseTempo_1s;
volatile signed int Aux_Conta_1ms = BaseTempo_1ms;

//Maquina de estados do atuador
extern MaquinaDeEstados MaquinaDeEstados_Atuadores;
extern MaquinaDeEstados Posiciona_Elevador;
extern MaquinaDeEstados Movimenta_Elevador;
extern MaquinaDeEstados AUX_SolicitaCompra;
extern MaquinaDeEstados	MaquinaDeEstados_AutoTeste;

//Porta principal
extern volatile signed int TimeOut_PortaPrincipal;
extern volatile bool Flag_Le_PortaPrincipal;

//delay de uso geral
extern volatile signed long int DelayTimer0_ms;
extern volatile bool Flag_Delay;

//USART0
extern volatile char Num_msg_fila_MiniPC;
extern struct f Fila_MiniPC[TamanhoFilaUSART0];

extern volatile unsigned char Buffer_USART0[Tamanho_BufferUSART0], Pacote_USART0[Tamanho_BufferUSART0];
extern volatile signed char	TempoEsperaProximoPacote, TempoEsperaFimDoPacote, TempoEsperaFimTransmissaoModBus;

extern volatile unsigned char ContadorBytesRxUSART0,TamanhoDoPacote;
extern volatile bool Flag_JaRecebeuInicioDoPacote, Flag_PacoteRecebido, Flag_USART0_Ocupada;

extern volatile bool Flag_EnviaInformacao_MiniPC;

//Controlador de temperatura
extern volatile unsigned char Buffer_Modbus[Tamanho_BufferModbus];
extern volatile unsigned char Pacote_Modbus[Tamanho_BufferModbus];
extern volatile unsigned char ContadorBytesRx_ModBus;
extern volatile unsigned char TamanhoDoPacote_ModBus_Bytes;
extern volatile signed int TempoEsperaProximoByte_ModBus;
extern volatile bool Flag_PacoteRecebido_ModBus;
extern volatile bool Flag_EnviaInformacao_ControladorDeTemperatura;

extern volatile signed char TempoEsperaPulsoDoDegelo_ControladorDeTemperatura_s;

extern volatile bool Flag_JaRecebeuEndereco_ControladorDeTemperatura;

//fila
extern volatile unsigned char Num_msg_fila_ControladorDeTemperatura;
extern struct f_CT Fila_CT[TamanhoFilaModBus];

//Union com saídas do expansor
extern union Saida Saida_74HC595;

//Maquina de estados principal
extern volatile MaquinaDeEstados Maquina_Principal;

extern volatile bool LigaDesliga_SensorDeQueda;
extern volatile bool Flag_Status_SensorDeQueda;

//Leitura dos fim de curso
extern volatile bool Flag_Le_EntradasComFiltro;

//Interrupção do TIMER0
ISR(TIMER0_COMPA_vect) {
	//Interrupção de comparação do canal A do timer 0
	
	//Temporizador de 0,0001s OU 100us

	//Reinicia a contagem do timer 0
	TCNT0 = 0;

	//Base de tempo de 1ms
	if(--Aux_Conta_1ms <= 0) {
		Aux_Conta_1ms = BaseTempo_1ms;

		//Temporização do delay de uso geral
		if(DelayTimer0_ms > 0) DelayTimer0_ms--;
		else {
			if(DelayTimer0_ms == 0) {
				DelayTimer0_ms = -1;
				Flag_Delay = true;
			}
		}

		//Indica que deve ler as entradas
		Flag_Le_EntradasComFiltro = true;

		//Se sensor de queda HABILITADO e sensor acionado marca Flag_Status_SensorDeQueda
		if(LigaDesliga_SensorDeQueda == true){
			
			if(Leitura_Port_SensorDeQueda & (1 << Pino_SensorDeQueda)) {
				//Acionado
				Flag_Status_SensorDeQueda = true;
			} 
		}	

//TEMPORIZAÇÃO DO MODBUS
		//Temporização do envio pelo ModBus
		if(TempoEsperaFimTransmissaoModBus > 0) TempoEsperaFimTransmissaoModBus--;
		else{
			if(TempoEsperaFimTransmissaoModBus == 0) {
				//Fim da temporização
				
				//Garante recepção de dados pelo ModBus
				CLEAR_BIT(Port_TX_RX_MODBUS,Pino_TX_RX_MODBUS);		
				
				//Indica que USART está livre para uso
				Flag_USART0_Ocupada = false;		
			}
		}
		
		//Temporização da resposta pelo Modbus		
		if((Fila_CT[0].TempoEsperaResposta_ControladorDeTemperatura_ms > 0) && (Num_msg_fila_ControladorDeTemperatura > 0) && (Num_msg_fila_MiniPC == 0)) Fila_CT[0].TempoEsperaResposta_ControladorDeTemperatura_ms--;
				
		if(TempoEsperaProximoByte_ModBus > 0) TempoEsperaProximoByte_ModBus--;
		else if(TempoEsperaProximoByte_ModBus == 0) {			
			//Aqui ficou o tempo necesario sem receber bytes
			
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
		
		//Lógica da fila de envio ModBus
		if((Num_msg_fila_ControladorDeTemperatura > 0) && (Num_msg_fila_MiniPC == 0)) {
			//Se tiver elemento na fila


			//Espera tempo para enviar mensagem
			if(Fila_CT[0].TempoEsperaEnvioMsg > 1) {
				Fila_CT[0].TempoEsperaEnvioMsg--;
			}
			if(Fila_CT[0].TempoEsperaEnvioMsg == 1) {
				//Já esperou o tempo necessário para enviar mensagem
				Fila_CT[0].TempoEsperaEnvioMsg = 0;
				//Marca flag para enviar mensagem
				Flag_EnviaInformacao_ControladorDeTemperatura = true;
				
			}
		}

//Temporização da USART0
		//Espera tempo sem receber informações para identificar fim do pacote
		if(TempoEsperaFimDoPacote > 1) {
			TempoEsperaFimDoPacote--;	
		} 
		if(TempoEsperaFimDoPacote == 1) {
			//Esperou o tempo necessário
		
			//Zera contador
			TempoEsperaFimDoPacote = 0;	
		
			//Tempo para receber o próximo pacote
			//Descontar o tempo de recebimento de 1 byte (9600 bps) +/- 1,04ms
			TempoEsperaProximoPacote = TempoReceberProximoPacote_ms;		

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
				
		//Espera tempo para receber próximo pacote
		if(TempoEsperaProximoPacote > 1) {
			TempoEsperaProximoPacote--;
		}
		if(TempoEsperaProximoPacote == 1) {
			//Esperou tempo necessário
			TempoEsperaProximoPacote = 0;			
		}
	
		//Lógica da fila de envio serial
		if(Num_msg_fila_MiniPC > 0) {
			//Se tiver elemento na fila

			//Espera tempo para enviar mensagem
			if(Fila_MiniPC[0].TempoEsperaEnvioMsg > 1) {
				Fila_MiniPC[0].TempoEsperaEnvioMsg--;
			}	
			if(Fila_MiniPC[0].TempoEsperaEnvioMsg == 1) {
				//Já esperou o tempo necessário para enviar mensagem
				Fila_MiniPC[0].TempoEsperaEnvioMsg = 0;	
				//Marca flag para enviar mensagem		
				Flag_EnviaInformacao_MiniPC = true;			
			}
		
			//Já enviou resposta?
			if(Fila_MiniPC[0].Flag_JaEnviouResposta == true) {
				//Sim
							
				if(Fila_MiniPC[0].EsperaRespostaMiniPc == true) {
					//Sim
								
					if(Fila_MiniPC[0].TempoEsperaRespostaMiniPc > 1) {
						Fila_MiniPC[0].TempoEsperaRespostaMiniPc--;					
					}
					if(Fila_MiniPC[0].TempoEsperaRespostaMiniPc == 1) {
						//Já passou o tempo para mestre responder				
						Fila_MiniPC[0].TempoEsperaRespostaMiniPc = 0;
					
						if(--Fila_MiniPC[0].TentativaEnvioRespostaMiniPc > 0) {
							//Ainda não esgotou as tentativas
																							
							Fila_MiniPC[0].TempoEsperaEnvioMsg = TempoEsperaTentativasEnvioResposta;	//EsperaRespostaMiniPc + esse valor													
							Fila_MiniPC[0].TempoEsperaRespostaMiniPc = TempoEsperaResposta_ms;
						
							//Indica que ainda não enviou resposta para repetir as tentativas
							Fila_MiniPC[0].Flag_JaEnviouResposta = false;
							
						} else {
							//Esgotou as tentativas
						
							//Inidica que não está mais esperando resposta do Mini Pc
							Fila_MiniPC[0].EsperaRespostaMiniPc = false;

							RemoveMsgFila_MiniPc(0x01);
						}
					}
				} else {
					//Não espera resposta
				
					RemoveMsgFila_MiniPc(0x01);					
				}
			} 
		}

//Temporização da maquina de estado dos atuadores		
		if(MaquinaDeEstados_Atuadores.TempoProximoEstado > 0) {
			MaquinaDeEstados_Atuadores.TempoProximoEstado--;
		} else{
			if(MaquinaDeEstados_Atuadores.TempoProximoEstado == 0) {
				//Fim da temporização
				//Desliga temporização
				MaquinaDeEstados_Atuadores.TempoProximoEstado = -1;	
			}
		}
		
		//Temporização do autoteste
		if(MaquinaDeEstados_AutoTeste.TempoProximoEstado > 0) {
			MaquinaDeEstados_AutoTeste.TempoProximoEstado--;
			} else {
			if(MaquinaDeEstados_AutoTeste.TempoProximoEstado == 0) {
				//Fim da temporização
				MaquinaDeEstados_AutoTeste.TempoProximoEstado = -1;
			}
		}	

		//Deve temporizar solicitação de compra?
		if(AUX_SolicitaCompra.TempoProximoEstado > 0) {
			//Sim				
			AUX_SolicitaCompra.TempoProximoEstado--;
				
			//Contador terminou?
			if(AUX_SolicitaCompra.TempoProximoEstado == 0) {
				//Sim
				//Desliga temporizador
				AUX_SolicitaCompra.TempoProximoEstado = -1;
			}
		}	

//Temporização de movimentação do elevador
		//Deve temporizar movimentação do elevador?
		if(Movimenta_Elevador.TempoProximoEstado > 0 ) {
			//Sim		
			Movimenta_Elevador.TempoProximoEstado--;					
			
			//Contador terminou?
			if(Movimenta_Elevador.TempoProximoEstado == 0) {
				//Sim
				//Desliga temporizador
				Movimenta_Elevador.TempoProximoEstado = -1;
			}
		}													
	}
	
	//Base de tempo de 1 segundo
	if(Aux_Conta_1s > 0) {
		Aux_Conta_1s--;
		
		//Contador terminou?
		if(Aux_Conta_1s == 0) {	
			//Sim	
			//Passou 1 segundo
			
			//Recarrega base de tempo
			Aux_Conta_1s = BaseTempo_1s;

			//Verifica porta principal a cada 1s
			Flag_Le_PortaPrincipal = true;

//DURAÇÃO DO PULSO DO DEGELO
			if(TempoEsperaPulsoDoDegelo_ControladorDeTemperatura_s > 0) {
				TempoEsperaPulsoDoDegelo_ControladorDeTemperatura_s--;
				
				if(TempoEsperaPulsoDoDegelo_ControladorDeTemperatura_s == 0) {
					TempoEsperaPulsoDoDegelo_ControladorDeTemperatura_s = -1;
					
					Maquina_Principal.Estado = M_Ocioso;
					
					//Termina pulso do Degelo
					Saida_74HC595.DEGELO = 0;
					Escreve74HC595(Saida_74HC595.Expansor);
				}
			}

//TEMPORIZAÇÃO DO ALARME DA PORTA ABERTA		
			//Deve temporizar para acionar o BUZZER?
			if(TimeOut_PortaPrincipal > 0) {
				//Sim
				TimeOut_PortaPrincipal--;
		
				//Contador terminou?
				if (TimeOut_PortaPrincipal == 0) {
					//Sim
					//Encera contador
					TimeOut_PortaPrincipal = -1;
			
					//Liga o BUZZER
					SET_BIT(Port_BUZZER,Pino_BUZZER);
				}
			}
	
//TEMPORIZAÇÃO DO ELEVADOR		

			//Posicionamento inicial do elevador
			//Deve temporizar posição do elevador?
			if(Posiciona_Elevador.TempoProximoEstado > 0) {
				//Sim
			
				Posiciona_Elevador.TempoProximoEstado--;
		
				//Contador terminou?
				if(Posiciona_Elevador.TempoProximoEstado == 0) {
					//Sim
					//Desliga temporizador
					Posiciona_Elevador.TempoProximoEstado = -1;
			
					if(Posiciona_Elevador.Estado == MP_ElevadorGaveta1) {
						//Se chegou aqui elevador não chegou no sensor 1 em 10 segundos
				
						//Vai para próximo estado 
						Posiciona_Elevador.Estado = MP_DesceElevador;
					} else if(Posiciona_Elevador.Estado == MP_ElevadorGaveta6) {
						//Se chegou aqui elevador não chegou no sensor 6 em 10 segundos

						//Vai para próximo estado
						Posiciona_Elevador.Estado = MP_ElevadorNaPosicao;				
					}
				}
			}
		}
	}
	
}

/* Configura e inicializa o timer 0 para gerar interrupção de acordo com o parâmetro OverflowTimer0
 * Parâmetro de entrada:
 * Parâmetro 1 - Intervalo de tempo para gerar uma interrupção do timer 0 em us (microsegundos)
 *
 * Parâmetro de saída:
 * Não tem.
*/
void InicializaTimer0 (unsigned long int OverflowTimer0) {
	unsigned long int basetmr0 = 0;
	
	//Não deixa ajustar overflow menor que 1us
	if(OverflowTimer0 < 3) OverflowTimer0 = 2;
	
	//Calcula base de tempo do timer 0
	basetmr0 = (F_CPU/64)/(1000000/(OverflowTimer0-1));

	TCCR0A = 0b00000000;
	TCCR0B = 0b00000011;	//1:64 16MHz ~4us 
	TCNT0 = 0;
	OCR0A = (unsigned char) basetmr0;
	//Liga interrupção de comparação do canal 0 do timer 0
	TIMSK0 |= (1 << OCIE0A);	
}
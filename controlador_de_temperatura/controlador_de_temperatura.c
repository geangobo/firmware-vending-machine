/*
 * controlador_de_temperatura.c
 *
 * Created: 06/01/2023 10:07:04
 *  Author: André
 */ 

#include "controlador_de_temperatura.h"

//Comunicação com MiniPC
extern volatile unsigned char Num_msg_fila_MiniPC;
extern struct f Fila_MiniPC[TamanhoFilaUSART0];

extern volatile unsigned char msg[Tamanho_BufferUSART0];

extern volatile unsigned char QuemEstaUsandoSerial;

volatile unsigned char Buffer_ControladorDeTemperatura_Conversao[Tamanho_Buffer_ControladorDeTemperatura_Conversao];

//Variáveis com parâmetros do controlador de temperatura
volatile unsigned char Aux_ControladorTemperatura[2];

volatile unsigned char Setpoint_C[Comprimento_Setpoint];
volatile unsigned char Histerese_C[Comprimento_Histerese];
volatile unsigned char SetpointMinimo_C[Comprimento_SetpointMinimo];
volatile unsigned char SetpointMaximo_C[Comprimento_SetpointMaximo];
volatile unsigned char AlarmeTemperaturaAmbienteBaixa_C[Comprimento_AlarmeTemperaturaAmbienteBaixa];
volatile unsigned char AlarmeTemperaturaAmbienteAlta_C[Comprimento_AlarmeTemperaturaAmbienteAlta];
volatile unsigned char IntervaloEntreDegelos_Minutos[Comprimento_IntervaloEntreDegelos];
volatile unsigned char TempoDeDuracaoMaximaDoDegelo_Minutos[Comprimento_TempoDeDuracaoMaximaDoDegelo];
volatile unsigned char TempoDeIndicacaoDeTemperaturaTravada_Minutos[Comprimento_TempoDeIndicacaoDeTemperaturaTravada];
volatile unsigned char ControlaBuzzer_OnOff[Comprimento_ControlaBuzzer];

volatile signed char TempoEsperaPulsoDoDegelo_ControladorDeTemperatura_s = -1;

//Comunicação 
volatile unsigned char ControlaComunicacao_Controlador_de_temperatura = 0; //liga ou desliga
volatile unsigned char Estado_Comunicacao_ControladorDeTemperatura = 0;	//maquina de estados
volatile bool Flag_JaRecebeuEndereco_ControladorDeTemperatura = false;
volatile unsigned char IdentificaComando_ControladorDeTemperatura = 0; //1 configuração completa 2 configura só setpoint 3 leitura das configurações
volatile unsigned char Identifica_Parametro_ControladorDeTemperatura = 0;	

//fila
volatile unsigned char Num_msg_fila_ControladorDeTemperatura = 0;

//Comunicação
volatile unsigned char Buffer_Modbus[Tamanho_BufferModbus];
volatile unsigned char Pacote_Modbus[Tamanho_BufferModbus];
volatile unsigned char ContadorBytesRx_ModBus = 0;
volatile unsigned char TamanhoDoPacote_ModBus_Bytes = 0;
volatile signed int TempoEsperaProximoByte_ModBus = -1;
volatile bool Flag_PacoteRecebido_ModBus = false;
volatile bool Flag_EnviaInformacao_ControladorDeTemperatura = false;

//Declara vetor com de structs f para Fila do ModBus
struct f_CT Fila_CT[TamanhoFilaModBus];

//Struct de status do auto teste
extern AT	StatusAutoTeste;

//Maquina de estados principal
extern volatile MaquinaDeEstados Maquina_Principal;

//Union auxiliar para manipular bytes
union {
	unsigned char bytes[4];
	signed int s_int;
	unsigned int u_int;
	unsigned long u_long_int;
	float f;
}ManipulaBytes;

/* Comunica com controlador de temperatura Full Gauge TC900E Log
 * Parâmetro de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void Comunicacao_ControladorDeTemperatura () {
	static unsigned char Retorno_USART; 
	float Aux_f = 0.0;
	
	if((Num_msg_fila_MiniPC == 0) && (Fila_MiniPC[0].EsperaRespostaMiniPc==false)) {
		//Não tem mensagem da fila da UART e não está esperando resposta do MiniPC
			
		switch(Estado_Comunicacao_ControladorDeTemperatura) {
			case Estado_CT_Inicia_MODBUS:
				//Liga comunicação MODBUS Controlador de temperatura
				SET_BIT(Port_Selecao_USART_MODBUS, Pino_Selecao_USART);
				//Direciona USART do uC para MODBUS
				CLEAR_BIT(Port_Selecao_USART_MODBUS, Pino_Selecao_MODBUS);
				
				Retorno_USART = 0;
					
				QuemEstaUsandoSerial = _ModBus;
				Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_EsperaResposta_MODBUS;
					
			break;
				
			case Estado_CT_EsperaResposta_MODBUS:
				if(Fila_CT[0].TempoEsperaResposta_ControladorDeTemperatura_ms > 0) {
					//Esperando resposta do controlador de temperatura
					
					if(Flag_PacoteRecebido_ModBus == true) {
						//Recebeu informação pelo ModBUs
						
						//Indica que não recebeu informação pelo ModBus
						Flag_PacoteRecebido_ModBus = false;			
						
						//Verifica mensagem
							
						if(IdentificaComando_ControladorDeTemperatura == Configura_ControladorDeTemperatura) {

							//Compara o valor enviado ao controlador com o valor retornado pelo controlador de temperatura byte a byte															
							if(ComparaInformacao_ControladorDeTemperatura(TamanhoDoPacote_ModBus_Bytes) ){
								//Escrita correta
#warning ADICIONAR NA ROTINA REMOVEMSGFILA_MODBUS PARA LIMPAR TODA A FILA																
								//Remove informação da fila e envia a próxima se houver
								//RemoveMsgFila_ControladorDeTemperatura(1);
								InicializaFila_ControladorDeTemperatura(TamanhoFilaModBus);
									
								//Enviou todas informações da fila ?
								if(Num_msg_fila_ControladorDeTemperatura == 0) {
									//Sim									
									//StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK = true;
									#warning REVISAR 2 CONTROLADOR DE TEMPERATURA
									//Retorna OK para MiniPC
									Retorno_USART = Resposta_ControladorDeTemperatura_ConfiguracaoOK;//OK
									Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_EnviaRespostaPela_UART;
								} else {
									//Ainda não										
								}									
									
							} else {

								//Se resposta não foi igual ao comando enviado
								//Verifica se houve a tentativa de configuração com valor inválido
								//Controlador respode no byte 1 o valor 0x86
								if(Pacote_Modbus[1] == 0x86) {
									Retorno_USART = Resposta_ControladorDeTemperatura_DadoInvalido;
								} else {
									Retorno_USART = Resposta_ControladorDeTemperatura_Desconectado;									
								}
												
								//Reinicializa a fila do ModBus apagando toda fila
								InicializaFila_ControladorDeTemperatura(TamanhoFilaModBus);
								#warning REMOVIDO PARA TESTE
								//RemoveMsgFila_ControladorDeTemperatura(1);																
								
								Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_EnviaRespostaPela_UART;								
							}

						} else if(IdentificaComando_ControladorDeTemperatura == LeConfiguracao_ControladorDeTemperatura) {
							//Copia resposta do controlador de temperatura para Aux
							Aux_ControladorTemperatura[1] = Pacote_Modbus[3];	//MSB
							Aux_ControladorTemperatura[0] = Pacote_Modbus[4];	//LSB								

							InicializaFila_ControladorDeTemperatura(TamanhoFilaModBus);	
							//Remove informação da fila e envia a próxima se houver
							//RemoveMsgFila_ControladorDeTemperatura(1);
																		
							//Inidca que recebeu resposta do controlador de temperatura e atualiza status no auto teste
							//StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK = true;
							#warning REVISAR 1 CONTROLADOR DE TEMPERATURA
										
							//Retorna OK para MiniPC
							Retorno_USART = Resposta_ControladorDeTemperatura_ConfiguracaoOK;//OK
							Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_EnviaRespostaPela_UART;				

						} else if(IdentificaComando_ControladorDeTemperatura == TC900Elog_ValidaComunicacaoCom_ControladorDeTemperatura) {
							Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_EnviaRespostaPela_UART;
								
							if((Pacote_Modbus[0] == EnderecoModBus_ControladorDeTemperatura) && 
							   (Pacote_Modbus[1] == Modbus_LeituraUnicoRegistrador) &&
							   (Pacote_Modbus[2] == Tamanho_RespostaIDFabricante_ControladorDeTemperatura) && 
							   (Pacote_Modbus[3] == RX_MSB_ID_Fabricante_ControladorDeTemperatura) &&
							   (Pacote_Modbus[4] == RX_LSB_ID_Fabricante_ControladorDeTemperatura) && 
							   (Pacote_Modbus[5] == RX_MSB_CRC_ID_Fabricante_ControladorDeTemperatura)	&&
							   (Pacote_Modbus[6] == RX_LSB_CRC_ID_Fabricante_ControladorDeTemperatura)) {
								//Escrita correta
								StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK = ControladorDeTemperatura_TC900Elog_Conectado;
																								
							} else {
								//Erro na escrita									
								//StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK = ControladorDeTemperatura_Desconectado;
								#warning COMENTADO PARA TESTE
							}	
								
							//Reinicializa a fila do ModBus apagando toda fila
							InicializaFila_ControladorDeTemperatura(TamanhoFilaModBus);			
							
						} else if(IdentificaComando_ControladorDeTemperatura == MT512Elog_ValidaComunicacaoCom_ControladorDeTemperatura) {
							Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_EnviaRespostaPela_UART;
						
							if((Pacote_Modbus[0] == EnderecoModBus_ControladorDeTemperatura) &&
							(Pacote_Modbus[1] == Modbus_LeituraUnicoRegistrador) &&
							(Pacote_Modbus[2] == Tamanho_RespostaIDFabricante_ControladorDeTemperatura) &&
							(Pacote_Modbus[3] == RX_MSB_ID_Fabricante_ControladorDeTemperatura) &&
							(Pacote_Modbus[4] == RX_LSB_ID_Fabricante_ControladorDeTemperatura) &&
							(Pacote_Modbus[5] == RX_MSB_CRC_ID_Fabricante_ControladorDeTemperatura)	&&
							(Pacote_Modbus[6] == RX_LSB_CRC_ID_Fabricante_ControladorDeTemperatura)) {
								//Escrita correta
								StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK = ControladorDeTemperatura_MT512Elog_Conectado;
							
							} else {
								//Erro na escrita
								//StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK = ControladorDeTemperatura_Desconectado;
								#warning COMENTADO PARA TESTE EVITAR
							}
						
							//Reinicializa a fila do ModBus apagando toda fila
							InicializaFila_ControladorDeTemperatura(TamanhoFilaModBus);																								
						} else {
							//Reinicializa a fila do ModBus apagando toda fila
							InicializaFila_ControladorDeTemperatura(TamanhoFilaModBus);
									
							//Retorna OK para MiniPC
							#warning ALTERACAO 2 Controlador de temperatura
							//Retorno_USART = 2;//ERRO
							Retorno_USART = Resposta_ControladorDeTemperatura_Desconectado;
							
							Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_EnviaRespostaPela_UART;								
						}
					}
				} else {
					//Timeout na resposta do controlador de temperatura
					
					if(--Fila_CT[0].NumeroDeTentativaEnvio_ControladorDeTemperatura > 0) {
						//Ainda não esgotou todas as tentativas de comunicação com o controlador de temperatura
						
						//Reinicia tempo para enviar novamente a mensagem da fila
						Fila_CT[0].TempoEsperaEnvioMsg = TempoEsperaParaEnviarPacote_ControladorDeTemperatura_ms;
						//Reinicia tempo para esperar a resposta do controlador de temperatura
						Fila_CT[0].TempoEsperaResposta_ControladorDeTemperatura_ms = TempoEsperaParaReceberResposta_ControladorDeTemperatura_ms;
						
					} else {
						//Esgotou as tentativas

#warning QUANDO ESGOTA AS TENTATIVAS DE ENVIO PARA O CONTROLADOR MANTEM STATUS CONECTADO
						//Erro na escrita
						//StatusAutoTeste.ComunicacaoComControladorDeTemperatura_OK = ControladorDeTemperatura_Desconectado;
									
						//Reinicializa a fila do ModBus apagando toda fila
						InicializaFila_ControladorDeTemperatura(TamanhoFilaModBus);
						#warning REMOVIDO PARA TESTE 
						//RemoveMsgFila_ControladorDeTemperatura(1);
						
						#warning ALTERACAO 3 Controlador de temperatura			
						//Retorna OK para MiniPC
						//Retorno_USART = 2;//ERRO
						Retorno_USART = Resposta_ControladorDeTemperatura_Desconectado;
						
						Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_EnviaRespostaPela_UART;
					}
				}
			break;
				
			case Estado_CT_EnviaRespostaPela_UART:
				//Liga comunicação com dispositivo USART
				CLEAR_BIT(Port_Selecao_USART_MODBUS, Pino_Selecao_USART);
				//Direciona USART do uC para USART
				SET_BIT(Port_Selecao_USART_MODBUS, Pino_Selecao_MODBUS);
				
				QuemEstaUsandoSerial = _USART;
			
				//Quando é comando de configuração retorna apenas OK ou ERRO!
				//Responde de acordo com o comando

				if(IdentificaComando_ControladorDeTemperatura == Configura_ControladorDeTemperatura) {

					//Responde para MiniPC pela USART
					LimpaVetor(msg,Tamanho_BufferUSART0);
					msg[0] = Caractere_Inicio;
					msg[1] = ProtocoloVersao;
				    msg[2] = ComandoConfiguraParametroDoControladorDeTemperatura;
					msg[3] = 0x01;	//Tamanho
					msg[4] = Retorno_USART;							
					//Calcula CRC
					ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
							
					//Acrescenta CRC no pacote
					msg[5] = ManipulaBytes.bytes[0]; //LSB
					msg[6] = ManipulaBytes.bytes[1]; //MSB
						
					msg[7] = Caractere_Fim;			//fim do pacote

					AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
					
				} else if(IdentificaComando_ControladorDeTemperatura == LeConfiguracao_ControladorDeTemperatura) {
					//Responde para MiniPC pela USART
					LimpaVetor(msg,Tamanho_BufferUSART0);
					
					#warning ALTERACAO 4 Controlador de temperatura
					if(Retorno_USART == Resposta_ControladorDeTemperatura_Desconectado)	{
					//if(Retorno_USART == 2) {
						//ERRO Timeout na comunicação com o controlador

						msg[0] = Caractere_Inicio;
						msg[1] = ProtocoloVersao;
					
						msg[2] = ComandoLeConfiguracaoDoControladorDeTemperatura;

						msg[3] = 0x01;	//Tamanho
						msg[4] = Retorno_USART;
						//Calcula CRC
						ManipulaBytes.u_int = crc16_XMODEM_TX(msg,5);
						
						//Acrescenta CRC no pacote
						msg[5] = ManipulaBytes.bytes[0]; //LSB
						msg[6] = ManipulaBytes.bytes[1]; //MSB
						
						msg[7] = Caractere_Fim;			//fim do pacote

						AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,8,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);							
					} else {
						
						msg[0] = Caractere_Inicio;
						msg[1] = ProtocoloVersao;
					
						msg[2] = ComandoLeConfiguracaoDoControladorDeTemperatura;						
						
						msg[4] = Identifica_Parametro_ControladorDeTemperatura;

						if((Identifica_Parametro_ControladorDeTemperatura >= Parametro_SetPoint_C_ControladorDeTemperatura) && (Identifica_Parametro_ControladorDeTemperatura <= Parametro_AlarmeTemperaturaAmbienteAlta_C_ControladorDeTemperatura)) {
							msg[3] = Tamanho_Comando_Temperatura_ParametroDoControladorDeTemperatura;	//Tamanho

							//Converte a temperatura 
							ManipulaBytes.f = 0.0;
							ManipulaBytes.bytes[0] = Aux_ControladorTemperatura[0];
							ManipulaBytes.bytes[1] = Aux_ControladorTemperatura[1];		

//ATENÇÃO: NÃO ALTERAR A SEQUÊNCIA ABAIXO
							Aux_f = (float) ManipulaBytes.s_int;
							Aux_f /= 10.0;

							LimpaVetor(Buffer_ControladorDeTemperatura_Conversao, Tamanho_Buffer_ControladorDeTemperatura_Conversao);
							FloatToStr(Aux_f, Buffer_ControladorDeTemperatura_Conversao, 3,1);
		
							msg[5] = Buffer_ControladorDeTemperatura_Conversao[0];
							msg[6] = Buffer_ControladorDeTemperatura_Conversao[1];
							msg[7] = Buffer_ControladorDeTemperatura_Conversao[2];
							msg[8] = Buffer_ControladorDeTemperatura_Conversao[3];
							msg[9] = Buffer_ControladorDeTemperatura_Conversao[4];
							msg[10] = Buffer_ControladorDeTemperatura_Conversao[5];

							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,11);
							
							//Acrescenta CRC no pacote
							msg[11] = ManipulaBytes.bytes[0]; //LSB
							msg[12] = ManipulaBytes.bytes[1]; //MSB
							
							msg[13] = Caractere_Fim;			//fim do pacote

							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,14,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
						} else if((Identifica_Parametro_ControladorDeTemperatura >= Parametro_IntervaloEntreDegelos_Minutos_ControladorDeTemperatura) && (Identifica_Parametro_ControladorDeTemperatura <= Parametro_TempoDeIndicacaoDeTemperaturaTravada_Minutos_ControladorDeTemperatura)) {						
							msg[3] = Tamanho_Comando_Tempo_ParametroDoControladorDeTemperatura;	//Tamanho

							//Converte a tempo 
							ManipulaBytes.f = 0.0;
							ManipulaBytes.bytes[0] = Aux_ControladorTemperatura[0];
							ManipulaBytes.bytes[1] = Aux_ControladorTemperatura[1];		

							Aux_f = (float) ManipulaBytes.s_int;

							LimpaVetor(Buffer_ControladorDeTemperatura_Conversao, Tamanho_Buffer_ControladorDeTemperatura_Conversao);
							FloatToStr(Aux_f, Buffer_ControladorDeTemperatura_Conversao, 4,0);		
						
							msg[5] = Buffer_ControladorDeTemperatura_Conversao[1];	//Remove sinal
							msg[6] = Buffer_ControladorDeTemperatura_Conversao[2];
							msg[7] = Buffer_ControladorDeTemperatura_Conversao[3];
							msg[8] = Buffer_ControladorDeTemperatura_Conversao[4];
#warning ADICIONADA LEITURA DO 4 DIGITO NA leitura da configuracao
							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,9);
							
							//Acrescenta CRC no pacote
							msg[9] = ManipulaBytes.bytes[0]; //LSB
							msg[10] = ManipulaBytes.bytes[1]; //MSB
							
							msg[11] = Caractere_Fim;			//fim do pacote

							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,12,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
						} else if(Identifica_Parametro_ControladorDeTemperatura == Parametro_Buzzer_ControladorDeTemperatura) {
							msg[3] = Tamanho_Comando_Buzzer_ParametroDoControladorDeTemperatura;	//Tamanho

							//Converte a tempo 
							ManipulaBytes.f = 0.0;
							ManipulaBytes.bytes[0] = Aux_ControladorTemperatura[0];
							ManipulaBytes.bytes[1] = Aux_ControladorTemperatura[1];		

							Aux_f = (float) ManipulaBytes.s_int;

							LimpaVetor(Buffer_ControladorDeTemperatura_Conversao, Tamanho_Buffer_ControladorDeTemperatura_Conversao);
							FloatToStr(Aux_f, Buffer_ControladorDeTemperatura_Conversao, 1,0);		
						
							#warning ALTERACAO 5 Controlador de temperatura
							//msg[5] = Buffer_ControladorDeTemperatura_Conversao[1];	//Remove sinal
							msg[5] = Buffer_ControladorDeTemperatura_Conversao[1]+1;	//Remove sinal

							//Calcula CRC
							ManipulaBytes.u_int = crc16_XMODEM_TX(msg,6);
							
							//Acrescenta CRC no pacote
							msg[6] = ManipulaBytes.bytes[0]; //LSB
							msg[7] = ManipulaBytes.bytes[1]; //MSB
							
							msg[8] = Caractere_Fim;			//fim do pacote

							AdicionaMsgFila_MiniPc(msg,TempoEsperaEnvioResposta_ms,9,true,TempoEsperaResposta_ms,NumeroTentativasEnvioRespostaPadrao);
						}
					}
				}
					
				//Sinaliza que não deve se comunicar com o controlador de temperatura
				ControlaComunicacao_Controlador_de_temperatura = 0;
				
				Estado_Comunicacao_ControladorDeTemperatura = Estado_CT_Inicia_MODBUS;
				
				IdentificaComando_ControladorDeTemperatura = 0;
				Identifica_Parametro_ControladorDeTemperatura = 0;
					
				//Se a maquina de estdos principal estiver ocupada retorna para ocioso
				if(Maquina_Principal.Estado == M_ComunicandoCom_ControladorDeTemperatura) InicializaMaquinaDeEstados_Principal(M_Ocioso);
			break;
		}
	}	
}

/* Compara informação presente no vetor "Pacote_Modbus" com a primeira posição da fila de envio "Fila_CT[0]"
 * Parâmetro de entrada:
 * Parâmetro 1 - Numero de bytes a ser comparado
 *
 * Parâmetro de saída: 
 * 	0 se Pacote_ModBus for DIFERENTE de Fila_CT[0]
 *  1 se Pacote_ModBus for IGUAL de Fila_CT[0]
*/
unsigned char ComparaInformacao_ControladorDeTemperatura(unsigned char _TamanhoDoPacote_ModBus_Bytes) {
	unsigned char i,x = 0, Retorno = 0;
	
	if(Fila_CT[0].Tamanho_msg_ModBus_Bytes == _TamanhoDoPacote_ModBus_Bytes) {
		//Verifica byte a byte
		for(i = 0; i < _TamanhoDoPacote_ModBus_Bytes ; i++) {
			if(Pacote_Modbus[i] == Fila_CT[0].msg[i]) x++;										
		}
	
		if(_TamanhoDoPacote_ModBus_Bytes == x) Retorno = 1;
	}
	return(Retorno);
}

/*Inicializa a fila de envio pelo MODBUS para controlador de temperatura
 * Parâmetros de entrada:
 * Parâmetro 1 - Número máximo de mensagens possível na fila
 *
 * Parâmetro de saída:
 * Não tem.
 */
void InicializaFila_ControladorDeTemperatura(volatile unsigned char TamanhoMaxFila) {
	volatile unsigned char i,x;
	
	//Inicializa a fila
	for(i = 0 ; i < TamanhoMaxFila ; i++) {
		//Limpa o tempo de espera na resposta do controlador de temperatura
		Fila_CT[i].TempoEsperaResposta_ControladorDeTemperatura_ms = 0;

		//Limpa número de tentativas de envio 
		Fila_CT[i].NumeroDeTentativaEnvio_ControladorDeTemperatura = 0;

		//Limpa tamanho da msg em bytes
		Fila_CT[i].Tamanho_msg_ModBus_Bytes = 0;
		
		//limpa tempo de espera para enviar próxima mensagem
		Fila_CT[i].TempoEsperaEnvioMsg = 0;
	
		//Limpa todo vetor da mensagem
		for(x = 0 ; x < Tamanho_BufferModbus ; x++) Fila_CT[i].msg[x] = 0;
	}	
	Num_msg_fila_ControladorDeTemperatura = 0;	//Inicializa sem mensagens na fila do MODBUS
}

/* Adiciona mensagem na fila de envio pelo MODBUS para controlador de temperatura
 * Parâmetros de entrada:
 * Parâmetro 1 - Informações a serem enviadas 
 * Parâmetro 2 - Tempo em milisegundos para esperar para realizar o envio da mensagem
 * Parâmetro 3 - Tamanho do Pacote a ser enviado em bytes
 * Parâmetro 4 - Tempo em milisegundos para esperar retorno do controlador de temperatura
 * Parâmetro 5 - Número de tentativas de envio da mensagem para o controlador de temperatura
 *
 * Parâmetro de saída:
 * Não tem.
*/
void AdicionaMsgFila_ControladorDeTemperatura(volatile unsigned char *_msg,
volatile unsigned char _tempo_espera_envio_msg, 
volatile unsigned char TamanhoPacoteModBus_bytes,
volatile signed int _TempoEsperaResposta_ControladorDeTemperatura_ms,
volatile char _TentativasDeEnvio_Resposta_ControladorDeTemperatura) {

	int i;
			
	//A fila não está cheia ?
	if(Num_msg_fila_ControladorDeTemperatura < TamanhoFilaModBus) {
		//Não, ainda tem espaço na Fila
		
		//Sinaliza que deve enviar informação para o controlador de temperatura
		ControlaComunicacao_Controlador_de_temperatura = 1;
		
		Fila_CT[Num_msg_fila_ControladorDeTemperatura].TempoEsperaResposta_ControladorDeTemperatura_ms = _TempoEsperaResposta_ControladorDeTemperatura_ms;
		
		Fila_CT[Num_msg_fila_ControladorDeTemperatura].NumeroDeTentativaEnvio_ControladorDeTemperatura = _TentativasDeEnvio_Resposta_ControladorDeTemperatura;
		
		//Copia o tamanho do PACOTE para a fila 
		Fila_CT[Num_msg_fila_ControladorDeTemperatura].Tamanho_msg_ModBus_Bytes = TamanhoPacoteModBus_bytes;
									
		//Limpa vetor que receberá a mensagem a ser adicionada na fila
		for(i = 0 ; i < Tamanho_BufferModbus ; i++) Fila_CT[Num_msg_fila_ControladorDeTemperatura].msg[i] = (unsigned char) 0;
		
		//Adiciona mensagem 
		for(i = 0 ; i < Fila_CT[Num_msg_fila_ControladorDeTemperatura].Tamanho_msg_ModBus_Bytes ; i++) Fila_CT[Num_msg_fila_ControladorDeTemperatura].msg[i] = _msg[i]; 
		
		//Adiciona tempo de espera para enviar a mensagem 
		Fila_CT[Num_msg_fila_ControladorDeTemperatura].TempoEsperaEnvioMsg = _tempo_espera_envio_msg; //* copia valor
		
		//Contabiliza mensagens na fila para envio
		Num_msg_fila_ControladorDeTemperatura++;

	} else {
		//Fila cheia RETORNA ERRO
	}
}

/* Remove mensagem da fila de envio pelo MODBUS para controlador de temperatura
 * Parâmetros de entrada:
 * Parâmetro 1 - Número de mensagens a ser removida da fila
 *
 * Parâmetro de saída:
 * Não tem.
*/
void RemoveMsgFila_ControladorDeTemperatura(unsigned char c) {
	//Salva cópia de Num_msg_fil
	int i,x,
	    CopiaNum_msg_fila = Num_msg_fila_ControladorDeTemperatura, 
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
				
				//Copia tempo de espera da resposta anterior para atual
				Fila_CT[i].TempoEsperaResposta_ControladorDeTemperatura_ms = Fila_CT[i+1].TempoEsperaResposta_ControladorDeTemperatura_ms;
				//Limpa tempo de espera da resposta anterior
				Fila_CT[i+1].TempoEsperaResposta_ControladorDeTemperatura_ms = 0;

				//Copia número de tentativas de envio da mensagem anterior para atual
				Fila_CT[i].NumeroDeTentativaEnvio_ControladorDeTemperatura = Fila_CT[i+1].NumeroDeTentativaEnvio_ControladorDeTemperatura;
				//Limpa número de tentativas de envio anterior
				Fila_CT[i+1].NumeroDeTentativaEnvio_ControladorDeTemperatura = 0;
								
				//Copia tamanho do PACOTE da mensagem anterior para atual
				Fila_CT[i].Tamanho_msg_ModBus_Bytes = Fila_CT[i+1].Tamanho_msg_ModBus_Bytes;
				//Limpa tamanho do PACOTE anterior
				Fila_CT[i+1].Tamanho_msg_ModBus_Bytes = 0;
												
				//Copia tempo de espera para enviar mensagem anterior para atual
				Fila_CT[i].TempoEsperaEnvioMsg = Fila_CT[i+1].TempoEsperaEnvioMsg;
				//Limpa tempo de espera anterior
				Fila_CT[i+1].TempoEsperaEnvioMsg = 0;					
															
				//Limpa vetor com a mensagem atual para receber a mensagem anterior
				for(x = 0 ; x < Tamanho_BufferModbus ; x++) Fila_CT[i].msg[x] = 0;
					
				//Copia mensagem anterior para atual	
				for(x = 0 ; x < Fila_CT[i].Tamanho_msg_ModBus_Bytes ; x++) Fila_CT[i].msg[x] = Fila_CT[i+1].msg[x]; //0 <- 1									
			}				
		}
			
		//Atualiza contador de mensagens na fila
		Num_msg_fila_ControladorDeTemperatura-= i_controle_vezes;
	}	
}

/* Calcula CRC16 MODBUS
 * Parâmetro de entrada:
 * Parâmetro 1 - Informações a serem calculado o CRC
 * Parâmetro 2 - Tamanho ou quantidade de bytes a ser calculado o CRC
 *
 * Parâmetro de saída:
 * CRC calculado
 *
 * Referência: https://modbus.org/docs/Modbus_over_serial_line_V1_02.pdf
 */
unsigned int Calcula_CRC16_MODBUS (volatile unsigned char *Str, unsigned char Tamanho) {
	unsigned int CRC16 = 0xFFFF, poly = 0xA001;
	unsigned char i = 0, j = 0;

/*
1) Inicializa CRC16 com todos os bits em 1 0xFFFF
2) Faz OU exclusivo entre a mensagem e o CRC16
3) Verifica o bit 1 do CRC16.
   Se igual a 1 -> Desloca CRC16 1 bit a direita em seguida faz OU explusivo entre CRC16 e poly (0xA001)
   Se igual a 0 -> Desloca CRC16 1 bit
4) Repita o passo 3 por 8 vezes (8 bits)
*/		
	for(i = 0 ; i < Tamanho ; i++) {
		CRC16 ^= Str[i];	
		for(j = 0 ; j < 8 ; j++) {						
			if((CRC16 & 0x0001) == 1) {
				CRC16 >>= 1;
				CRC16 ^= poly;
			} else {
				CRC16 >>= 1;
			}
		}
	}

	return(CRC16);
}

/* Inicializa maquina de estados da comunicação com o controlador de temperatura
 * Parâmetros de entrada:
 * Parâmetro 1 = Estado inicial da maquina de estados
 *
 * Parâmetro de saída:
 * Não tem
 */
void InicializaMaquinaDeEstados_ControladorDeTemperatura(unsigned char EstadoInicial) {
	Estado_Comunicacao_ControladorDeTemperatura = EstadoInicial;
}
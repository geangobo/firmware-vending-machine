/*
 usart0.h - Arquivo header
 Escrito por André N. da Cruz, Dezembro 2021
 SmartMach.

 Versão: 01
 Dezembro/2021 - André Cruz
  - Vetor de interrupção do periférico USART0 (Leitura de informações)   
  - Envio de informações em fila FIFO com temporização
 */ 

#ifndef USART0_H_
#define USART0_H_

//Include
#include <avr/io.h>
#include <avr/interrupt.h>

#include "../main.h"

//Seleção entre USART e ModBus
#define Port_Selecao_USART_MODBUS					PORTC
#define Pino_Selecao_USART							4		//PC4 USART_MCP
#define Pino_Selecao_MODBUS							5		//PC5 USART_RS485

//Seleção de escrita e leitura do ModBus
#define Port_TX_RX_MODBUS							PORTD
#define Pino_TX_RX_MODBUS							3		//PD3 TX_EN

#define TamanhoFilaUSART0							6	//Mensagens
#define Tamanho_BufferUSART0						70//20	//Bytes

#define TamanhoFilaCompras							9	//Máximo de 8 itens + 1 byte quantidade de itens no inicio
#define TamanhoFilaStatusCompras					9	//Máximo de 9 itens (Para manter compatibilidade com otimização de código)

//Fila
struct f{
	//string a ser enviada para Mini PC
	volatile unsigned char msg[Tamanho_BufferUSART0];
	//Tempo em ms para enviar a string
	volatile unsigned char TempoEsperaEnvioMsg;
	volatile bool Flag_JaEnviouResposta;
	//true = espera resposta do Mini PC
	//false = Não espera resposta do Mini PC
	volatile bool EsperaRespostaMiniPc;
	//Tempo em ms de espera da resposta enviada pelo MiniPc
	volatile unsigned char TempoEsperaRespostaMiniPc;
	//Número de tentativas de envio da reposta para o Mini Pc
	volatile unsigned char TentativaEnvioRespostaMiniPc;
	//true = Fila em uso
	//false = Fila disponível
	volatile unsigned char Tamanho_msg_bytes;
};

//Define se envio sera pela USART ou MODBUS
#define _USART									0
#define _ModBus									1

//Protocolo de comunicação

//Temporização
#define TempoEsperaFimDoPacote_ms					9			//ms Descontar latência de 1 byte a 9600 bps
																//Tempo algoritmo espera para considerar o último byte do pacote
				
#define TempoReceberProximoPacote_ms				10			//ms NÃO Descontar latência de 1 byte a 9600 bps
																//Tempo algoritmo ignora informações recebidas 

#define TempoEsperaEnvioResposta_ms					11			//ms NÃO Descontar latência de 1 byte a 9600 bps
																//Tempo que o algoritmo espera para enviar a resposta ao mestre
																
#define TempoEsperaResposta_ms						50			//ms NÃO Descontar latência de 1 byte a 9600 bps
																//Tempo que o algoritmo aguarda pela resposta do mestre
																
//TempoEsperaResposta_ms + TempoEsperaTentativasEnvioResposta - (Numero de bytes * 1,041ms)
//1,41ms tempo para enviar 1 byte com baudrate de 9600bps
//120 = 165
#define TempoEsperaTentativasEnvioResposta			125															
#define NumeroTentativasEnvioRespostaPadrao			5

//LATÊNCIA DE +/- 8,4ms
//159 = 166ms
#define TempoRespostaIntegridade_ms					164//ms Tempo para responder ACK ou NACK
																//Tempo que o algoritmo aguarda para enviar resposta da integridade do último pacote recebido

//Caracteres constantes do protocolo rev. 1.1
#define Caractere_Inicio							0x10
#define Caractere_Fim								0x1E
#define ACK											0x06
#define NACK										0x25

//Indicação de inicio e fim da informação
#define CaracterInicio_Informacao					0x02
#define CaracterFim_Informacao						0x03

#define ProtocoloVersao								0x03

#define OK											0x02

#define TamanhoRX_ParametroMAXIMO					48	//Bytes
#define TamanhoRX_ParametroMINIMO					1	//bytes

//Comandos

//Comando ACIONAMENTO INDIVIDUAL DO ATUADOR
#define ComandoAcionaAtuador						0x11
#define TamanhoRX_ComandoAcionaAtuador				1		//Byte
#define TamanhoTX_ComandoAcionaAtuador				1		//Byte
#define Parametro_AtuadorMinimo						1
#define Parametro_AtuadorMaximo						72
//***************************************************************************************************************************************************

//Comando ACIONAMENTO DO RELÉ DA PORTA PRINCIPAL
#define ComandoAcionaPortaPrincipal					0x12
#define TamanhoRX_ComandoAcionaPortaPrincipal		1	//Byte
#define TamanhoTX_ComandoAcionaPortaPrincipal		1	//Byte
#define Parametro_FechaPortaPrincipal				0x01
#define Parametro_AbrePortaPrincipal				0x02
//***************************************************************************************************************************************************

//Comando ACIONAMENTO INDIVIDUAL DO ELEVADOR
#define ComandoMovimentaElevador					0x13
#define TamanhoRX_ComandoMovimentaElevador			1	//Byte
#define TamanhoTX_ComandoMovimentaElevador			1	//Byte
#define Parametro_SobeElevador						0x01
#define Parametro_DesceElevador						0x02
//***************************************************************************************************************************************************

//Comando LÊ O STATUS DA TRAVA DAS GAVETAS
#define ComandoVerificaTravaDasGavetas				0x14
#define TamanhoRX_ComandoVerificaTravaDasGavetas	0x01	//Byte
#define TamanhoTX_ComandoVerificaTravaDasGavetas	0x01	//Byte
#define Parametro_VerificaGavetas					0x01
#define Resposta_TravaDasGavetas_Fechada			0x01
#define Resposta_TravaDasGavetas_Aberta				0x02
//***************************************************************************************************************************************************

//Comando LÊ A POSIÇÃO ATUAL DO ELEVADOR
#define ComandoVerificaPosicaoElevador				0x15
#define VerificaPosicaoElevador						0x01
#define TamanhoRX_ComandoVerificaPosicaoElevador	1	//Byte
#define TamanhoTX_ComandoVerificaPosicaoElevador	1	//Bytes
#define PosicaoElevadorMinima						1
#define PosicaoElevadorMaxima						7
#define Offset_Retorno_PosicaoElevador				0x25
//***************************************************************************************************************************************************

//Não usado
#define ComandoDetectaProdutoNaBandeija				0x16
//***************************************************************************************************************************************************

//Comando LÊ TEMPERATURA INTERNA DA MÁQUINA. LEITURA INDEPENDENTE DO CONTROLADOR DE TEMPERATURA
#define	ComandoLeTemperatura						0x17
#define TamanhoRX_ComandoLeTemperatura				1	//Bytes
#define TamanhoTX_ComandoLeTemperatura_SensorDesconectado				1	//Bytes
#define TamanhoTX_ComandoLeTemperatura				7	//Bytes
#define	Parametro_LeTemperatura						0x01

#define TX_SensorDesconectado						0x03
#define TX_SensorConectado							0x02
//***************************************************************************************************************************************************

//Comando REALIZA COMPRA DE PRODUTO.
#define ComandoRequisitaCompraDeProduto						0x18
#define TamanhoRX_ComandoRequisitaCompraDeProduto			1 //Bytes
#define TamanhoRX_ComandoRequisitaCompraDeProdutoMinimo		0x01
#define TamanhoRX_ComandoRequisitaCompraDeProdutoMaximo		0x07
#define TamanhoTX_ComandoRequisitaCompraDeProduto			1	//byte
#define RequisitaProdutoMinimo								1
#define RequisitaProdutoMaximo								72
//***************************************************************************************************************************************************

//Comando REALIZA OU APENAS RETORNA RESULTADO DO ÚLTIMO AUTOTESTE
#define ComandoAutoTeste							0x19
#define TamanhoRX_ComandoAutoTeste					1	//Bytes
#define Parametro_RealizaAutoTeste						0x01
#define Parametro_RespondeResultadoDoUltimoAutoTeste	0x02
#define TamanhoTX_ComandoAutoTeste					14	//bytes
#define Resultado_AutoTesteParcial_OK				0x01
#define Resultado_AutoTesteParcial_Falha			0x02
#define Resultado_AutoTesteParcial_ElevadorDesabilitado	0x03

#define Resultado_SensorDePosicaoDoElevador_OK		0x01
#define Resultado_SensorDePosicaoDoElevador_Danificado		0x02
#define Resultado_SensorDePosicaoDoElevador_Desabilitado		0x03

#define TesteGeral_OK								0xC1
//***************************************************************************************************************************************************

//Comando CONFIGURA O TIPO DE ATUADOR (MOTOR MOLA, ESTEIRA, GANCHEIRA) PRESENTE EM CADA ATUADOR DA GAVETA
#define ComandoConfiguraTipoDosAtuadoresDaGaveta	0x1A
#define TamanhoRX_ComandoConfiguraTipoDosAtuadoresDaGaveta	0x0D //bytes
#define TamanhoTX_ComandoConfiguraTipoDosAtuadoresDaGaveta	1 //bytes
#define ConfiguraTipoDosAtuadoresDaGaveta_Ok		0x26
#define ConfiguraTipoDosAtuadoresDaGaveta_Falha		0x27
//***************************************************************************************************************************************************

//Comando LÊ CONFIGURAÇÃO DO TIPO DE ATUADOR (MOTOR MOLA, ESTEIRA, GANCHEIRA) PRESENTE EM CADA ATUADOR DA GAVETA
#define ComandoLeConfiguracaoDosAtuadoresDaGaveta	0x1B
#define TamanhoRX_ComandoLeConfiguracaoDosAtuadoresDaGaveta		0x01 //bytes
#define TamanhoTX_ComandoLeConfiguracaoDosAtuadoresDaGaveta		13 //bytes
//***************************************************************************************************************************************************

//Comando ACIONA SOLENOIDE DA PORTINHOLA 
#define ComandoAcionaPortinhola						0x1C
#define TamanhoRX_ComandoAcionaPortinhola			0x01	//bytes
#define AbrePortinhola								0x01	//bytes
#define FechaPortinhola								0x02
//***************************************************************************************************************************************************

//Comando LÊ INFORMAÇÕES DE IDENTIFICAÇÃO DA MÁQUINA (NÚMERO DE SÉRIE, MODELO, VERSÃO DE HARDWARE E FIRMWARE)
#define ComandoLeInformacoesDeIdentificacaoDaMaquina	0x1D
#define TamanhoRX_ComandoLeInformacoesDeIdentificacaoDaMaquina	0x01 //bytes
#define Parametro_LeInformacoesDeIdentificacaoDaMaquina	0x01
//***************************************************************************************************************************************************

//Comando CONFIGURA INFORMAÇÕES DE IDENTIFICAÇÃO DA MÁQUINA (NÚMERO DE SÉRIE, MODELO, VERSÃO DE HARDWARE E FIRMWARE)
#define ComandoConfiguraInformacoesDeIdentificacaoDaMaquina	0x1E
#define TamanhoRX_Minimo_ComandoConfiguraInformacoesDeIdentificacaoDaMaquina	12 //bytes
#define TamanhoRX_Maximo_ComandoConfiguraInformacoesDeIdentificacaoDaMaquina	58 //bytes
//***************************************************************************************************************************************************

//Comando CONFIGURA CONTROLADOR DE TEMPERATURA
#define ComandoConfiguraParametroDoControladorDeTemperatura	0x1F
#define Tamanho_Comando_Temperatura_ParametroDoControladorDeTemperatura	7	//bytes
#define Tamanho_Comando_Tempo_ParametroDoControladorDeTemperatura	5	//bytes
#define Tamanho_Comando_Buzzer_ParametroDoControladorDeTemperatura	2	//bytes

//Parametros de configuração ou leitura do CONTROLADOR DE TEMPERATURA
#define Parametro_SetPoint_C_ControladorDeTemperatura				0x26
#define Parametro_Histerese_C_ControladorDeTemperatura			0x27
#define Parametro_SetPointMinimo_C_ControladorDeTemperatura		0x28
#define Parametro_SetPointMaximo_C_ControladorDeTemperatura		0x29
#define Parametro_AlarmeTemperaturaAmbienteBaixa_C_ControladorDeTemperatura	0x2A
#define Parametro_AlarmeTemperaturaAmbienteAlta_C_ControladorDeTemperatura	0x2B
#define Parametro_IntervaloEntreDegelos_Minutos_ControladorDeTemperatura		0x2C
#define Parametro_TempoDeDuracaoMaximaDoDegelo_Minutos_ControladorDeTemperatura		0x2D
#define Parametro_TempoDeIndicacaoDeTemperaturaTravada_Minutos_ControladorDeTemperatura 	0x2E
#define Parametro_Buzzer_ControladorDeTemperatura		0x2F
//***************************************************************************************************************************************************

//Comando RESET NA PLACA CONTROLADORA
#define ComandoResetPlacaControladora				0x20
#define TamanhoRX_ComandoResetPlacaControladora		1	//Byte
#define ComandoReset								1
//***************************************************************************************************************************************************

//Comando PLACA CONTROLADORA RESPONDE SE ESTÁ ONLINE PARA MINIPC
#define ComandoVerificaStatusDaPlacaControladora	0x21
#define TamanhoRX_ComandoStatusPlacaControladora	1	//bytes
#define TamanhoTX_ComandoStatusPlacaControladora	1	//bytes
#define Parametro_VerificaStatusDaPlacaControladora	0x01
//***************************************************************************************************************************************************

//Comando LÊ STATUS INDIVIDUAL DOS ATUADORES DA GAVETA (CONECTADO, DESCONECTADO OU EM CURTO)
#define ComandoLeStatusDosAtuadoresDaGaveta		0x22
#define TamanhoRX_ComandoLeStatusDosAtuadoresDaGaveta		0x01 //bytes
#define TamanhoTX_ComandoLeStatusDosAtuadoresDaGaveta		0x0D //bytes
//***************************************************************************************************************************************************

//Comando LÊ STATUS DA PORTA PRINCIPAL
#define ComandoLeStatusDaPortaPrincipal				0x23
#define TamanhoRX_ComandoLeStatusDaPortaPrincipal	0x01
#define TamanhoTX_ComandoLeStatusDaPortaPrincipal	0x01
#define Parametro_LeStatusDaPortaPrincipal			0x01
#define PortaPrincipalFechada						0x02
#define PortaPrincipalAberta						0x03
//***************************************************************************************************************************************************

//Comando INICIA PROCESSO DE DEGELO IMEDIATAMENTE
#define ComandoRealizaDegeloImediato				0x24
#define TamanhoRX_ComandoRealizaDegeloImediato		1	//bytes
#define Parametro_RealizaDegeloImediato				0x01
//***************************************************************************************************************************************************

//COMANDO 0x25 NÃO UTILIZADO PARA EVITAR ERRO NA RECEPÇÃO DOS DADOS NO MIDDLEWARE

//Comando CONFIGURA SE ELEVADOR ESTÁ HABILITADO OU DESABILITADO
#define ComandoConfiguraElevador					0x26
#define TamanhoRX_ComandoLigaDesligaElevador		1	//bytes
#define Parametro_HabilitaElevador					0x02
#define Parametro_DesabilitaElevador				0x01								
//***************************************************************************************************************************************************

//Comando LÊ CONFIGURAÇÃO DO ELEVADOR (HABILITADO OU DESABILITADO)
#define ComandoLeConfiguracaoDoElevador				0x27
#define TamanhoRX_ComandoLeConfiguracaoDoElevador	1	//bytes
#define Parametro_LeConfiguracaoDoElevador			0x01
//***************************************************************************************************************************************************

//Comando CONFIGURA SENSOR DE QUEDA (HABILITADO OU DESABILITADO)
#define ComandoConfigura_SensorDeQueda				0x28
#define TamanhoRX_ComandoConfigura_SensorDeQueda	1	//bytes
#define Parametro_HabilitaSensorDeQueda				0x02
#define Parametro_DesabilitaSensorDeQueda			0x01
//***************************************************************************************************************************************************

//Comando LÊ CONFIGURAÇÃO DO SENSOR DE QUEDA (HABILITADO OU DESABILITADO)
#define ComandoLeConfiguracao_SensorDeQueda			0x29
#define TamanhoRX_ComandoLeConfiguracao_SensorDeQueda	1	//bytes
#define TamanhoTX_ComandoLeConfiguracao_SensorDeQueda	1	//bytes
#define Parametro_LeConfiguracao_SensorDeQueda			0x01
//***************************************************************************************************************************************************

//Comando LEITURA DA CONFIGURAÇÃO DO CONTROLADOR DE TEMPERATURA
#define ComandoLeConfiguracaoDoControladorDeTemperatura	0x2A
#define TamanhoRX_ComandoLeConfiguracaoDoControladorDeTemperatura	1 //bytes
#define TamanhoTX_ComandoLeConfiguracaoDoControladorDeTemperatura	7 //bytes
//***************************************************************************************************************************************************

void ConfiguraUSART0(unsigned long int Baudrate);
void EnviaStringUSART0(volatile unsigned char *str, unsigned char NumeroBytes, char c, bool SelecionaUSART_MODBUS);
unsigned int crc16_XMODEM_RX(volatile unsigned char *str, volatile unsigned int NumeroBytes);
unsigned int crc16_XMODEM_TX(volatile unsigned char *str, volatile unsigned int NumeroBytes);
void TrataUSART0 (void); 

//Fila do MiniPc
void InicializaFila_MiniPc(volatile unsigned char TamanhoMaxFila);
void AdicionaMsgFila_MiniPc(volatile unsigned char *_msg, volatile unsigned char _tempo_espera_envio_msg, volatile unsigned char Tamanho, volatile bool _espera_resposta_mini_pc, volatile char _tempo_espera_resposta_mini_pc, volatile char _tentativa_envio_resposta_mini_pc);
void RemoveMsgFila_MiniPc(unsigned char c);

unsigned int SelecionaRegistradorModbusDoControladorDeTemperatura(unsigned char Parametro_USART);
#endif /* USART0_H_ */
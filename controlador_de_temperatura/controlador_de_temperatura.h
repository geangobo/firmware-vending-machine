/*
 * controlador_de_temperatura.h
 *
 * Created: 06/01/2023 10:07:31
 *  Author: André
 */ 


#ifndef CONTROLADOR_DE_TEMPERATURA_H_
#define CONTROLADOR_DE_TEMPERATURA_H_

#include "../main.h"

#define DuracaoDoPulsoParaRealizarDegelo_ControladorDeTemperatura_s		3	

#define Tamanho_BufferModbus	16	//bytes
#define Tamanho_Buffer_ControladorDeTemperatura_Conversao	10

#define TempoEsperaParaReceberProximoByte_ModBus_ms	4	//ms
#define TempoEsperaParaReceberResposta_ControladorDeTemperatura_ms	200
#define TempoEsperaParaEnviarPacote_ControladorDeTemperatura_ms		150		
#define NumeroDeTentativas_EnvioComando_ControladorDeTemperatura	5

#define Escrita_ControladorDeTemperatura							1
#define Leitura_ControladorDeTemperatura							2

#define EnderecoModBus_ControladorDeTemperatura						0x01

#define Comprimento_Setpoint	2	//bytes
#define Comprimento_Histerese	2	//bytes
#define Comprimento_SetpointMinimo	2	//bytes
#define Comprimento_SetpointMaximo	2	//bytes
#define Comprimento_AlarmeTemperaturaAmbienteBaixa		2	//bytes
#define Comprimento_AlarmeTemperaturaAmbienteAlta		2	//bytes
#define Comprimento_IntervaloEntreDegelos		2	//bytes
#define Comprimento_TempoDeDuracaoMaximaDoDegelo	2	//bytes
#define Comprimento_TempoDeIndicacaoDeTemperaturaTravada	2 //bytes
#define Comprimento_ControlaBuzzer	2	//bytes

//Estados do Controle de temperatura (CT)
#define Estado_CT_Inicia_MODBUS				0
#define Estado_CT_EsperaResposta_MODBUS		1
#define Estado_CT_EnviaRespostaPela_UART	2

#define TamanhoFilaModBus					15

//Fila
struct f_CT{
	//string a ser enviada para Controlador de temperatura
	volatile unsigned char msg[Tamanho_BufferModbus];
	//Tempo em ms para enviar a string
	volatile unsigned char TempoEsperaEnvioMsg;
	
	volatile unsigned char Tamanho_msg_ModBus_Bytes;
	
	//Tempo em ms de espera da resposta enviada pelo MiniPc
	volatile signed int TempoEsperaResposta_ControladorDeTemperatura_ms;
	//Número de tentativas de envio da reposta para o Mini Pc
	volatile unsigned char NumeroDeTentativaEnvio_ControladorDeTemperatura;
};

enum Id_Comando_ControladorDeTemperatura {
	
	//ConfiguracaoCompleta_ControladorDeTemperatura = 1,
	Configura_ControladorDeTemperatura = 1,
	Le_Configuracao_ControladorDeTemperatura,

	LeConfiguracao_ControladorDeTemperatura,
	TC900Elog_ValidaComunicacaoCom_ControladorDeTemperatura,
	MT512Elog_ValidaComunicacaoCom_ControladorDeTemperatura	
};

enum Id_Informacao_ControladorDeTemperatura {
	Id_Informacao_SetPoint	= 1,
	Id_Informacao_Histerese,
	Id_Informacao_SetpointMinimo,
	Id_Informacao_SetpointMaximo,
	Id_Informacao_AlarmeTemperaturaAmbienteBaixa,
	Id_Informacao_AlarmeTemperaturaAmbienteAlta,
	Id_Informacao_IntervaloEntreDegelos,
	Id_Informacao_TempoDeDuracaoMaximaDoDegelo,
	Id_Informacao_TempoDeIndicacaoDeTemperaturaTravada_Minutos,
	Id_Informacao_ControlaBuzzer
};

//Comunicação MODBUS com controlador de temperatura
#define Modbus_Endereco_ControladorDeTemperatura	0x01

#define Modbus_EscritaUnicoRegistrador				0x06
#define Modbus_LeituraUnicoRegistrador				0x03

//1 - Setpoint
#define TC900Elog_RegistradorSetpoint_ControladorDeTemperatura	0x001F //Comando 31
#define MT512Elog_RegistradorSetpoint_ControladorDeTemperatura	0x0000 //Comando 1

//2 - Histerese
#define TC900Elog_ResgistradorHisterese_ControladorDeTemperatura	0x0002	//Comando 2
#define MT512Elog_ResgistradorHisterese_ControladorDeTemperatura	0x0004	//Comando 5

//3 - Setpoint minimo permitido que o usuário configure
#define TC900Elog_ResgistradorSetpointMinimo_ControladorDeTemperatura	0x0003	//Comando 3
#define MT512Elog_ResgistradorSetpointMinimo_ControladorDeTemperatura	0x0002	//Comando 3

//4 - Setpoint maximo permitido que o usuário configure
#define TC900Elog_ResgistradorSetpointMaximo_ControladorDeTemperatura	0x0004	//Comando 4
#define MT512Elog_ResgistradorSetpointMaximo_ControladorDeTemperatura	0x0003	//Comando 4

//5 - Alarme de temperatura ambiente baixa
#define TC900Elog_ResgistradorAlarmeTemperaturaAmbienteBaixa_ControladorDeTemperatura	0x0006	//Comando 6

//6 - Alarme de temperatura ambiente alta
#define TC900Elog_ResgistradorAlarmeTemperaturaAmbienteAlta_ControladorDeTemperatura	0x0007	//Comando 7

//7 - Intervalo entre degelos
#define TC900Elog_ResgistradorIntervaloEntreDegelos_ControladorDeTemperatura	0x0008	//Comando 8
#define MT512Elog_ResgistradorIntervaloEntreDegelos_ControladorDeTemperatura	0x0008	//Comando 9

//8 - Duração máxima do degelo
#define TC900Elog_ResgistradorTempoDeDuracaoMaximaDoDegelo_ControladorDeTemperatura	0x000E //Comando 14
#define MT512Elog_ResgistradorTempoDeDuracaoMaximaDoDegelo_ControladorDeTemperatura	0x0009 //Comando 10

//9 - Tempo que a indicação de temperatura se mantém travada durante o degelo
#define TC900Elog_ResgistradorTempoDeIndicacaoDeTemperaturaTravada_ControladorDeTemperatura	0x0011 //Comando 17

//10 - Controla buzzer do controlador
#define TC900Elog_ResgistradorControlaBuzzer_ControladorDeTemperatura	0x0038	//Comando 56

//Identificação do controlador de temperatura
#warning ALTERACAO 7 512ELOGV10
#define TC900Elog_RegistradorID_Fabricante_ControladorDeTemperatura	0x0071	//Comando 113
#define MT512ELog_RegistradorID_Fabricante_ControladorDeTemperatura	0x009F	//

#define Tamanho_RespostaIDFabricante_ControladorDeTemperatura	0x02

//Resposta MODBUS do TC900Elog ou MT512Elog
#define RX_MSB_ID_Fabricante_ControladorDeTemperatura				0x46
#define RX_LSB_ID_Fabricante_ControladorDeTemperatura				0x47

#define RX_MSB_CRC_ID_Fabricante_ControladorDeTemperatura			0xCA
#define RX_LSB_CRC_ID_Fabricante_ControladorDeTemperatura			0x16


//Identificação do Controlador de temperatura
#define ControladorDeTemperatura_TC900Elog_Conectado			0x01
#define ControladorDeTemperatura_MT512Elog_Conectado			0x02
#define ControladorDeTemperatura_Desconectado					0x03

//Resposta USART
#define Resposta_ControladorDeTemperatura_ConfiguracaoOK		0x01
#define Resposta_ControladorDeTemperatura_Desconectado			0x32
#define Resposta_ControladorDeTemperatura_ComandoInvalido		0x33
#define Resposta_ControladorDeTemperatura_DadoInvalido			0x34

//Protótipos de funções
void Comunicacao_ControladorDeTemperatura(void);
unsigned char ComparaInformacao_ControladorDeTemperatura(unsigned char _TamanhoDoPacote_ModBus_Bytes);

void InicializaFila_ControladorDeTemperatura(volatile unsigned char TamanhoMaxFila);
void AdicionaMsgFila_ControladorDeTemperatura(volatile unsigned char *_msg, volatile unsigned char _tempo_espera_envio_msg, volatile unsigned char TamanhoPacoteModBus_bytes, volatile signed int _TempoEsperaResposta_ControladorDeTemperatura_ms, volatile char _TentativasDeEnvio_Resposta_ControladorDeTemperatura);
void RemoveMsgFila_ControladorDeTemperatura(unsigned char c);

void InicializaMaquinaDeEstados_ControladorDeTemperatura(unsigned char EstadoInicial);
unsigned int Calcula_CRC16_MODBUS (volatile unsigned char *Str, unsigned char Tamanho);
#endif /* CONTROLADOR_DE_TEMPERATURA_H_ */
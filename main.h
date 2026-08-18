/*
 * main.h
 *
 * Created: 08/11/2021 22:22:01
 *  Author: André
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#define F_CPU 16000000UL  // 16 MHz Frequência do cristal em Hz

//Includes do compilador
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <math.h>

//Includes do projeto
#include "adc/adc.h"
#include "spi/spi.h"
#include "timer/timers.h"
#include "_74HC595/_74HC595.h"
#include "_74HC165/_74HC165.h"
#include "usart/usart0.h"
#include "eeprom/eeprom.h"
#include "autoteste/autoteste.h"
#include "atuadores/atuadores.h"
#include "sensor_de_queda/sensor_de_queda.h"
#include "sensores/sensores.h"
#include "auxiliar/auxiliar.h"
#include "controlador_de_temperatura/controlador_de_temperatura.h"

typedef struct {
	volatile unsigned char Estado;
	volatile signed int TempoProximoEstado;	
} MaquinaDeEstados;

union Saida {
	unsigned long int Expansor;
	struct {
		unsigned char MAIS_MOTOR_1:1;	//QA - Pino 15 
		unsigned char MAIS_MOTOR_3:1;	//QB - Pino 1
		unsigned char MAIS_MOTOR_5:1;	//QC - Pino 2
		unsigned char MAIS_MOTOR_6:1;	//QD - Pino 3
		unsigned char MAIS_MOTOR_2:1;	//QE - Pino 4
		unsigned char MAIS_MOTOR_4:1;   //QF - Pino 5
		unsigned char MENOS_MOTOR_1:1;	//QG - Pino 6
		unsigned char MENOS_MOTOR_2:1;	//QH - Pino 7
		unsigned char MENOS_MOTOR_3:1;	//QA - Pino 15
		unsigned char MENOS_MOTOR_5:1;	//QB - Pino 1
		unsigned char MENOS_MOTOR_4:1;	//QC - Pino 2
		unsigned char MENOS_MOTOR_6:1;	//QD - Pino 3
		unsigned char MENOS_MOTOR_7:1;	//QE - Pino 4
		unsigned char MENOS_MOTOR_8:1;	//QF - Pino 5
		unsigned char MENOS_MOTOR_9:1;	//QG - Pino 6
		unsigned char MENOS_MOTOR_10:1;	//QH - Pino 7
		unsigned char MENOS_MOTOR_11:1;	//QA - Pino 15
		unsigned char MENOS_MOTOR_12:1;	//QB - Pino 1		
		unsigned char TRAVA_MAGNETICA:1;//QC - Pino 2	
		unsigned char SOLENOIDE:1;		//QD - Pino 3
		unsigned char ID_ATUADORES:1;	//QE - Pino 4
		unsigned char SEL_ATUADOR:1;	//QF - Pino 5
		unsigned char DEGELO:1;			//QG - Pino 6
		unsigned char SENSOR_DE_QUEDA:1;//QF - Pino 7
	};
};

union Entrada {
	unsigned long int Expansor;
	struct {
		unsigned char PORTINHOLA:1;				//D0 - Pino 11
		unsigned char FIM_CURSO_SUPERIOR:1;		//D1 - Pino 12
		unsigned char FIM_CURSO_INFERIOR:1;		//D2 - Pino 13
		unsigned char FIM_CURSO_RESERVA:1;		//D3 - Pino 14
		unsigned char PORTA_PRINCIPAL:1;		//D4 - Pino 3
		unsigned char TRAVA_GAVETAS:1;			//D5 - Pino 4
		unsigned char NAO_USADO1:1;				//D6 - Pino 5
		unsigned char FUSIVEL_ATUADORES:1;		//D7 - Pino 6
		unsigned char ELEVADOR_P5:1;			//D0 - Pino 11
		unsigned char ELEVADOR_P6:1;			//D1 - Pino 12
		unsigned char NAO_USADO2:1;				//D2 - Pino 13
		unsigned char ELEVADOR_P7:1;			//D3 - Pino 14
		unsigned char ELEVADOR_P4:1;			//D4 - Pino 3
		unsigned char ELEVADOR_P3:1;			//D5 - Pino 4
		unsigned char ELEVADOR_P2:1;			//D6 - Pino 5
		unsigned char ELEVADOR_P1:1;			//D7 - Pino 6		
	};
};
	
//Pinagem do ATMega328p
//BUZZER
#define Config_Port_BUZZER	DDRB		
#define Port_BUZZER			PORTB
#define Pino_BUZZER			0

//Pinagem do Elevador
#define Config_Port_Elevador	DDRC
#define Port_Elevador			PORTC
#define Pino_Sobe_Elevador		1		//PC1 - Pino 24
#define Pino_Desce_Elevador		2		//PC2 - Pino 25

//Temporização
#define BaseTempo_TMR0			100		//us 
#define BaudRate_USART0			9600	//bps

//Tempo para tocar alarme de porta aberta
#define EsperaAlarmePortaAberta_s		120		//segundos 

//Ciclos do looping principal para acionar o led de status
#define CiclosLedStatusPlaca_On		1500	//+/- 10Hz
#define CiclosLedStatusPlaca_Off	8500

//Tempo para manter a solenoide ligada
#define EsperaSolenoideLigada_ms		30000	//mili segundos

//Maquina de estados
//Modos de operação maquina de estados principal
#define M_AutoTeste					1
#define M_Falha						2
#define M_AcionaAtuador				3
#define M_SolicitaCompra			4
#define M_AcionaPortaPrincipal		5
#define M_AcionaElevador			6
#define M_PosicaoElevador			7
#define M_LeituraTemperatura		8
#define M_Reset						9
#define M_Status					10
#define M_IdentificaAtuadores		12
#define M_Ocioso					13
#define M_ComunicandoCom_ControladorDeTemperatura					14

//Modos de operação maquina de estados acionamento do elevador
#define M_AE_Inicio					0
#define M_AE_MovimentaElevador		1

//Modos de operação maquina de estados solicita compra
#define M_SC_Verifica_FilaDeCompra	0
#define M_SC_ValidaHardware_Elevador			1

#define M_SC_VerificaStatus_Elevador	2
#define M_SC_VerificaStatus_SensorDePosicao	3
#define M_SC_VerificaStatus_SensorDeQueda	4
#define M_SC_Liga_SensorDeQueda	5
#define M_SC_Testa_SensorDeQueda	6
#define M_SC_Aciona_Elevador	7

#define M_SC_Aciona_Atuador		8

#define M_SC_Valida_Entrega		9
#define M_SC_Entrega_Compra		10
#define M_SC_Proximo_ItemFilaDeCompra	11

#define M_SC_Responde_StatusDaCompra			12

#define M_SC_Valida_SensorMagnetico_Esteira	13

#define M_SC_Aciona_Solenoide	14

#define M_SC_Volta_Para_Repouso		15
#define M_SC_Entrega_ElevadorDesabilitado		16

#define M_SC_ValidaHardware_Atuador				56
#define M_SC_ValidaHardware_SensorDeQueda		57
#define M_SC_ValidaHardware_Le_SensorDeQueda	58
#define M_SC_Avalia_Hardware					55
#define M_SC_MovimentaElevadorNaGaveta			59

#define M_SC_Le_SensorDeQueda					60
#define M_SC_Avalia_Compra						61
#define M_SC_MovimentaElevadorNaEntrega			62

#define M_SC_MovimentaElevadorParaRepouso		63

#define M_SC_Espera_Solenoide_Acionada	64


#define Offset_Atuador_Solicita_Compra	0	//0x01 e 0x02 usado na confirmação da entrega
#define Offset_Elevador_Solicita_Compra	7	//
#define Offset_SensorDeQueda_Solicita_Compra	0x38	//

//Protótipos de funções
void ConfiguraIOs(void);
void InicializaMaquinaDeEstados_Principal(unsigned char EstadoInicial);
void InicializaMaquinaDeEstados_AUX_AcionaElevador(unsigned char EstadoInicial);
void InicializaMaquinaDeEstados_AUX_SolicitaCompra(unsigned char EstadoInicial);
void InicializaMaquinaDeEstados_AUX_Autoteste(unsigned char EstadoInicial);

#endif /* MAIN_H_ */
/*
 * sensores.h
 *
 * Created: 19/12/2021 16:58:29
 *  Author: André
 */ 


#ifndef SENSORES_H_
#define SENSORES_H_

#include "../main.h"

//cada ciclo +/- 44us
#define DEBOUNCE_SENSORPOSICAOELEVADOR_CICLOS   10
#define NUMERO_BORDASDEDESCIDA_SENSORDEPOSICAODOELEVADOR    2

//Tensão de referência do ADC em Volts, pino 18 AVCC
#define VrefADC								3.5

//Resolução do ADC
#define ResolucaoADC						1024.0		//2^10 = 1024
#define TensaoPorBit						(VrefADC/ResolucaoADC)	//3,5/1024 = 0,00341V = 3,41mV

//Coeficientes do NTC 
#define CoeficienteA						0.0011312
#define CoeficienteB						0.0002337
#define CoeficienteC						0.00000008863

//Resistor do divisor de tensão para ler o NTC
#define ResistenciaDivisorNTC_Ohms			10000.0	//Ohms

#define ZeroKelvinEmCelcius					-273.15

//Limita a temperatura mínima
#define TemperaturaMinima_C					-40.0	//°C
//Limita a temperatura máxima
#define TemperaturaMaxima_C					90.0	//°C

//Leitura da esteira
#define PortLeitura_LeituraEsteira		PIND
#define	Config_Port_LeituraEsteira		DDRD
#define Port_LeituraEsteira				PORTD
#define Pino_LeituraEsteira				5		//PD5 PINO 9

//Leitura dos fim de curso
#define TempoParaConsiderar_FimDeCurso_Acionado_ms  30

//Leitura da trava das gavetas
#define TempoParaConsiderar_TravaDasGavetas_Aberta_ms   100

//Protótipos de funções
unsigned char PosicaoAtualElevador (void);
unsigned char Verifica_Posicao_Elevador(unsigned char Posicao_Elevador_Desejada);
unsigned char Valida_SensorDePosicaoDoElevador(signed char _p);
float CalculaTemperatura(void);
unsigned char VerificaPortaPrincipal (void);
void ConfiguraLeituraEsteira(void);

void Le_Entradas_Com_Filtro(unsigned char TempoAmostragem_ms);

#endif /* SENSORES_H_ */
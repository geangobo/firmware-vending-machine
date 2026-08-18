/*
 * adc.h
 *
 * Created: 07/06/2021 22:34:32
 *  Author: André
 */ 


#ifndef ADC_H_
#define ADC_H_

//Include
#include "../main.h"

//Defines
//Referência do ADC
#define ReferenciaADC_PinoAREF				0b00000000
#define ReferenciaADC_PinoAVCC_Cap_AREF		0b01000000
#define Referencia_Interna1V1_Cap_AREF		0b11000000

//Canais de leitura do ADC
#define CanalADC_IElevador		0			//PC0 - Canal 0 do ADC
#define CanalADC_NTC			6			//ADC6
#define CanalADC_IAtuador		7			//ADC7
#define CanalADC_VAtuador		3			//PC3 - Canal 3 do ADC

#define AmostrasMediaADC		4

//Temporização
//Usa 300us 
//100us leitura do canal0 (I_Elevador)
//100us leitura do canal3 (V_Atuador)
//100us leitura do canal 7 (I_Atuador) 
//Cada canal é lido a cada 3 ciclos do Timer0

//ciclos da interrupção do ADC ~300us
#define  TempoADC_IdentificaCurto_Atuador					600		//600 * 0,0003 = 0,18s

#define  TempoADC_IdentificaCurto_Elevador					1666	//1666 * 0,0003 = 0,5s

//Indentificação do ATUADOR EM CURTO

//Valor de referência para curto

//Valor ADC para considerar curto no atuador
//3,5V (Tensão de referência do ADC) / 1024 (Resolução do ADC) = 0,0034V / bit
//Corrente máxima para o acionamento de 3 atuadores simultâneos = 0,26A
//Aplica-se um fator de segurança de 20% -> 0,26A * 1,2 = 0,312A

//Tensão no Shunt = Resistência do Shunt * Corrente no Atuador = 1R * 0,312 = 0,312V
//Tensão no ADC = Tensão no Shunt * Ganho do Amp Op = 0,312V * 10 = 3,12V

//Valor do ADC = Tensão no ADC / Tensão/bit = 3,12V / 0,0034V = 917

#define ValorADC_IdentificaCurto_Atuador					917


//Valor ADC para considerar curto no elevador
//3,5V (Tensão de referência do ADC) / 1024 (Resolução do ADC) = 0,0034V / bit
//Para corrente máxima = 7A 

//Tensão no Shunt = Resistência do Shunt * Corrente máxima = 0,033R * 7 = 0,231V 
//Tensão no ADC = Tensão no Shunt * Ganho do Amp Op = 0,231V * 12 = 2,772V

//Valor do ADC = Tensão no ADC / Tensão/bit = 2,772V / 0,0034V = 815

#define ValorADC_IdentificaCurto_Elevador					815		//7A


//Valor ADC para validar leitura do +24V
//3,5V (Tensão de referência do ADC) / 1024 (Resolução do ADC) = 0,0034V / bit
//Divisor de tensão 15K (R129) e 2,2K (R128) = 0,127
//24V * 0,127 = 3,048V
//5V * 0,127 = 0,635V
//Valor do ADC = Tensão no ADC / Tensão/bit = 0,635/0,0034 = 186
//Aplica fator de +/- 25% 
//Valor minimo = 0,635V - (0,635V * 0,25) = 0,635V-0,158V = 0,477V >>> ADC 0,477/0,0034 = 140
//Valor máximo = 0,635 + (0,635 * 0,25) = 0,635 + 0,158 = 0,793V >>> ADC 0,793/0,0034 = 233
#define ValorADCMinimo_ValidaFonteDeCorrente_5V         140
#define ValorADCMaximo_ValidaFonteDeCorrente_5V         233
 
//Protótipos de funções
void ConfiguraADC(unsigned char refADC);
float LeTemperaturaNtc(void);

#endif /* ADC_H_ */
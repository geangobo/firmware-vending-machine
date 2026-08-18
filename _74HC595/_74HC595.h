/*
 * CI74HC595.h
 *
 * Created: 14/12/2021 17:21:26
 *  Author: André
 */ 


#ifndef _74HC595_H_
#define _74HC595_H_

//Include
#include "../main.h"

//Pinagem
//Pino Enable 74HC595
#define Config_Port_OE_74HC595	DDRD
#define Port_OE_74HC595			PORTD
#define Pino_OE_74HC595			4		//PD4 - Pino 2

void Escreve74HC595(unsigned long int Dado);


#endif /* 74HC595_H_ */
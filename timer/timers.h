/*
 * timers.h
 *
 * Created: 05/08/2021 07:29:39
 *  Author: André Cruz
 */ 


#ifndef TIMERS_H_
#define TIMERS_H_

//Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../main.h"

//Defines
//Preescaler do timer 0
#define		PreescalerTimer0			64

#define BaseTempo_1ms				10			//0,0001 * 10 = 0,001 = 1ms
#define BaseTempo_1s				10000		//0,0001 * 10000 = 1 = 1s

//Protótipos de funções
void InicializaTimer0(unsigned long int OverflowTimer0);

#endif /* TIMERS_H_ */
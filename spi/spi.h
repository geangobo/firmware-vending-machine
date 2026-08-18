/*
 * spi.h
 *
 * Created: 14/12/2021 16:47:16
 *  Author: André
 *
 * Versão 1.0 - 14/12/2021
 *				Criação do arquivo header
 *
 * Versão 1.1 - 18/12/2023
 *				Atualização na forma de utilizar os #define dos pinos utilizados na SPI
 
 */ 


#ifndef SPI_H_
#define SPI_H_

//Include
#include <avr/io.h>

#include "../main.h"

#define	Config_Port_SPI		DDRB
#define Port_SPI			PORTB
#define _CS_74HC165			PB1		//PB1 PINO13
#define _CS_74HC595			PB2		//PB2 PINO14
#define _MOSI				PB3		//PB3 PINO15
#define _MISO				PB4		//PB4 PINO16
#define _SCK				PB5		//PB5 PINO17

//Protótipos de função
void ConfiguraSPI(void);


#endif /* SPI_H_ */
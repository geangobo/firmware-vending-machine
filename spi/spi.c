/*
 * spi.c
 *
 * Created: 14/12/2021 16:46:59
 *  Author: André
 *
 * Versão 1.0 - 14/12/2021
 *				Criação do arquivo
 *
 * Versão 1.1 - 18/12/2023
 *				Atualização na forma e sequência de inicialização da SPI
 */ 

#include "../spi/spi.h"

/* Configura periférico SPI
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void ConfiguraSPI() {
	//Inicializa SPI	
	
	//Configura como saída
	DDRB |= (1<<_MOSI) | (1<<_CS_74HC595) | (1<<_SCK) | (1<<_CS_74HC165);
	
	//Habilita a SPI como master e clock de FOSC/16 SPR1=0 e SPR0=1
	SPCR0 = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}
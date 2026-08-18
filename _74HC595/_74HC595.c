/*
 * CI74HC595.c
 *
 * Created: 14/12/2021 17:20:58
 *  Author: André
 *
 * Versão 1.0 - 14/12/2021
 *				Criação da biblioteca
 *
 * Versão 1.1 - 18/12/2023
 *				Atualização na inicialização e maneira de enviar informações pela SPI
 */ 

#include "_74HC595.h"


/* Escreve nos três 74HC595 em sequência e no fim atualiza suas saídas.
 * Paramêtro de entrada:
 * Parâmetro 1 - Valor de cada um dos pinos do 74HC595 a ser escritos/atualizados.
 *
 * Parâmetro de saída:
 * Não tem
*/
void Escreve74HC595(unsigned long int Dado) {
	union {
		volatile unsigned long int Completo;
		volatile unsigned char bytes[4];
	} Separa_Bytes;
	
	//Muda SPI para ler os bits na borda de subida
	CLEAR_BIT(SPCR0,CPOL);	

	Separa_Bytes.Completo = Dado;

	//Coloca nível lógico BAIXO no Chip Select
	Port_SPI&= ~(1<<_CS_74HC595);
	
	//Escreve no terceiro 74HC595
	SPDR0 = Separa_Bytes.bytes[2];
	//Espera finaliza a transmissão
	while(!(SPSR0&(1<<SPIF)));

	//Escreve no segundo 74HC595	
	SPDR0 = Separa_Bytes.bytes[1];
	//Espera finaliza a transmissão
	while(!(SPSR0&(1<<SPIF)));

	//Escreve no primeiro 74HC595
	SPDR0 = Separa_Bytes.bytes[0];
	//Espera finaliza a transmissão
	while(!(SPSR0&(1<<SPIF)));
	
	//Da pulso no CS do 74HC595 (Atualiza saída)
	Port_SPI |= (1<<_CS_74HC595);
	Port_SPI &= ~(1<<_CS_74HC595);
}
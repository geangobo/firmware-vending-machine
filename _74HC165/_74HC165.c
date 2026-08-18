/*
 * _74HC165.c
 *
 * Created: 14/12/2021 18:55:04
 *  Author: André
 */ 

#include "_74HC165.h"

/* Lê os dois 74HC165
 * ATENÇÃO: O 74HC165 atualiza sua saída serial (SER - pino 14) a cada borda de subida do clock (SRCLK - pino 11)
 *			para realizar a leitura corretamente é necessário configurar o uC para realizar a leitura (MISO) na borda
 *			de descida do clock setando o registrador SPCR, CPOL bit 3 antes de realizar uma leitura e 
 *			limpando o registrador SPCR, CPOL para retornar a SPI para trabalhar na borda de subida.
 *
 * Parâmetro de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Valor lido dos dois 74HC165
*/
unsigned int Le74HC165 (void) {
	union {
		volatile unsigned int Completo;
		volatile unsigned char bytes[2];
	} Separa_Bytes;
	
	//Muda SPI para ler os bits na borda de descida
	SET_BIT(SPCR0,CPOL);
		
	//Gera borda de subida no CS do 74HC165 (Lê entradas)
	//Baixo
	Port_SPI &= ~(1<<_CS_74HC165);
	Port_SPI |= (1<<_CS_74HC165);

//LEITURA DO 74HC165
	//Escreve no barramento
	SPDR0 = 0x00;
	//Espera finaliza a transmissão
	while(!(SPSR0&(1<<SPIF)));

	//Lê segundo 74HC165 - U6
	Separa_Bytes.bytes[0] = SPDR0;

	//Escreve no barramento
	SPDR0 = 0x00;
	//Espera finaliza a transmissão
	while(!(SPSR0&(1<<SPIF)));

	//Lê primeiro 74HC165 - U9
	Separa_Bytes.bytes[1] = SPDR0;

	//Coloca pino CS do 74HC165 em nível lógico baixo para desabilitar entrada de CLK em CLK e CLK INB
	Port_SPI &= ~(1<<_CS_74HC165);

	//Muda SPI para ler os bits na borda de subida
	CLEAR_BIT(SPCR0,CPOL);			
	
	return(Separa_Bytes.Completo);
}
/*
 eeprom.h - Código header
 Escrito por André N. da Cruz, Junho 2021

 Versão: 01
 Junho/2021 - André Cruz
 */ 

#ifndef EEPROM_H_
#define EEPROM_H_

//Include
#include <avr/io.h>

#include "../auxiliar/auxiliar.h"

//EEPROM
//Inicialização da EEPROM
#define Comprimento_Inicializacao									1	//Bytes
#define	End_EEPROM_Verifica_inicializacao							0	//Endereço de verificação de inicialização da EEPROM
#define EEPROM_Nao_inicializada										0xFF
#define EEPROM_Inicializada											0x33

//Número de série
#define Comprimento_Numero_de_serie									21	//Bytes
#define End_EEPROM_Numero_de_serie									End_EEPROM_Verifica_inicializacao + Comprimento_Inicializacao //0 + 1 Endereço 1

//Modelo
#define Comprimento_Modelo											21	//Bytes
#define End_EEPROM_Modelo											End_EEPROM_Numero_de_serie + Comprimento_Numero_de_serie	//1 + 21 Endereço 22

//Versão de hardware
#define Comprimento_Versao_de_Hardware								6	//Bytes
#define End_EEPROM_Versao_de_Hardware								End_EEPROM_Modelo + Comprimento_Modelo	//22 + 21 Endereço 43

//Versão de firmware
#define Comprimento_Versao_de_Firmware								6	//Bytes
#define End_EEPROM_Versao_de_Firmware								End_EEPROM_Versao_de_Hardware + Comprimento_Versao_de_Hardware //43 + 6 Endereço 49

//Elevador
#define Comprimento_Elevador_Ligado_Ou_Desligado					1	//Bytes
#define	End_EEPROM_Elevador_Ligado_Ou_Desligado						End_EEPROM_Versao_de_Firmware + Comprimento_Versao_de_Firmware //49 + 1 Endereço 50
#define Elevador_Ligado_Ou_Desligado_PADRAO							0	//Elevador desligado

//Tipo de atuador
#define Comprimento_Tipo_De_Atuador									72	//Bytes
#define End_EEPROM_Tipo_De_Atuador									End_EEPROM_Elevador_Ligado_Ou_Desligado+Comprimento_Elevador_Ligado_Ou_Desligado //50 + 1 Endereço 51

//Sensor de queda
#define Comprimento_SensorDeQueda_Ligado_Ou_Desligado               1   //Byte
#define End_EEPROM_SensorDeQueda_Ligado_Ou_Desligado                End_EEPROM_Tipo_De_Atuador + Comprimento_Tipo_De_Atuador
#define SensorDeQueda_Ligado_Ou_Desligado_PADRAO					1	//Sensor de queda ligado

//Protótipos de funções
//Escrita
void Escreve_uchar_eeprom(unsigned int Endereco, unsigned char Dado);
void Escreve_uint_eeprom(unsigned int Endereco, unsigned int Dado);
void Escreve_ulongint_eeprom(unsigned int Endereco, volatile unsigned long int *Dado, unsigned char Tamanho);
//Leitura
unsigned char Le_uchar_eeprom(unsigned int Endereco);
unsigned int Le_uint_eeprom(unsigned int Endereco);
void Le_ulongint_eeprom(unsigned int Endereco, volatile unsigned long int *Dado, unsigned char Tamanho);

void InicializaEEPROM(void);

void Atualiza_TipoDeAtuador(void);
 
#endif /* EEPROM_H_ */
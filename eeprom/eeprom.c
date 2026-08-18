/*
 eeprom.c - Código fonte
 Escrito por André N. da Cruz, Agosto 2021

 Versão: 01
 Agosto/2021 - André Cruz
  - Manipula periférico EEPROM (Escrita e leitura de uchar, uint, ulongint) 
 */ 
//Include
#include "eeprom.h"

//Variáveis
union {
	unsigned char bytes[4];
	unsigned int u_int;
	unsigned long u_long_int;
}ManipulaBytes;	

extern volatile unsigned char Numero_de_Serie[Comprimento_Numero_de_serie];
extern volatile unsigned char Modelo[Comprimento_Modelo];
extern volatile unsigned char Versao_de_Hardware[Comprimento_Versao_de_Hardware];
extern volatile unsigned char Versao_de_Firmware[Comprimento_Versao_de_Firmware];
extern volatile bool LigaDesliga_Elevador;
extern volatile bool LigaDesliga_SensorDeQueda;

extern volatile unsigned char TipoDeAtuador[NumeroMaximoDeAtuadores+1];

/* Escreve unsigned char (1 byte) na EEPROM interna.
 * Parâmetro de entrada:
 * Parâmetro 1 - Posição da EEPROM (endereço) a ser escrita a informação.
 * Parâmetro 2 - Informação a ser escrita na EEPROM.
 *	
 * Parâmetro de saída:
 * Não tem.
*/
void Escreve_uchar_eeprom(unsigned int Endereco, unsigned char Dado) {
	if(Endereco > 1023) Endereco = 1023;
	
	//Espera terminar escrita
	while(EECR & (1<<EEPE));
	
	EEAR = Endereco;			//Endereço
	EEDR = Dado;				//Dado
	
	//Habilita escrita 
	EECR |= (1<<EEMPE);	
	EECR |= (1<<EEPE);	
}

/* Escreve unsigned int (2 bytes) na EEPROM interna.
 * Parâmetro de entrada:
 * Parâmetro 1 - Posição da EEPROM (endereço) a ser escrita a informação.
 * Parâmetro 2 - Informação a ser escrita na EEPROM.
 *
 * Parâmetro de saída:
 * Não tem.
 */
void Escreve_uint_eeprom(unsigned int Endereco, unsigned int Dado) {
	if(Endereco > 1023) Endereco = 1023;
	ManipulaBytes.u_long_int = 0;
	
	ManipulaBytes.u_int = Dado;
	Escreve_uchar_eeprom(Endereco, ManipulaBytes.bytes[1]);
	Escreve_uchar_eeprom(++Endereco, ManipulaBytes.bytes[0]);
}

/* Escreve unsigned long int (4 bytes) na EEPROM interna.
 * Parâmetro de entrada:
 * Parâmetro 1 - Posição da EEPROM (endereço) inicial de escrita.
 * Parâmetro 2 - Informação a ser escrita na EEPROM.
 * Parâmetro 3 - Tamanho ou quantidade de posições do unsigned long int (4 bytes) deve ser escritos na EEPROM.
 *
 * Parâmetro de saída:
 * Não tem.
 */
void Escreve_ulongint_eeprom(unsigned int Endereco, volatile unsigned long int *Dado, unsigned char Tamanho) {
	unsigned char Aux_i;
	if(Endereco > 1023) Endereco = 1023;
	for(Aux_i = 0 ; Aux_i < Tamanho ; Aux_i++) {
		ManipulaBytes.u_long_int = Dado[Aux_i];
		Escreve_uchar_eeprom(Endereco, ManipulaBytes.bytes[3]);
		Escreve_uchar_eeprom(++Endereco, ManipulaBytes.bytes[2]);
		Escreve_uchar_eeprom(++Endereco, ManipulaBytes.bytes[1]);
		Escreve_uchar_eeprom(++Endereco, ManipulaBytes.bytes[0]);	
		Endereco++;	
	}
}

/* Leitura de unsigned char (1 byte) da EEPROM interna.
 * Parâmetro de entrada:
 * Parâmetro 1 - Posição da EEPROM (endereço) a ser lida a informação.
 *
 * Parâmetro de saída:
 * Informação lida na EEPROM.
 */
unsigned char Le_uchar_eeprom(unsigned int Endereco) {
	
	//Esperar finalizar o ciclo de escrita
	while(EECR & (1<<EEPE));
	
	EEAR = Endereco;			//Endereço
	//Habilita leitura
	EECR |= (1<<EERE);
	
	return(EEDR);
}

/* Leitura de unsigned int (2 bytes) da EEPROM interna.
 * Parâmetro de entrada:
 * Parâmetro 1 - Posição da EEPROM (endereço) a ser lida a informação.
 *
 * Parâmetro de saída:
 * Informação lida na EEPROM.
 */
unsigned int Le_uint_eeprom(unsigned int Endereco) {
	ManipulaBytes.u_long_int = 0;
	ManipulaBytes.bytes[1] = Le_uchar_eeprom(Endereco);
	ManipulaBytes.bytes[0] = Le_uchar_eeprom(++Endereco);	
	return(ManipulaBytes.u_int);
}

/* Leitura de unsigned long int (4 bytes) da EEPROM interna.
 * Parâmetro de entrada:
 * Parâmetro 1 - Posição da EEPROM (endereço) inicial de litura da informação.
 * Parâmetro 2 - Ponteiro para variável que receberá a informação lida na EEPROM.
 * Parâmetro 3 - Tamanho ou quantidade de posições do unsigned long int (4 bytes) deve ser lidos na EEPROM.
 *
 * Parâmetro de saída:
 * Informação lida na EEPROM.
 */
void Le_ulongint_eeprom(unsigned int Endereco, volatile unsigned long int *Dado, unsigned char Tamanho) {
	unsigned char Aux_i;
	
	for(Aux_i = 0 ; Aux_i < Tamanho ; Aux_i++) {
		ManipulaBytes.u_long_int = 0;
		ManipulaBytes.bytes[3] = Le_uchar_eeprom(Endereco);
		ManipulaBytes.bytes[2] = Le_uchar_eeprom(++Endereco);
		ManipulaBytes.bytes[1] = Le_uchar_eeprom(++Endereco);
		ManipulaBytes.bytes[0] = Le_uchar_eeprom(++Endereco);	
		Endereco++;	
		Dado[Aux_i] = ManipulaBytes.u_long_int;
	}

	//return(ManipulaBytes.u_long_int);
}

/* Inicializa a EEPROM escrevendo informações básicas ao ligar a placa pela primeira vez como:
 * Numero de série, modelo, versão de hardware, firmware, status do elevador e tipo de atuador padrão.
 * Parâmetro de entrada:
 * Não tem.
 *
 * Parâmetro de saída:
 * Não tem.
 */
void InicializaEEPROM() {
	unsigned int i;
	
	//Zera toda memória EEPROM [PARA DESENVOLVIMENTO]
	/*for(i = 0 ; i < 1024 ; i++) {
		Escreve_uchar_eeprom(End_EEPROM_Verifica_inicializacao + i, 0xff);
	}*/

	//Verifica se EEPROM já foi incializada
	if(Le_uchar_eeprom(End_EEPROM_Verifica_inicializacao) != EEPROM_Inicializada) {
		//Não, inicializa
		
		//Número de série padrão 00000000000000000000
		LimpaVetor(Numero_de_Serie, Comprimento_Numero_de_serie);
		
		Numero_de_Serie[0] = '0';
		Numero_de_Serie[1] = '0';
		Numero_de_Serie[2] = '0';
		Numero_de_Serie[3] = '0';
		Numero_de_Serie[4] = '0';
		Numero_de_Serie[5] = '0';
		Numero_de_Serie[6] = '0';
		Numero_de_Serie[7] = '0';
		Numero_de_Serie[8] = '0';
		Numero_de_Serie[9] = '0';
		Numero_de_Serie[10] = '0';
		Numero_de_Serie[11] = '0';
		Numero_de_Serie[12] = '0';
		Numero_de_Serie[13] = '0';
		Numero_de_Serie[14] = '0';
		Numero_de_Serie[15] = '0';
		Numero_de_Serie[16] = '0';
		Numero_de_Serie[17] = '0';
		Numero_de_Serie[18] = '0';
		Numero_de_Serie[19] = '0';
		
		for(i = 0 ; i < Comprimento_Numero_de_serie ; i++) {
			Escreve_uchar_eeprom(End_EEPROM_Numero_de_serie + i, Numero_de_Serie[i]);
		}
		
		//Modelo padrão
		LimpaVetor(Modelo,Comprimento_Modelo);
		
		Modelo[0] = 'S';
		Modelo[1] = 'm';
		Modelo[2] = 'a';
		Modelo[3] = 'r';
		Modelo[4] = 't';
		Modelo[5] = 'm';
		Modelo[6] = 'a';
		Modelo[7] = 'c';
		Modelo[8] = 'h';
		
		for(i = 0 ; i < Comprimento_Modelo ; i++) {
			Escreve_uchar_eeprom(End_EEPROM_Modelo + i, Modelo[i]);
		}
		
		//Versão de Hardware
		LimpaVetor(Versao_de_Hardware,Comprimento_Versao_de_Hardware);
		
		Versao_de_Hardware[0] = '1';
		Versao_de_Hardware[1] = '.';
		Versao_de_Hardware[2] = '3';
		
		for(i = 0 ; i < Comprimento_Versao_de_Hardware ; i++) {
			Escreve_uchar_eeprom(End_EEPROM_Versao_de_Hardware + i, Versao_de_Hardware[i]);
		}	
		
		//Versão de Firmware
		LimpaVetor(Versao_de_Firmware, Comprimento_Versao_de_Firmware);
		
		Versao_de_Firmware[0] = '1';
		Versao_de_Firmware[1] = '.';
		Versao_de_Firmware[2] = '3';
		#warning ALTERACAO 6 VERSAO DO FIRMWARE
		Versao_de_Firmware[3] = '3';
		
		for(i = 0 ; i < Comprimento_Versao_de_Firmware ; i++) {
			Escreve_uchar_eeprom(End_EEPROM_Versao_de_Firmware + i, Versao_de_Firmware[i]);
		}	
		
		//Salva configuração padrão do Elevador DESLIGADO
		Escreve_uchar_eeprom(End_EEPROM_Elevador_Ligado_Ou_Desligado, Elevador_Ligado_Ou_Desligado_PADRAO);
		
		//Inicializa o tipo de atuador com o atuador padrão motor mola simples
		for(i = 1 ; i <= Comprimento_Tipo_De_Atuador ; i++) {
			//Atualiza o tipo de atuador
			TipoDeAtuador[i] = Motor_Mola_Simples;

			//Salva na EEPROM o tipo de atuador padrão
			Escreve_uchar_eeprom(End_EEPROM_Tipo_De_Atuador + i, TipoDeAtuador[i]);
		}
		
		//Salva configuração padrão do Sensor de queda LIGADO
		Escreve_uchar_eeprom(End_EEPROM_SensorDeQueda_Ligado_Ou_Desligado, SensorDeQueda_Ligado_Ou_Desligado_PADRAO);

		//Indica que EEPROM foi inicializada
		Escreve_uchar_eeprom(End_EEPROM_Verifica_inicializacao, EEPROM_Inicializada);
	}


	//Lê informações da EEPROM
	//Número de série
	for(i = 0 ; i < Comprimento_Numero_de_serie ; i++) {
		Numero_de_Serie[i] = Le_uchar_eeprom(End_EEPROM_Numero_de_serie + i);
	}
	//Modelo
	for(i = 0 ; i < Comprimento_Modelo ; i++) {
		Modelo[i] = Le_uchar_eeprom(End_EEPROM_Modelo + i);
	}
	//Versão de Hardware
	for(i = 0 ; i < Comprimento_Versao_de_Hardware ; i++) {
		 Versao_de_Hardware[i] = Le_uchar_eeprom(End_EEPROM_Versao_de_Hardware + i);
	}
	//Versão de Firmware
	for(i = 0 ; i < Comprimento_Versao_de_Firmware ; i++) {
		Versao_de_Firmware[i] = Le_uchar_eeprom(End_EEPROM_Versao_de_Firmware + i);
	}	
	
	//Lê o tipo de atuador da EEPROM
	Atualiza_TipoDeAtuador();

	//Elevador ligado ou desligado
	LigaDesliga_Elevador = Le_uchar_eeprom(End_EEPROM_Elevador_Ligado_Ou_Desligado);

	//Sensor de queda ligado ou desligado
	LigaDesliga_SensorDeQueda = Le_uchar_eeprom(End_EEPROM_SensorDeQueda_Ligado_Ou_Desligado);
}

void Atualiza_TipoDeAtuador(void) {
	unsigned int i;
	
	for(i = 1 ; i <= Comprimento_Tipo_De_Atuador ; i++) {
		TipoDeAtuador[i] = Le_uchar_eeprom(End_EEPROM_Tipo_De_Atuador + i);
	}	
}


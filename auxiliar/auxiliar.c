/*
 auxiliar.c - Código fonte
 Escrito por André N. da Cruz, Dezembro 2021
 SmartMach.

 Versão: 01
 Dezembro/2021 - André Cruz
  - Manipulação de vetores (Limpar, Verificar tamanho, Copia, inversão)
  - Conversão de formatos (String para int, Longint para String, Float para String, String para Float e potenciação)
 Versão: 02
 Julho/2023 - André Cruz
  - Alteração na função de conversão de Float para String para evitar arredondamento.
 */ 

//INCLUDE
#include "auxiliar.h"

//delay de uso geral
extern volatile signed long int DelayTimer0_ms;
extern volatile bool Flag_Delay;

//Union auxiliar para manipular bytes 
union {
	unsigned char bytes[4];
	unsigned int u_int;
	unsigned long u_long_int;
	float f;
}ManipulaBytes;

/* Limpa (escreve 0) no vetor passado por ponteiro a quantidade de bytes passada por parâmetro.
 * Parâmetros de entrada:
 * Parâmetro 1 - Ponteiro para vetor a ser limpo.
 * Parâmetro 2 - Quantidade de bytes ou posições do vetor a ser limpos. [0 a 255]
 *
 * Parâmetro de saída:
 * Não tem
*/
void LimpaVetor (volatile unsigned char *VetorLimpar, volatile unsigned char NumeroBytes) {
	unsigned char i;
	//Varre ponteiro limpando cada posição
	for(i = 0 ; i < NumeroBytes ; i++) {
		VetorLimpar[i] = 0;
	}
}

/* Limpa (escreve 0) na matriz passada por ponteiro a quantidade de bytes passada por parâmetro.
 * Parâmetros de entrada:
 * Parâmetro 1 - Ponteiro para matriz a ser limpa.
 * Parâmetro 2 - Quantidade de bytes ou posições da matriz a ser limpos. [0 a 255]
 *
 * Parâmetro de saída:
 * Não tem
*/
void LimpaMatriz (volatile unsigned char *p, volatile unsigned char NumeroBytes) {
	unsigned char i;
	//Varre ponteiro limpando cada posição
	for(i = 0 ; i < NumeroBytes ; i++) {
		*(p+i) = 0;
	}
}

/* Verifica ou conta o número de elementos do ponteiro até encontrar um caractere 0 ou NULL
 * Parâmetros de entrada:
 * Parâmetro 1 - Ponteiro para matriz a ser verificada
 *
 * Parâmetro de saída:
 * Quantidade de elementos presente na matriz.
*/
unsigned char VerificaTamanhoMatriz(volatile unsigned char *v) {
	unsigned char c = 1;
	//Conta elementos do vetor diferentes de 0 ou NULL
	while(*(v+c) != 0) {
		//Caso não tenha 0 na string não fica em loop
		if(++c == 255) break;			
	}
	return(c);
}

/* Verifica ou conta o número de elementos do ponteiro até encontrar um caractere 0 ou NULL
 * Parâmetros de entrada:
 * Parâmetro 1 - Ponteiro para o vetor a ser verificado
 *
 * Parâmetro de saída:
 * Quantidade de elementos presente no vetor.
*/
unsigned char VerificaTamanho(volatile unsigned char *v) {
	unsigned char c = 0;
	//Conta elementos do vetor diferentes de 0 ou NULL
	while(v[c] != 0) {
		//Caso não tenha 0 na string não fica em loop
		if(++c == 255) break;	
	}
	return(c);
}

/* Copia conteúdo de um vetor para outro vetor o numero de elementos ou tamanho especificado no parâmetro "NumeroBytes"
 * Parâmetro de entrada:
 * Parâmetro 1 - Ponteiro para o vetor a ser copiado
 * Parâmetro 2 - Ponteiro para vetor que receberá a cópia
 * Parâmetro 3 - Quantidade de bytes ou posições do vetor a ser copiado. [0 a 255]
 * 	
 * Parâmetro de saída:
 * Não tem
*/
void CopiaVetor(volatile unsigned char *VetorOriginal, volatile unsigned char *VetorCopia, volatile unsigned char NumeroBytes) {
	unsigned char i;
	for(i = 0 ; i < NumeroBytes ; i++) {
		VetorCopia[i] = VetorOriginal[i];
	}
}

/* Copia conteúdo de um vetor para uma matriz o numero de elementos ou tamanho especificado no parâmetro "NumeroBytes"
 * Parâmetro de entrada:
 * Parâmetro 1 - Ponteiro para o vetor a ser copiado
 * Parâmetro 2 - Ponteiro para matriz que receberá a cópia
 * Parâmetro 3 - Quantidade de bytes ou posições do vetor a ser copiado. [0 a 255]
 * 	
 * Parâmetro de saída:
 * Não tem
*/
void CopiaVetorParaMatriz(volatile unsigned char *Vetor, volatile unsigned char *Matriz, volatile unsigned char NumeroBytes) {
	unsigned char i;
	for(i = 0 ; i < NumeroBytes ; i++) {
		*(Matriz+i) = *(Vetor+i);
	}
}

/* Inverte elementos de uma string (vetor de char) o numero de elementos ou tamanho especificado no parâmetro "NumeroBytes"
 * Parâmetro de entrada:
 * Parâmetro 1 - Ponteiro para string a ser invertida
 * Parâmetro 2 - Quantidade de bytes ou posições da string a ser invertida. [0 a 255]
 * 	
 * Parâmetro de saída:
 * Não tem
*/
void InverteString(volatile unsigned char* str, volatile unsigned char NumeroBytes) {
	unsigned char i = 0, j = NumeroBytes - 1, temp;
	while (i < j) {
		//Copia valor da posição i para temp
		temp = str[i];
		//Copia valor da posição j para posição i
		str[i] = str[j];
		//Copia valor da posição i para posição j
		str[j] = temp;
		i++;
		j--;
	}
}


/* Converter um valor inteiro sem sinal em string.
 * Parâmetro de entrada:
 * Parâmetro 1 - Inteiro sem sinal a ser convertido para string.
 * Parâmetro 2 - Ponteiro para o vetor de char sem sinal que armazenará a string.
 * Parâmetro 3 - Número de digitos da string a ser armazenada no ponteiro (Parâmetro 2).		
 *
 * Parâmetro de saída:
 * Retorna inteiro com a quantidade de elementos presente no vetor str.
*/
int intToStr(unsigned int x, volatile unsigned char *str, unsigned int Digitos) {
	unsigned int i = 0;
	while (x) {
		str[i++] = (x % 10) + '0';
		x = x / 10;
	}
	
	while (i < Digitos)
	str[i++] = '0';
	
	InverteString(str, i);
	str[i] = '\0';
	return i;
}

/* Converter um valor inteiro com sinal em string.
 * Parâmetro de entrada:
 * Parâmetro 1 - Inteiro com sinal a ser convertido para string.
 * Parâmetro 2 - Ponteiro para o vetor de char sem sinal que armazenará a string.
 * Parâmetro 3 - Número de digitos da string a ser armazenada no ponteiro (Parâmetro 2).		
 *
 * Parâmetro de saída:
 * Retorna inteiro com a quantidade de elementos presente no vetor str.
*/
int s_intToStr(signed int x, volatile unsigned char *str, unsigned int Digitos) {
	unsigned int i = 0, aux_neg = 0;
	
	if(x < 0) {
		//Se número negativo
		aux_neg = 1;
		x *= -1;
	}
	
	while (x) {
		str[i++] = (x % 10) + '0';
		x = x / 10;
	}
	
	while (i < Digitos)
	str[i++] = '0';
	
	//Se número negativo adiciona o sinal
	if(aux_neg == 1) {
		str[Digitos-1] = '-';
	}
	
	InverteString(str, i);
	str[i] = '\0';
	return i;
}

/* Converter um valor inteiro longo em string.
 * Parâmetro de entrada:
 * Parâmetro 1 - Inteiro longo a ser convertido para string.
 * Parâmetro 2 - Ponteiro para o vetor de char sem sinal que armazenará a string.
 * Parâmetro 3 - Número de digitos da string a ser armazenada no ponteiro (Parâmetro 2).	
 * 
 * Parâmetro de saída:
 * Retorna inteiro com a quantidade de elementos presente no vetor str.
*/
int longintToStr(long int x, volatile unsigned char *str, unsigned int Digitos, unsigned char Sinal) {
	unsigned char i = 0;
	
	if(Sinal == NUMERO_NEGATIVO) {
		//Se número negativo
		x *= -1;
	} 
	
	//Converte de decimal para ascii
	while (x) {
		str[i++] = (x % 10) + '0';
		x = x / 10;
	}
	
	while (i < Digitos) 
		str[i++] = '0';
	
	if(Sinal != NUMERO_SEM_SINAL) {
		//Adiciona sinal a string
		if(Sinal == NUMERO_NEGATIVO) {
			//Número negativo
			str[i++] = '-';
		} else if(Sinal == NUMERO_POSITIVO) {
			//Número positivo
			str[i++] = '+';
		}
	}
	InverteString(str, i);
	str[i] = '\0';
	return i;
}

/* Converter um valor float em string.
 * Parâmetro de entrada:
 * Parâmetro 1 - float a ser convertido para string.
 * Parâmetro 2 - Ponteiro para o vetor de char sem sinal que armazenará a string.
 * Parâmetro 3 - Número de digitos da string a ser armazenada no ponteiro (Parâmetro 2).	
 * 
 * Parâmetro de saída:
 *	Não tem.
 *	
Referência
https://www.geeksforgeeks.org/convert-floating-point-number-string/	
*/
void FloatToStr(float x, volatile unsigned char* str, int NumeroDeDigito_ParteInteira, int NumeroDeDigito_ParteDecimal) {
	unsigned char Sinal = NUMERO_SEM_SINAL;
	float ParteDecimal = 0.0;
	long int ParteInteira = 0;

	//Armazena a parte inteira
	ParteInteira = (long int)x;
	
	//Armazena a parte decimal
	ParteDecimal = (x * pow2(10.0, NumeroDeDigito_ParteDecimal)) - (ParteInteira * pow2(10.0, NumeroDeDigito_ParteDecimal));

	//Seleciona o sinal
	if(x < 0) Sinal = NUMERO_NEGATIVO;
	else Sinal = NUMERO_POSITIVO;

	//Converte a parte inteira
	int i = longintToStr(ParteInteira, str, NumeroDeDigito_ParteInteira , Sinal);

	//Se número negativo garante parte decimal positiva
	if(ParteDecimal < 0) ParteDecimal *= -1.0;
	
	//Se deve utilizar o ponto decimal '.'
	if (NumeroDeDigito_ParteDecimal != 0) {
		//Adiciona o .
		str[i] = '.'; 				
		//Converte parte decimal
		longintToStr((long int)ParteDecimal, str + i + 1, NumeroDeDigito_ParteDecimal, NUMERO_SEM_SINAL);
	}
}

/* Converter uma string em float.
 * Parâmetro de entrada:
 * Parâmetro 1 - Ponteiro para vetor com a string a ser convertida.
 * Parâmetro 2 - Número de digitos da conversão
 * 	
 * Parâmetro de saída:
 * Valor passado pelo ponteiro com o numero de digitos desejado.
*/
float StrToFloat(volatile unsigned char *str, int Digitos) {
	unsigned char i = 0, Offset = 0, PosicaoPontoDecimal = 0, TamanhoString = 0, Sinal = NUMERO_SEM_SINAL;
	float ValorFloat = 0.0;
	
	//Verifica sinal do número
	if(str[0] == '-') {
		Offset = 1;
		Sinal = NUMERO_NEGATIVO;
	} else if(str[0] == '+') {
		Offset = 1;
		Sinal = NUMERO_POSITIVO;	
	} else Offset = 0;
	
	//Lê tamanho do vetor
	TamanhoString = VerificaTamanho(str);	
	
	//Vare vetor com string e busca ponto decimal
	for(i = 0 ; i < TamanhoString ; i++) {
		if(str[i] == '.') PosicaoPontoDecimal = (i);
	}

	//Verifica senão tem o ponto decimal
	if(PosicaoPontoDecimal == 0) {
		//Posiciona o ponto depois ultimo valor
		PosicaoPontoDecimal = TamanhoString;
		//Zera os digitos
		Digitos = 0;
	}

	for(i = (0 + Offset) ; i < (TamanhoString) ; i++) {	
		//Antes do ponto multiplica
		if(i < PosicaoPontoDecimal) {
			ValorFloat += (str[i] - 0x30) * pow2(10,((PosicaoPontoDecimal - 1) - i));
		}
		//Depois do ponto divide
		if(i > PosicaoPontoDecimal) {
			ValorFloat += (str[i] - 0x30) / pow2(10, (i-PosicaoPontoDecimal));
		}
	}
	//Se número negativo multiplica por -1
	if(Sinal == NUMERO_NEGATIVO) ValorFloat *=-1.0;
	return (ValorFloat);
}

/* Calcula potenciação da base elevada ao expoente.
 * Parâmetro de entrada:
 * Parâmetro 1 - Valor da base
 * Parâmetro 2 - Valor do expoente
 *	
 * Parâmetro de saída:
 * Potenciação da base elevada ao expoente.
*/
float pow2(float base, unsigned char expoente) {
	unsigned char i;
	float Resultado = (float) base;
	if(expoente == 0) {
		Resultado = 1.0;
	} else if(expoente > 1) {
		for(i = 1 ; i < expoente ; i++) {
			Resultado *= (float) base;
		}
	}	
	return ((float) Resultado);
}

/* Delay (Atraso) por instrução de milisegundos.
 * ATENÇÃO: ROTINA FORNECE ATRASO DE ATÉ 1 segundo.
 * Parâmetro de entrada:
 * Parâmetro 1 - Tempo que rotina deve esperar em milisegundos
 *	
 * Parâmetro de saída:
 * Não tem.
*/
void delayms (unsigned int long Atraso) {
	Flag_Delay = false;
	DelayTimer0_ms = Atraso;
	
	while((DelayTimer0_ms > 0) || (Flag_Delay == true));
}
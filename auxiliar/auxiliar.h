/*
 auxiliar.h - Código header
 Escrito por André N. da Cruz, Junho 2021

 Versão: 01
 Junho/2021 - André Cruz
  - Macro para manipular bits.
 */ 


#ifndef AUXILIAR_H_
#define AUXILIAR_H_

//Arquivos de include
#include "../main.h"

//Macro para manipular bits
#define SET_BIT(Registrador,Bit) Registrador |= (1<<Bit)
#define CLEAR_BIT(Registrador,Bit) Registrador &= ~(1<<Bit)

#define NUMERO_POSITIVO 1
#define NUMERO_NEGATIVO 2
#define NUMERO_SEM_SINAL 3

//Protótipos de funções
void LimpaVetor (volatile unsigned char *MatrizLimpar, volatile unsigned char NumeroBytes);
void LimpaMatriz (volatile unsigned char *p, volatile unsigned char NumeroBytes);
unsigned char VerificaTamanho(volatile unsigned char *v);
unsigned char VerificaTamanhoMatriz(volatile unsigned char *v);
void CopiaVetor(volatile unsigned char *VetorOriginal, volatile unsigned char *VetorCopia, volatile unsigned char NumeroBytes);
void CopiaVetorParaMatriz(volatile unsigned char *Vetor, volatile unsigned char *Matriz, volatile unsigned char NumeroBytes); 
void InverteString(volatile unsigned char* str, volatile unsigned char NumeroBytes);
int intToStr(unsigned int x, volatile unsigned char *str, unsigned int Digitos);
int s_intToStr(signed int x, volatile unsigned char *str, unsigned int Digitos); 
int longintToStr(long int x, volatile unsigned char *str, unsigned int Digitos, unsigned char Sinal);
void FloatToStr(float x, volatile unsigned char* str, int NumeroDeDigito_ParteInteira, int NumeroDeDigito_ParteDecimal);
float StrToFloat(volatile unsigned char *str, int Digitos);
float pow2(float base, unsigned char expoente);
void delayms (unsigned int long Atraso);
#endif /* AUXILIAR_H_ */
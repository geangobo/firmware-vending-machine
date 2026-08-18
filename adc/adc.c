/*
 * adc.c
 *
 * Created: 07/06/2021 22:34:19
 *  Author: André
 */ 

//Include
#include "adc.h"

//Union com saídas do expansor
extern union Saida Saida_74HC595;

//Variáveis
//Média móvel
volatile unsigned int Canal0ADC[AmostrasMediaADC] = {0,0,0,0}, Canal0NPontos = 0, Canal0SomaAnterior = 0, Canal0Media = 0;	//I_Elevador
volatile unsigned int Canal3ADC[AmostrasMediaADC] = {0,0,0,0}, Canal3NPontos = 0, Canal3SomaAnterior = 0, Canal3Media = 0;	//V_Atuadores
volatile unsigned int Canal6ADC[AmostrasMediaADC] = {0,0,0,0}, Canal6NPontos = 0, Canal6SomaAnterior = 0, Canal6Media = 0;	//NTC
volatile unsigned int Canal7ADC[AmostrasMediaADC] = {0,0,0,0}, Canal7NPontos = 0, Canal7SomaAnterior = 0, Canal7Media = 0;	//I_Atuadores

//Identificação do atuador
volatile int	NumeroAmostrasADCAtuadores_Corrente = 0,		//Número de leituras do ADC para identificar a CORRENTE do atuador
				NumeroAmostrasADCAtuadores_Tensao = 0,			//Número de leituras do ADC para identificar a TENSÃO do atuador
				ValorPicoMedioAtuadores_ADC_Corrente = 0,		//Valor de pico médio da CORRENTE para identificar atuador
				ValorPicoMedioAtuadores_ADC_Tensao = 0;			//Valor de pico médio da TENSÃO para identificar atuador

//Temporização identificação de curto
volatile int ContadorIdentificarCurto_Atuador = TempoADC_IdentificaCurto_Atuador,
				ContadorIdentificarCurto_Elevador = TempoADC_IdentificaCurto_Elevador;

extern volatile bool CurtoAtuador;
extern volatile bool CurtoElevador;

//Leitura da temperatura
extern volatile bool Flag_LeituraNTC;
//Temporização para leitura da temperatura
volatile signed int Conta_Leitura_Temperatura = 10000;	//0,0001 * 10000 ~ 1s

//Inicia leitura pelo canal 6 (NTC)
volatile unsigned char Controla_Canal = CanalADC_NTC;


/* Configura o modo de funcionamento do conversor analógico digital (ADC)
 * Parâmetros de entrada:
 * Parâmetro 1 - Qual referência será utilizada pelo conversor ADC
 *
 * Parâmetros de saída:
 * Não tem
*/
void ConfiguraADC (unsigned char refADC) {
	
	//Configura referência do ADC
	ADMUX = refADC;
	
	//Seleciona o canal a ser lido
	ADMUX = (refADC & 0b11100000)|(Controla_Canal & 0x0F);
		
	//Liga ADC 
	//Inicia a conversão
	//Habilita o auto trigger
	//Habilita interrupção de fim de conversão
	//Preescaler do ADC FOSC/16 = 2us/ciclo +/- 30us
	ADCSRA = 0b11101101;
	
	//Auto trigger comparação do canal A do TMR0
	ADCSRB = 0b00000011;
}
/*
Sequência de leitura dos canais do ADC
1) LÊ NTC (A CADA 1 SEGUNDO)
2) LÊ CORRENTE NO ATUADOR
3) LÊ CORRENTE ELEVADOR >>> VOLTA PARA 2) LÊ CORRENTE NO ATUADOR
*/

ISR(ADC_vect) {
	//Interrupção do fim da conversão do ADC a cada 100us
	
	//Limpa flag
	CLEAR_BIT(ADCSRA,4);
	
//Calcula média móvel dos canais ADC		
//Roda em 38uS	
	switch(Controla_Canal) {
		case CanalADC_IElevador:

			//Coloca no vetor a amostra atual
			Canal0ADC[Canal0NPontos] = ADC;
			
			//Soma amostra atual para calcular a média
			Canal0SomaAnterior += ADC;
			
			//Leu os pontos necessários para começar o cálculo da média?
			if(Canal0NPontos == (AmostrasMediaADC - 1)) {
				//Sim
				
				//Calcula a média
				Canal0Media = Canal0SomaAnterior/(Canal0NPontos+1);
			
				//Retira primeiro ponto do vetor
				Canal0SomaAnterior -= Canal0ADC[0];
				//Copia amostras anteriores para inserir a próxima amostra
				Canal0ADC[0] = Canal0ADC[1];
				Canal0ADC[1] = Canal0ADC[2];
				Canal0ADC[2] = Canal0ADC[3];
			} else {
				Canal0NPontos++;
			}

			//Detecta curto no elevador
			//Se corrente no atuador acima do valor definido POSSÍVEL CURTO por mais de X ms (ContadorIdentificarCurto_Elevador)
			
			//Elevador sem carga pico de corrente dura +/- 100ms
						
			if(Canal0Media >= ValorADC_IdentificaCurto_Elevador) {	
				//#warning AJUSTE TEMPO E CORRENTE DE CURTO DO ELEVADOR
				
				
				//Passou o tempo para identificar o curto no elevador?
				if(ContadorIdentificarCurto_Elevador > 1) {
					//Não, continua contando 
					ContadorIdentificarCurto_Elevador--;					
				} else if(ContadorIdentificarCurto_Elevador == 1) {
					//Sim				
					//IDENTIFICOU CURTO NO ELEVADOR

					//Zera temporizador
					ContadorIdentificarCurto_Elevador = 0;
					
					//Sinaliza curto no elevador				
					CurtoElevador = true;											
				}				
			} else {
				//Se corrente abaixo do valor definido reinicia temporizador
				ContadorIdentificarCurto_Elevador = TempoADC_IdentificaCurto_Elevador;
			}

			//Próximo canal do ADC a ser lido
			Controla_Canal = CanalADC_VAtuador;
		break;
		
		case CanalADC_VAtuador:
				
			//Coloca no vetor a amostra atual
			Canal3ADC[Canal3NPontos] = ADC;
			
			//Soma amostra atual para calcular a média
			Canal3SomaAnterior += ADC;
			
			//Leu os pontos necessários para começar o cálculo da média?
			if(Canal3NPontos == (AmostrasMediaADC - 1)) {
				//Sim
				
				//Calcula a média
				Canal3Media = Canal3SomaAnterior/(Canal3NPontos+1);
				
				//Retira primeiro ponto do vetor
				Canal3SomaAnterior -= Canal3ADC[0];
				//Copia amostras anteriores para inserir a próxima amostra
				Canal3ADC[0] = Canal3ADC[1];
				Canal3ADC[1] = Canal3ADC[2];
				Canal3ADC[2] = Canal3ADC[3];
			} else {
				Canal3NPontos++;
			}

			//Identificação do atuador conectado
			//Lê amostras e detecta maior valor médio para
			if(NumeroAmostrasADCAtuadores_Tensao >= 1) {
				NumeroAmostrasADCAtuadores_Tensao++;
				if(Canal3Media > ValorPicoMedioAtuadores_ADC_Tensao) {
					ValorPicoMedioAtuadores_ADC_Tensao = Canal3Media;
				}
			} 	
						
			//Próximo canal do ADC a ser lido
			Controla_Canal = CanalADC_IAtuador;		
		break;
		
		case CanalADC_NTC:
			
			//Coloca no vetor a amostra atual
			Canal6ADC[Canal6NPontos] = ADC;
			
			//Soma amostra atual para calcular a média
			Canal6SomaAnterior += ADC;
			
			//Leu os pontos necessários para começar o cálculo da média?
			if(Canal6NPontos == 3) {
				//Sim
				
				//Só permite ler o próximo canal do ADC depois de terminar a primeira média da temperatura
				Controla_Canal = CanalADC_IAtuador;

				//Calcula a média
				Canal6Media = Canal6SomaAnterior/(Canal6NPontos+1);
				
				//retira primeiro ponto do vetor
				Canal6SomaAnterior -= Canal6ADC[0];
				//Copia amostras anteriores para inserir a próxima amostra
				Canal6ADC[0] = Canal6ADC[1];
				Canal6ADC[1] = Canal6ADC[2];
				Canal6ADC[2] = Canal6ADC[3];	
				
				//Indica que realizou uma nova leitura do NTC
				Flag_LeituraNTC = true;			
											
			} else {
				Canal6NPontos++;
			}			
		break;
	
		case CanalADC_IAtuador:

			//Coloca no vetor a amostra atual
			Canal7ADC[Canal7NPontos] = ADC;
			
			//Soma amostra atual para calcular a média
			Canal7SomaAnterior += ADC;
			
			//Leu os pontos necessários para começar o cálculo da média?
			if(Canal7NPontos == 3) {
				//Sim
				
				//Calcula a média
				Canal7Media = Canal7SomaAnterior/(Canal7NPontos+1);
			
				//retira primeiro ponto do vetor
				Canal7SomaAnterior -= Canal7ADC[0];
				//Copia amostras anteriores para inserir a próxima amostra
				Canal7ADC[0] = Canal7ADC[1];
				Canal7ADC[1] = Canal7ADC[2];
				Canal7ADC[2] = Canal7ADC[3];
				
			} else {
				Canal7NPontos++;
			}			
			
			//Identificação do atuador conectado
			//Lê amostras e detecta maior valor médio 
			if(NumeroAmostrasADCAtuadores_Corrente >= 1) {
				NumeroAmostrasADCAtuadores_Corrente++;
				if(Canal7Media > ValorPicoMedioAtuadores_ADC_Corrente) {
					ValorPicoMedioAtuadores_ADC_Corrente = Canal7Media;
				}
			} 
			
			//Detecta curto no atuador
			//Se corrente no atuador acima de 0,312A POSSÍVEL CURTO	
			//Duração do consumo de corrente motor mola ~ 50ms
			
			if(ADC >= ValorADC_IdentificaCurto_Atuador) {
				
				//Passou o tempo para identificar o curto no atuador?
				if(ContadorIdentificarCurto_Atuador > 1) {
					ContadorIdentificarCurto_Atuador--;								
				}
				else if(ContadorIdentificarCurto_Atuador == 1) {
					//Sim
					
					//Zera temporizador
					ContadorIdentificarCurto_Atuador = 0;
					
					//Redundância desliga atuadores AQUI
					Desliga_MatrizDosAtuadores();
					//Sinaliza que atuador está em curto
					CurtoAtuador = true;	
				}
				
			} else {
				//Se corrente abaixo 0,312A reinicia temporizador
				ContadorIdentificarCurto_Atuador = TempoADC_IdentificaCurto_Atuador;
			}
			
			//Próximo canal do ADC a ser lido
			Controla_Canal = CanalADC_IElevador;	
		break;
	}

//A cada 1 segundo lê o NTC temperatura 
	if(--Conta_Leitura_Temperatura == 0) {
		Conta_Leitura_Temperatura = 10000;		
		Controla_Canal = CanalADC_NTC;
	}
		
	//Seleciona o próximo canal a ser lido
	ADMUX = (ADMUX & 0b11100000)|(Controla_Canal & 0x0F);
}
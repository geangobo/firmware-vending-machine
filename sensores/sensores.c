/*
 * sensores.c
 *
 * Created: 19/12/2021 16:58:14
 *  Author: André
 */ 

#include "sensores.h"

extern union Entrada Entrada_74H165;
extern volatile unsigned int Canal6Media;
volatile unsigned char SensoresPosicaoElevador_Validos[NumeroDeGavetas+1] = {0,0,0,0,0,0,0};

extern volatile unsigned char UltimaPosicaoValidaDoElevador;

//Leitura dos fim de curso
volatile unsigned int Contador_FimDeCurso_Acionado = 0;//, Contador_TravaDasGavetas_Acionado = 0;
volatile bool Flag_Le_EntradasComFiltro = false;
volatile bool Flag_FimDeCurso = false;//  Flag_TravaDasGavetas = false;

volatile unsigned int Debounce_SensorDePosicaoElevador = 0;

/* Lê posição atual do elevador
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Posição atual do elevador. (1 a 7) sensores de posição
 * Retorna valor 0 quando o elevador não esta posicionado em nem um sensor válido
 */
unsigned char PosicaoAtualElevador (void) {
	unsigned char SensorElevador = 0;
	
	Entrada_74H165.Expansor = Le74HC165();
	
	//Retorna apenas a leitura dos sensores de posição válidos
	if((Entrada_74H165.ELEVADOR_P1 == 0) && (SensoresPosicaoElevador_Validos[0] == 1)) {
		SensorElevador = 1;
		UltimaPosicaoValidaDoElevador = 1;
	} else if((Entrada_74H165.ELEVADOR_P2 == 0) && (SensoresPosicaoElevador_Validos[1] == 1)) {
		SensorElevador = 2;
		UltimaPosicaoValidaDoElevador = 2;
	} else if((Entrada_74H165.ELEVADOR_P3 == 0) && (SensoresPosicaoElevador_Validos[2] == 1)) {
		SensorElevador = 3;
		UltimaPosicaoValidaDoElevador = 3;
	} else if((Entrada_74H165.ELEVADOR_P4 == 0) && (SensoresPosicaoElevador_Validos[3] == 1)) {
		SensorElevador = 4;
		UltimaPosicaoValidaDoElevador = 4;
	} else if((Entrada_74H165.ELEVADOR_P5 == 0) && (SensoresPosicaoElevador_Validos[4] == 1)) {
		SensorElevador = 5;
		UltimaPosicaoValidaDoElevador = 5;
	} else if((Entrada_74H165.ELEVADOR_P6 == 0) && (SensoresPosicaoElevador_Validos[5] == 1)) {
		SensorElevador = 6;
		UltimaPosicaoValidaDoElevador = 6;
	} else if((Entrada_74H165.ELEVADOR_P7 == 0) && (SensoresPosicaoElevador_Validos[6] == 1)) {
		SensorElevador = 7;
		UltimaPosicaoValidaDoElevador = 7;
	}

	/*unsigned char SensorElevador = 0;
	static unsigned char Leituras_Sensores[7];
	
	Entrada_74H165.Expansor = Le74HC165();
		//Retorna apenas a leitura dos sensores de posição válidos
	if((Entrada_74H165.ELEVADOR_P1 == 0) && (SensoresPosicaoElevador_Validos[0] == 1)) {
		if(Leituras_Sensores[0] > 5) {
			Leituras_Sensores[0] = 0;
			Leituras_Sensores[1] = 0;
			Leituras_Sensores[2] = 0;
			Leituras_Sensores[3] = 0;
			Leituras_Sensores[4] = 0;
			Leituras_Sensores[5] = 0;
			Leituras_Sensores[6] = 0;			
		} else {
			Leituras_Sensores[0]++;
		}

		if(Leituras_Sensores[0] == 5) {
			SensorElevador = 1;
			UltimaPosicaoValidaDoElevador = 1;
		} 
	} else if((Entrada_74H165.ELEVADOR_P2 == 0) && (SensoresPosicaoElevador_Validos[1] == 1)) {
		if(Leituras_Sensores[1] > 5) {
			Leituras_Sensores[0] = 0;
			Leituras_Sensores[1] = 0;
			Leituras_Sensores[2] = 0;
			Leituras_Sensores[3] = 0;
			Leituras_Sensores[4] = 0;
			Leituras_Sensores[5] = 0;
			Leituras_Sensores[6] = 0;			
		} else {
			Leituras_Sensores[1]++;
		}

		if(Leituras_Sensores[1] == 5) {
			SensorElevador = 2;
			UltimaPosicaoValidaDoElevador = 2;
		} 		
	} else if((Entrada_74H165.ELEVADOR_P3 == 0) && (SensoresPosicaoElevador_Validos[2] == 1)) {
		if(Leituras_Sensores[2] > 5) {
			Leituras_Sensores[0] = 0;
			Leituras_Sensores[1] = 0;
			Leituras_Sensores[2] = 0;
			Leituras_Sensores[3] = 0;
			Leituras_Sensores[4] = 0;
			Leituras_Sensores[5] = 0;
			Leituras_Sensores[6] = 0;			
		} else {
			Leituras_Sensores[2]++;
		}

		if(Leituras_Sensores[2] == 5) {
			SensorElevador = 3;
			UltimaPosicaoValidaDoElevador = 3;
		} 		

	} else if((Entrada_74H165.ELEVADOR_P4 == 0) && (SensoresPosicaoElevador_Validos[3] == 1)) {
		if(Leituras_Sensores[3] > 5) {
			Leituras_Sensores[0] = 0;
			Leituras_Sensores[1] = 0;
			Leituras_Sensores[2] = 0;
			Leituras_Sensores[3] = 0;
			Leituras_Sensores[4] = 0;
			Leituras_Sensores[5] = 0;
			Leituras_Sensores[6] = 0;			
		} else {
			Leituras_Sensores[3]++;
		}

		if(Leituras_Sensores[3] == 5) {
			SensorElevador = 4;
			UltimaPosicaoValidaDoElevador = 5;
		} 		

	} else if((Entrada_74H165.ELEVADOR_P5 == 0) && (SensoresPosicaoElevador_Validos[4] == 1)) {
		if(Leituras_Sensores[4] > 5) {
			Leituras_Sensores[0] = 0;
			Leituras_Sensores[1] = 0;
			Leituras_Sensores[2] = 0;
			Leituras_Sensores[3] = 0;
			Leituras_Sensores[4] = 0;
			Leituras_Sensores[5] = 0;
			Leituras_Sensores[6] = 0;			
		} else {
			Leituras_Sensores[4]++;
		}

		if(Leituras_Sensores[4] == 5) {
			SensorElevador = 5;
			UltimaPosicaoValidaDoElevador = 5;
		} 	

	} else if((Entrada_74H165.ELEVADOR_P6 == 0) && (SensoresPosicaoElevador_Validos[5] == 1)) {
		if(Leituras_Sensores[5] > 5) {
			Leituras_Sensores[0] = 0;
			Leituras_Sensores[1] = 0;
			Leituras_Sensores[2] = 0;
			Leituras_Sensores[3] = 0;
			Leituras_Sensores[4] = 0;
			Leituras_Sensores[5] = 0;
			Leituras_Sensores[6] = 0;			
		} else {
			Leituras_Sensores[5]++;
		}

		if(Leituras_Sensores[5] == 5) {
			SensorElevador = 6;
			UltimaPosicaoValidaDoElevador = 6;
		} 		

	} else if((Entrada_74H165.ELEVADOR_P7 == 0) && (SensoresPosicaoElevador_Validos[6] == 1)) {
		if(Leituras_Sensores[6] > 5) {
			Leituras_Sensores[0] = 0;
			Leituras_Sensores[1] = 0;
			Leituras_Sensores[2] = 0;
			Leituras_Sensores[3] = 0;
			Leituras_Sensores[4] = 0;
			Leituras_Sensores[5] = 0;
			Leituras_Sensores[6] = 0;			
		} else {
			Leituras_Sensores[6]++;
		}

		if(Leituras_Sensores[6] == 5) {
			SensorElevador = 7;
			UltimaPosicaoValidaDoElevador = 7;
		} 	
	} */
		
	return(SensorElevador);
}

/* Verifica se elevador está na posição passada por parâmetro e se sensor de posição está funcionando corretamente
 * Parâmetros de entrada:
 * Posicao_Elevador_Desejada (1 a 7)                     
 * 
 * Parâmetro de saída:
 * Posição atual do elevador com sensor válido. (1 a 7)
 */
unsigned char Verifica_Posicao_Elevador(unsigned char Posicao_Elevador_Desejada) {
	static unsigned char SensorElevador;
	
	Entrada_74H165.Expansor = Le74HC165();
	
	switch(Posicao_Elevador_Desejada) {
		case 1:
		if((Entrada_74H165.ELEVADOR_P1 == 0) && (SensoresPosicaoElevador_Validos[0] == 1)) {
			SensorElevador = 1;
			UltimaPosicaoValidaDoElevador = 1;
		}
		break;
		
		case 2:
		if((Entrada_74H165.ELEVADOR_P2 == 0) && (SensoresPosicaoElevador_Validos[1] == 1)) {
			SensorElevador = 2;
			UltimaPosicaoValidaDoElevador = 2;
		}
		break;
		
		case 3:
		if((Entrada_74H165.ELEVADOR_P3 == 0) && (SensoresPosicaoElevador_Validos[2] == 1)) {
			SensorElevador = 3;
			UltimaPosicaoValidaDoElevador = 3;
		}
		break;

		case 4:
		if((Entrada_74H165.ELEVADOR_P4 == 0) && (SensoresPosicaoElevador_Validos[3] == 1)) { 
			SensorElevador = 4;
			UltimaPosicaoValidaDoElevador = 4;
		}
		break;

		case 5:
		if((Entrada_74H165.ELEVADOR_P5 == 0) && (SensoresPosicaoElevador_Validos[4] == 1)) {
			SensorElevador = 5;
			UltimaPosicaoValidaDoElevador = 5;
		}
		break;

		case 6:
		if((Entrada_74H165.ELEVADOR_P6 == 0) && (SensoresPosicaoElevador_Validos[5] == 1)) {
			SensorElevador = 6;
			UltimaPosicaoValidaDoElevador = 6;
		}
		break;

		case 7:
		if((Entrada_74H165.ELEVADOR_P7 == 0) && (SensoresPosicaoElevador_Validos[6] == 1)) {
			SensorElevador = 7;
			UltimaPosicaoValidaDoElevador = 7;
		}
		break;
	}
	
	return(SensorElevador);
}

/* Verifica se sensor de posição enviado por parâmetro está funcionando corretamente
 * Parâmetros de entrada:
 * _p (1 a 7)
 *
 * Parâmetro de saída:
 * true = Sensor de posição funcionando corretamente
 * false = Sensor de posição não identificado
 */
unsigned char Valida_SensorDePosicaoDoElevador(signed char _p) {
	if(SensoresPosicaoElevador_Validos[_p-1]) return true;
	else return false;
}

/* Calcula temperatura utilizando sensor NTC 10K @ 25°C com coeficiente 3950
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Temperatura atual. 
 * Caso NTC esteja desconectado retornará -40°C (#define TemperaturaMinima_C)
 */
float CalculaTemperatura(void) {
	float TensaoNtc = 0.0, TempNtc = 0.0, ResistenciaNtc = 0.0;
	
	//Calcula tensão em cima do NTC
	TensaoNtc = (float) Canal6Media * TensaoPorBit;
	
	//Calcula a resistência do NTC
	ResistenciaNtc = (TensaoNtc * ResistenciaDivisorNTC_Ohms) / (VrefADC - TensaoNtc);

	//Calcula a temperatura em Celcius
	TempNtc = (1.0 / (CoeficienteA + (CoeficienteB * log(ResistenciaNtc)) + (CoeficienteC * log(ResistenciaNtc)* log(ResistenciaNtc)* log(ResistenciaNtc)))) + ZeroKelvinEmCelcius;
	
	//Limita range de temperatura
	if(TempNtc < TemperaturaMinima_C) TempNtc = TemperaturaMinima_C;
	if(TempNtc > TemperaturaMaxima_C) TempNtc = TemperaturaMaxima_C;
	
	return(TempNtc);
}

/* Lê status da porta principal
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 *					0 - Porta principal FECHADA
 *					1 - Porta principal ABERTA
 */
unsigned char VerificaPortaPrincipal (void) {
	unsigned int r = 0;
	
	Entrada_74H165.Expansor = Le74HC165();
	if(Entrada_74H165.PORTA_PRINCIPAL == 1) {
		//Porta aberta
		r = 1;
	}
	return r;
}

/* Configura pino para ler o sensor magnético da esteira
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetros de saída:
 * Não tem 
 */
void ConfiguraLeituraEsteira(void) {
	//Configura pino de Leitura da esteira como entrada digital
	CLEAR_BIT(Config_Port_LeituraEsteira,Pino_LeituraEsteira);	
}

/* Lê as chaves fim de curso com filtro para evitar que ruídos do acionamento do elevador atrapalhe a leitura.
 * Parâmetros de entrada:
 * TempoAmstragem_ms
 * 
 * Parâmetros de saída:
 * Não tem
 */
void Le_Entradas_Com_Filtro(unsigned char TempoAmostragem_ms) {

	Entrada_74H165.Expansor = Le74HC165();
	if((Entrada_74H165.FIM_CURSO_INFERIOR == 1) || (Entrada_74H165.FIM_CURSO_SUPERIOR == 1) || (Entrada_74H165.FIM_CURSO_RESERVA == 1)) {
		//Algum fim de curso acionado
		
		if(Contador_FimDeCurso_Acionado < TempoAmostragem_ms) {					
			if (Flag_FimDeCurso == false) {				
				Contador_FimDeCurso_Acionado++;
			}
		} else {
			if (Flag_FimDeCurso == false) {
				Flag_FimDeCurso = true;
			}
		}
	} else {
		Flag_FimDeCurso = false;
		Contador_FimDeCurso_Acionado = 0;		
	}	
}


/*
 * sensor_de_queda.c
 *
 * Created: 19/12/2021 10:43:01
 *  Author: André
 */ 

#include "sensor_de_queda.h"

//Union com saídas do expansor
extern union Saida Saida_74HC595;

/* Configura pino conectado ao SENSOR DE QUEDA como entrada
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void ConfiguraLeituraSensorDeQueda(void) {
	//Configura pino do Sensor de Queda como entrada digital
	CLEAR_BIT(Config_Port_SensorDeQueda,Pino_SensorDeQueda);

}

/* Liga alimentação de 24V do SENSOR DE QUEDA
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void LigaSensorDeQueda(void) {
	Saida_74HC595.SENSOR_DE_QUEDA = 1;
	Escreve74HC595(Saida_74HC595.Expansor);	
}

/* Desliga alimentação de 24V do SENSOR DE QUEDA
 * Parâmetros de entrada:
 * Não tem
 *
 * Parâmetro de saída:
 * Não tem
 */
void DesligaSensorDeQueda(void) {
	Saida_74HC595.SENSOR_DE_QUEDA = 0;
	Escreve74HC595(Saida_74HC595.Expansor);
}

ISR(INT0_vect) {
	//Vetor de interrupção de INT0
}
	
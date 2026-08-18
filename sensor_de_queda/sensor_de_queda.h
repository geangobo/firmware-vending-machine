/*
 * sensor_de_queda.h
 *
 * Created: 19/12/2021 10:43:19
 *  Author: André
 */ 


#ifndef SENSOR_DE_QUEDA_H_
#define SENSOR_DE_QUEDA_H_

#include "../main.h"

#define TempoEsperaParaValidar_SensorDeQueda_ms	250
#define TimeOutSensorDeQueda_ms			5000
#define TempoSensorDeQuedaFicaDesligadoEntreCompras_ms  250

#define SensorDeQueda_Nao_IdentificouAEntregaDoProduto	0x39
#define SensorDeQueda_IdentificouAEntregaDoProduto		0x3A
#define ProdutoNaoSolicitado                            0x3B
#define Multicarrinho_Desabilitado                      0x3C

#define Leitura_Port_SensorDeQueda		PIND
#define	Config_Port_SensorDeQueda		DDRD
#define Port_SensorDeQueda				PORTD
#define Pino_SensorDeQueda				2		//PD2 PINO32

//Protótipos de funções
void ConfiguraLeituraSensorDeQueda(void);
void LigaSensorDeQueda(void); 
void DesligaSensorDeQueda(void);

#endif /* SENSOR_DE_QUEDA_H_ */
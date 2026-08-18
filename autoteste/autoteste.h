/*
 * autoteste.h
 *
 * Created: 31/05/2022 20:01:45
 *  Author: André
 */ 


#ifndef AUTOTESTE_H_
#define AUTOTESTE_H_

//Include
#include "../main.h"

#define TempoEsperaRepostaNoAutoteste_ControladorDeTemperatura_ms 2000

typedef struct {
	bool Fusivel_Atuadores_OK;
	bool SensorDeCorrente_Atuadores_OK;
	bool SensorDeTensao_Atuadores_OK;
	bool Fusivel_Elevador_OK;
	bool TravaDasGavetas_OK;
	bool SensoresDePosicao_OK;
	bool FimDeCurso_OK;
	bool SensorDeRepouso_OK;
	bool SensorDeEntrega_OK;
	bool SensorDeTemperatura_OK;
	unsigned char ComunicacaoComControladorDeTemperatura_OK;
} AT;

//Retorno do AutoTeste
#define Status_AT_AutoTesteFinalizado			0
#define Status_AT_SubTesteFinalizado			1
#define Status_AT_RealizandoAutoTeste			2

//Estados do autoteste
#define M_AT_TestaFusivel_Atuadores											0
#define M_AT_TestaSensorDeCorrente_Atuadores								1
#define M_AT_TestaSensorDeTensao_Atuadores									2
#define M_AT_Identifica_Atuadores											3
#define M_AT_TestaFusivel_Elevador											4
#define M_AT_TestaTrava_Gavetas												5
#define M_AT_TestaElevador													8
#define M_AT_TestaSensorDeTemperatura										9
#define M_AT_FimDeCurso														10
#define M_AT_ComunicacaoControladorDeTemperatura							11

//Teste da comunicação com o controlador de temperatura (TCT)
#define M_TCT_TC900Elog_EnviaMensagem										0
#define M_TCT_TC900Elog_EsperaResposta										1
#define M_TCT_MT512Elog_EnviaMensagem										2
#define M_TCT_MT512Elog_EsperaResposta										3


//Teste do fusivel dos atuadores (TFA)
#define M_TFA_LIGA_24V_NOS_ATUADORES										0
#define M_TFA_ESPERA_LIGAR_24V_NOS_ATUADORES								1
#define M_TFA_TESTA_FUSIVEL_DOS_ATUADORES									2
#define M_TFA_ESPERA_PARA_TESTAR_FUSIVEL_DOS_ATUADORES						3

#define NUMERO_DE_LEITURAS_DO_FUSIVEL_DOS_ATUADORES							3

#define TEMPO_ESPERA_RELE_BATER_ms											100	// 100ms
#define INTERVALO_DE_ESPERA_ENTRE_LEITURAS_DO_FUSIVEL_DOS_ATUADORES_ms		10	//10ms

//Teste do sensor de corrente dos atuadores (TSCA)
#define M_TSCA_LIGA_24V_E_DESLIGA_MATRIZ									0
#define M_TSCA_ESPERA_LIGA_24V_E_DESLIGA_MATRIZ								1
#define M_TSCA_LE_CORRENTE_COM_MATRIZ_DESLIGADA								2
#define M_TSCA_ESPERA_LE_CORRENTE_COM_MATRIZ_DESLIGADA						3
#define M_TSCA_LIGA_HIGH_SIDE_DA_MATRIZ										4
#define M_TSCA_ESPERA_LIGA_HIGH_SIDE_DA_MATRIZ								5
#define M_TSCA_LE_CORRENTE_COM_MATRIZ_LIGADA								6
#define M_TSCA_ESPERA_LE_CORRENTE_COM_MATRIZ_LIGADA							7

#define NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_CORRENTE_DOS_ATUADORES		3

//Teste do sensor de tensão dos atuadores (TSTA)
#define M_TSTA_LIGA_24V_NOS_ATUADORES										0
#define M_TSTA_ESPERA_LIGAR_24V_NOS_ATUADORES								1
#define M_TSTA_LE_TENSAO_24V_NOS_ATUADORES									2
#define M_TSTA_ESPERA_LER_TENSAO_24V_NOS_ATUADORES							3
#define M_TSTA_LIGA_5V_NOS_ATUADORES										4
#define M_TSTA_ESPERA_LIGAR_5V_NOS_ATUADORES								5
#define M_TSTA_LE_TENSAO_5V_NOS_ATUADORES									6
#define M_TSTA_ESPERA_LER_TENSAO_5V_NOS_ATUADORES							7

#define NUMERO_DE_LEITURAS_DO_TESTE_DO_SENSOR_DE_TENSAO_DOS_ATUADORES		3

//Testa Atuadores (TA)
#define	M_TA_LIGA_5V_NOS_ATUADORES											0
#define M_TA_ESPERA_LIGAR_5V_NOS_ATUADORES									1
#define M_TA_SELECIONA_GAVETA												2
#define M_TA_ESPERA_SELECIONAR_GAVETA										3
#define M_TA_SELECIONA_ATUADOR_DA_GAVETA									4
#define M_TA_ESPERA_SELECIONAR_ATUADOR_DA_GAVETA							5
#define M_TA_LE_TENSAO_E_CORRENTE_DO_ATUADOR								6
#define M_TA_IDENTIFICA_ATUADOR												7

#define INTERVALO_DE_ESPERA_PARA_SELECIONAR_A_GAVETA_ms						50	//5ms

//Teste do fusível do elevador (TFE)
#define M_TFE_INICIA_TESTE_FUSIVEL_DO_ELEVADOR								0
#define M_TFE_TESTA_FUSIVEL_DO_ELEVADOR                						1
#define M_TFE_ESPERA_PARA_TESTAR_FUSIVEL_DO_ELEVADOR						2

#define NUMERO_DE_LEITURAS_DO_FUSIVEL_DO_ELEVADOR							3

#define INTERVALO_DE_ESPERA_ENTRE_LEITURAS_DO_FUSIVEL_DO_ELEVADOR_ms		10	//10ms

//Teste da trava das gavetas (TTG)
#define M_TTG_INICIA_TESTE_TRAVA_DAS_GAVETAS								0
#define M_TTG_TESTA_TRAVA_DAS_GAVETAS                						1
#define M_TTG_ESPERA_PARA_TESTAR_TRAVA_DAS_GAVETAS							2

#define NUMERO_DE_LEITURAS_DA_TRAVA_DAS_GAVETAS 							3

#define INTERVALO_DE_ESPERA_ENTRE_LEITURAS_DA_TRAVA_DAS_GAVETAS_ms			10	//10ms

//Teste do elevador (TE)
#define M_TE_TESTA_FIM_DE_CURSO										0
#define M_TE_SOBE_ELEVADOR											1
#define M_TE_ESPERA_ELEVADOR_SUBIR									2
#define M_TE_DESCE_ELEVADOR											3
#define M_TE_ESPERA_ELEVADOR_DESCER									4
#define M_TE_SOBE_ELEVADOR_NO_REPOUSO								5
#define M_TE_ESPERA_ELEVADOR_CHEGAR_NO_REPOUSO						6
#define M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_SUBINDO			7
#define M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_DESCENDO			8
#define M_TE_SOBE_ELEVADOR_FIM_DE_CURSO_ACIONADO					9
#define M_TE_DESCE_ELEVADOR_FIM_DE_CURSO_ACIONADO					10
#define M_TE_ESPERA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_CHEGAR_NO_REPOUSO	11
#define M_TE_POSICIONA_ELEVADOR_NO_ULTIMO_SENSOR_VALIDO				12

#define TEMPO_ESPERA_PARA_ELEVADOR_SAIR_DO_FIM_DE_CURSO_ms		250

//Leitura do fusível dos atuadores
#define NUMERO_DE_LEITURAS_DO_FUSIVEL_DOS_ATUADORES		3

//Teste do sensor de corrente dos atuadores
#define NUMERO_DE_LEITURAS_SENSOR_DE_CORRENTE_ATUADORES	3

//Cada led consome +/- 2,2mA -> 6 leds +/- 13,2mA
//Sensor de corrente com ganho 10 e shunt de 1R 5%
//0,132V em ADC 0,132 / (3,5/1024) = 38
//Na prática detecta com ADC = 30 -> APLICA MARGEM DE 50% 15
#define VALOR_DO_ADC_SENSOR_DE_CORRENTE_ATUADORES_OK	15

#define VALOR_DO_ADC_ATUADOR_CONECTADO					33

//Teste do sensor de tensão dos atuadores
#define NUMERO_DE_LEITURAS_SENSOR_DE_TENSAO_ATUADORES	3

//leitura do fusível do elevador
#define NUMERO_DE_LEITURAS_DO_FUSIVEL_DO_ELEVADOR		3

//Leitura da trava das gavetas
#define NUMERO_DE_LEITURAS_DA_TRAVA_DAS_GAVETAS			3

//Protótipos de função
unsigned char RealizaAutoteste(void);
unsigned char TestaComunicacao_ControladorDeTemperatura(void);
unsigned char TestaFusivel_Atuadores(void);
unsigned char TestaSensorDeCorrente_Atuadores(void);
unsigned char TestaSensorDeTensao_Atuadores(void);
unsigned char Identifica_Atuadores(void);
unsigned char TestaFusivel_Elevador(void);
unsigned char TestaTrava_Gavetas(void);
unsigned char TestaElevador(void);
void Retorna_StatusDoAutoteste(void);
#endif /* AUTOTESTE_H_ */
/*
 * atuadores.h
 *
 * Created: 19/12/2021 12:22:52
 *  Author: André
 */ 


#ifndef ATUADORES_H_
#define ATUADORES_H_

//Include
#include <avr/io.h>
#include "../main.h"

#define NumeroMinimoDeAtuadores				1
#define NumeroMaximoDeAtuadores				72

#define NumeroAtuadoresNaGaveta				12
#define NumeroDeGavetas						6

#define Gaveta_Inicial						0x11
#define Gaveta_Final						0x16

#define Tamanho_VetorIdentificaAtuadores	5
#define Tamanho_VetorTipoDeAtuadores		8

//Identifica atuador
enum IdentificaAtuador {
	Atuador_Desconectado = 0x26, Atuador_Conectado, Atuador_Em_Curto
};

//Seleção do tipo de atuador
enum TipoAtuador {
	Motor_Mola_Simples = 0x26,	Gancheira_Mola, Motor_Mola_Duplo, Gancheira, Esteira	
};		

#define LigaAtuador					1
#define DesligaAtuador				0

//Seleção de tensão dos atuadores (RELÉ)
#define Aplica_5V_20mA_Nos_Atuadores				1
#define Aplica_24V_Atuadores						0

//Selecão tensão para manter movimento do atuadores ou ler esteira
#define Ler_Sensor_Da_Esteira				0
#define Manter_Movimentacao_Dos_Atuadores	1

//Temporização
#define TempoEsperaParaVerificarFusivel_ms				100		//ms
#define TempoPulsoInicial_Motor_Mola_ms					1000	//ms
#define TempoPulsoInicial_Gancheira_ms					200		//ms Pulso inicial reduzido

																//produto pequeno leva +/- 16 segundos para percorer toda esteira
#define TempoPulsoInicial_Esteira_ms					21000	//ms = 21 segundos (Fator de segurança de 30%)

#define TempoEntreEnergizar_Esteira_E_MontorarSensorMagnetico	200//100	//ms

#define TempoEsperaFinalizarGiro_Motor_Mola_ms				2000	//ms
#define TempoEsperaFinalizar_Gancheira_ms					3500  //ms

//Pinagem
//Pino Enable 74HC595
#define Config_Port_Enable_74HC595	DDRD
#define Port_Enable_74HC595			PORTD
#define Pino_Enable_74HC595			4		//PD4 - Pino 2

//Elevador
#define TempoEntreAcionamentoDoElevador_ms		100	//ms
#define TempoEntreAcionamentoDesligarElevador_ms	10//ms
#define TimeOutMovimentaElevador_ms				10000	//ms

#define PosicaoGaveta_Entrega	6

//Maquina de estados Atuador
#define MA_AtuadorDisponivel			0
#define MA_VerificaAtuador				1
#define MA_LigaFonte					2
#define MA_EsperaFonteLigar				3
#define MA_VerificaFusivel				4
#define	MA_IdentificaLinhaEColuna		5
#define MA_VerificaSensorMagnetico_Esteira	6
#define MA_PulsoInicial				7
#define EsperaPulsoInicial			8
#define FinalizaPulsoInicial		9
#define EsperaFinalizarGiro			10
#define GiroFinalizado				11
#define AtuadorDesligado			12
#define MA_FinalizaAcionamento		13
#define EsperaEnergizarEsteira		14
//Retorno Maquina de estados Atuador
#define Status_MA_AcionandoAtuador			0
#define Status_MA_AcionamentoFinalizado					0x26//1
#define Status_MA_Erro_FusivelDosAtuadoresRompido		0x27//2
#define Status_MA_Erro_AtuadorDesconectado				0x28//3
#define Status_MA_Erro_AtuadorEmCurto					0x29//4
#define Status_MA_Erro_TimeoutEsteira					0x2A//5
#define Status_MA_Erro_SensorEsteiraAcionadoAntesDeIniciarMovimentacao	0x2B//6
#define Status_MA_Erro_AtuadorForaDoRange				0x2C//7

//Maquina de estados para posicionar Elevador
#define MP_ElevadorDisponivel		0
#define MP_SobeElevador				1
#define MP_ElevadorGaveta1			2
#define MP_DesceElevador			3
#define MP_ElevadorGaveta6			4
#define MP_ElevadorNaPosicao		5
//Retorno Maquina de estados para posicionar Elevador



//Maquina de estados para movimentar Elevador
#define MM_ElevadorDisponivel		0
#define MM_ValidaPosicao_E_Hardware			1
#define MM_VerificaPosicao			2
#define MM_Sobe						3
#define MM_Desce					4
#define MM_EsperaPosicao			5
#define MM_EsperaChegarEmCima		7
#define MM_EsperaChegarEmBaixo		8
#define MM_FinalizaAcionamento		9
#define MM_RetornaUltimaPosicaoValida	10
#define MM_Sobe_Curto					11
#define MM_Desce_Curto					12
#define MM_EsperaChegarNaUltimaPosicaoValida	13
#define MM_Espera_Descer			14
#define MM_Espera_Subir			5
#define MM_SobeElevador_Com_FimDeCurso_Acionado		15
#define MM_EsperaElevador_SairDo_FimDeCurso			16
#define MM_DesceElevador_Com_FimDeCurso_Acionado	17

//Retorno maquina de estados Movimentar Elevador
#define Status_MM_AcionandoElevador								0
#define Status_MM_AcionamentoFinalizado							0x26//1
#define Status_MM_SensorDePosicaoDoElevadorInvalido				0x27//2
#define Status_MM_PosicaoDoElevadorSolicitadaInvalida			0x28//3
#define Status_MM_SensorDeEntregaInvalido						0x29//4
#define Status_MM_SensorDeRepousoInvalido						0x2A//5
#define Status_MM_TimeoutElevador								0x2B//6
#define Status_MM_ElevadorEmCurto								0x2C//7
#define Status_MM_ElevadorDesabilitado							0x2D//8
#define Status_MM_TravaDasGavetasAberta							0x2E//9 
#define Status_MM_FimDeCurso									0x2F//10
#define Status_MM_PortinholaAbertaOuFusivelRompido				0x30//11

//Protótipos de funções

//Identifica atuadores
void AcionaIdentificacaoAtuadores (unsigned char NivelLogico);
void SelecionaAlimentacaoAtuadores (unsigned char NivelLogico);

unsigned char Verifica_GavetaDoAtuador(unsigned char atuador);
 
//Comanda atuadores
void InicializaMaquinaDeEstados_Atuadores(unsigned char EstadoInicial);

unsigned char AcionaAtuador(unsigned char NumeroAtuador, unsigned char TipoAtuador);
void ComandaHighSide(unsigned char Linha, unsigned char NivelLogico);
void ComandaLowSide(unsigned char Coluna, unsigned char NivelLogico);
void Desliga_MatrizDosAtuadores(void);

void AcionaTravaMagnetica (unsigned char NivelLogico);
void AcionaSolenoide (unsigned char NivelLogico);

void ConfiguraBUZZER(void);
void ConfiguraElevador(void);
void Configura_Pino_Enable_74HC595(void);

//Elevador
void InicializaMaquinaDeEstados_PosicionaElevador(unsigned char EstadoInicial); 
void InicializaMaquinaDeEstados_MovimentaElevador(unsigned char EstadoInicial);
unsigned char MovimentaElevador(unsigned char p);

#endif /* ATUADORES_H_ */
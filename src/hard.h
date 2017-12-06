/*
 * hard.h
 *
 *  Created on: 28/11/2013
 *      Author: Mariano
 */

#ifndef HARD_H_
#define HARD_H_

#include "stm32f0xx_adc.h"

//--- Board Configuration ---------------------------------------//
//----------- Defines For Configuration -------------
//----------- Hardware Board Version -------------
#define HARD_1_0
// #define HARD_2_0

//----------- Software Version -------------
// #define SOFT_1_0			//coincide con Hard 1.0
// #define SOFT_2_1				//nuevo

//-------- Type of Program o Heladera ----------------

//-------- Type of Temparature determination ----------------

//-------- Clock Frequency ------------------------------------
// #define CLOCK_FREQ_48_MHZ
#define CLOCK_FREQ_8_MHZ


//-------- Hardware and Soft resources for Type of Program ----------------
#ifdef SOFT_1_0
// #define LARGO_FILTRO_POTE 16
// #define DIVISOR_POTE      4  		//2 elevado al divisor = largo filtro
// #define UPDATE_FILTRO_POTE 10		//total de 160ms
//
// #define LARGO_FILTRO_TEMP 32
// #define DIVISOR_TEMP      5   //2 elevado al divisor = largo filtro
// //#define UPDATE_FILTRO_TEMP 312		//total de 10 segundos
// #define UPDATE_FILTRO_TEMP 32		//total de 1 segundos
#endif


//--- End Board Configuration -----------------------------------//


//para GPIO 1 solo bit uso Port bit set/reset register (GPIOx_BSRR) (x=A..G)
//GPIOA pin0

//GPIOA pin1
#define ACT_12V ((GPIOA->ODR & 0x0002) != 0)
#define ACT_12V_ON GPIOA->BSRR = 0x00000002
#define ACT_12V_OFF GPIOA->BSRR = 0x00020000

//GPIOA pin2
#define BOTON1 ((GPIOA->ODR & 0x0004) != 0)
#define BOTON1_ON GPIOA->BSRR = 0x00000004
#define BOTON1_OFF GPIOA->BSRR = 0x00040000

//GPIOA pin3
#define SW1 ((GPIOA->IDR & 0x0008) == 0)

//GPIOA pin4
#define LED ((GPIOA->ODR & 0x0010) != 0)
#define LED_ON	GPIOA->BSRR = 0x00000010
#define LED_OFF GPIOA->BSRR = 0x00100000

//GPIOA pin5
//GPIOA pin6
//GPIOA pin7
//GPIOA pin9
//GPIOA pin10


#define SW_ROOF	200
#define SW_THRESH	150


//ESTADOS DEL PROGRAMA PRINCIPAL
typedef enum
{
	main_init = 0,
	waiting_activate_or_config,
	alarm_on,
	wait_configuration,
	configuration,
	to_save,
	saved,
	saved_error,
	wait_shutdown

} main_state_t;


//ESTADOS DEL LED
#define START_BLINKING	0
#define WAIT_TO_OFF	1
#define WAIT_TO_ON	2
#define WAIT_NEW_CYCLE	3


/* Module Functions ------------------------------------------------------------*/
void UpdateLed (unsigned char);
unsigned char Switch (void);
void ResetLed (void);


#endif /* HARD_H_ */

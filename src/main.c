/**
  ******************************************************************************
  * @file    Template_2/main.c
  * @author  Nahuel
  * @version V1.0
  * @date    22-August-2014
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Use this template for new projects with stm32f0xx family.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"
#include "gpio.h"
#include "stm32f0xx_adc.h"
#include "core_cm0.h"
#include "hard.h"
#include "tim.h"

// #include <stdio.h>


//--- VARIABLES EXTERNAS ---//
volatile unsigned short wait_ms_var;
volatile unsigned short led_timer = 0;
volatile unsigned short	sw_filter = 0;
volatile unsigned char delay_timer = 0;

//--- VARIABLES GLOBALES ---//


//--- FUNCIONES DEL MODULO ---//
void TimingDelay_Decrement(void);



//-------------------------------------------//
// @brief  Main program.
// @param  None
// @retval None
//------------------------------------------//
int main(void)
{
	unsigned char i = 0;
	unsigned char led_state = 0;
	unsigned char blink = 0;


	//!< At this stage the microcontroller clock setting is already configured,
    //   this is done through SystemInit() function which is called from startup
    //   file (startup_stm32f0xx.s) before to branch to application main.
    //   To reconfigure the default setting of SystemInit() function, refer to
    //   system_stm32f0xx.c file

	//GPIO Configuration.
	GPIO_Config();

	//ACTIVAR SYSTICK TIMER
#ifdef CLOCK_FREQ_48_MHZ
	if (SysTick_Config(48000))
#endif
#ifdef CLOCK_FREQ_8_MHZ
	if (SysTick_Config(8000))
#endif
	{
		while (1)	/* Capture error */
		{
			if (LED)
				LED_ON;
			else
				LED_OFF;

			for (i = 0; i < 255; i++)
   		{
   			asm (	"nop \n\t"
   					"nop \n\t"
   					"nop \n\t" );
   		}
		}
	}

	//TIM Configuration.
	// TIM_3_Init();
	// TIM_16_Init();			//lo uso en HARD_2_0 para el sync de relay

	//UART configuration.
	//USART_Config()

	//ADC configuration.
	// AdcConfig();

	// TIM16Enable ();

	//Inicia prueba LED
	while (1)
	{
		LED_ON;
	   Wait_ms(1000);
	   LED_OFF;
		Wait_ms(1000);
	}
	//FIN prueba LED

//    //para pruebas
//    Wait_ms(9000);
//    while (1)
//    {
//    	if (!timer_relay)
//    	{
//    		if (RELAY)
//    			RELAY_OFF;
//    		else
//    			RELAY_ON;
//
//    		timer_relay = 10000;
//    	}
//    }
//    //para pruebas

	while (1)
	{



		//Cuestiones generales que no tienen que ver con el modo de stop
// #ifdef RELAY_ALWAYS_ON
//
//
// #elif defined RELAY_OFF_WITH_DOOR_OPEN
//
//
// #else
// #error "Falta elegir Type of Program hard.h"
// #endif

		//Cuestiones generales a todos los programas
		// UpdatePote();
		// UpdateTemp();

	}	//End of while 1


	//--- End of New Main loop ---//

//    while (1)
//    {
//    	if (SYNC)
//    		SYNC_OFF;
//    	else
//    		SYNC_ON;
//
//    	Wait_ms (10);
//    }

	return 0;
}


/**
  * @brief  Decrements the TimingDelay variable.	ESTA ES LA QUE LLAMA SYSTICK CADA 1MSEG
  * @param  None
  * @retval None
  */

// ------- Private Variables -------
volatile unsigned short secs = 0;

void TimingDelay_Decrement(void)
{
	if (wait_ms_var)
		wait_ms_var--;

	//filtro de ruido para el sw
	if (SW1)
	{
		if (sw_filter < SW_ROOF)
			sw_filter++;
	}
	else if (sw_filter)
		sw_filter--;

	if (led_timer)
		led_timer--;

	//cuenta de a 1 minuto
	if (secs > 9999)			//pasaron 1 segundo
	{
		secs = 0;

		if (delay_timer)
			delay_timer--;		//timer de reportes de a 1 segundo
	}
	else
		secs++;

}

//--- End of file ---//

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
#include "flash_program.h"

// #include <stdio.h>


//--- VARIABLES EXTERNAS ---//
volatile unsigned short wait_ms_var;
volatile unsigned short led_timer = 0;
volatile unsigned short	sw_filter = 0;
volatile unsigned char delay_timer = 0;

unsigned char blinks_rounds = 0;

//--- VARIABLES GLOBALES ---//
volatile unsigned short	timer_standby = 0;

parameters_typedef params;

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
	unsigned char last_switch = 0;
	unsigned char delay_timer_conf = 0;
	unsigned char save_conf = 0;
	unsigned char res = 0;

	main_state_t main_state = main_init;


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

	// //Inicia prueba LED
	// while (1)
	// {
	// 	// LED_ON;
	//    // Wait_ms(1000);
	//    // LED_OFF;
	// 	// Wait_ms(1000);
	//
	// 	if (LED)
	// 		LED_OFF;
	// 	else
	// 		LED_ON;
	//
	// 	Wait_ms(1000);
	// }
	// //FIN prueba LED

	// //Inicia prueba LED
	// while (1)
	// {
	// 	if (SW1)
	// 	{
	// 		LED_ON;
	// 		ACT_12V_ON;
	// 		BOTON1_ON;
	// 	}
	// 	else
	// 	{
	// 		LED_OFF;
	// 		ACT_12V_OFF;
	// 		BOTON1_OFF;
	// 	}
	//
	//
	// 	// if (LED)
	// 	// 	LED_OFF;
	// 	// else
	// 	// 	LED_ON;
	// 	//
	// 	// Wait_ms(1000);
	// }
	// //FIN prueba LED

	//Comienza Programa Principal

	//cargo las variables desde la memoria
	params.secs = ((parameters_typedef *) (unsigned char *) PAGE15)->secs;

	if ((params.secs < 1) || (params.secs > 10))
	{
		//debe estar vacia la memoria
		delay_timer_conf = 5;
	}
	else
		delay_timer_conf = params.secs;


	ACT_12V_OFF;


	while (1)
	{
		switch (main_state)
		{
			case main_init:
				delay_timer = delay_timer_conf;
				main_state++;
				LED_ON;
				timer_standby = 300;
				break;

			case waiting_activate_or_config:
				if (!timer_standby)
					LED_OFF;

				if (!delay_timer)
				{
					//debo activar, presiono boton1
					BOTON1_ON;
					ACT_12V_ON;
					timer_standby = 1000;
					main_state = alarm_on;
				}

				if (Switch())	//voy a configuracion
				{
					main_state = wait_configuration;
					LED_ON;
					BOTON1_OFF;
					ACT_12V_OFF;
				}
				break;

			case alarm_on:
				if (!timer_standby)
				{
					BOTON1_OFF;		//libero boton1
					main_state = wait_shutdown;
				}

				if (Switch())	//voy a configuracion
				{
					main_state = wait_configuration;
					LED_ON;
					BOTON1_OFF;
					ACT_12V_OFF;
				}
				break;

			case wait_shutdown:
				//me quedo esperando que se apague la placa o  configuracion

				if (Switch())	//voy a configuracion
				{
					main_state = wait_configuration;
					LED_ON;
					BOTON1_OFF;
					ACT_12V_OFF;
				}
				break;

			case wait_configuration:
				if (!Switch())
				{
					LED_OFF;
					last_switch = 0;
					main_state = configuration;
					blinks_rounds = 0;
				}
				break;

			case configuration:
				UpdateLed (delay_timer_conf);

				if ((Switch()) && (last_switch == 0))
				{
					if (delay_timer_conf < 10)
						delay_timer_conf++;
					else
						delay_timer_conf = 1;

					last_switch = 1;
					blinks_rounds = 0;
					save_conf = 1;
				}

				if (!Switch())
					last_switch = 0;

				//me fijo si tengo que guardar
				if ((save_conf) && (blinks_rounds > 4))
				{
					main_state = to_save;
					params.secs = delay_timer_conf;
				}
				break;

			case to_save:
				res = WriteConfigurations(&params);
				if (res == PASSED)
					main_state = saved;
				else
					main_state = saved_error;
				break;

			case saved:
				//no hago nada hasta que se apague la placa
				if (LED)
					LED_OFF;
				else
					LED_ON;

				Wait_ms(100);
				break;

			case saved_error:
				//no hago nada hasta que se apague la placa
				if (LED)
					LED_OFF;
				else
					LED_ON;

				Wait_ms(400);
				break;

			default:
				main_state = main_init;
				break;
		}



		//Cuestiones generales que no tienen que ver con el loop principal
		// UpdateSwitch();

	}	//End of while 1
	//--- End of New Main loop ---//
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

	if (timer_standby)
		timer_standby--;

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
	if (secs > 999)			//pasaron 1 segundo
	{
		secs = 0;

		if (delay_timer)
			delay_timer--;		//timer de reportes de a 1 segundo
	}
	else
		secs++;

}

//--- End of file ---//

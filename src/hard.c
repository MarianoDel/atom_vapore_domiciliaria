/*
 * hard.c
 *
 *  Created on: 28/03/2017
 *      Author: Mariano
 */

#include "hard.h"
#include "tim.h"

/* Externals variables ---------------------------------------------------------*/
extern unsigned short sw_filter;
extern volatile unsigned short led_timer;


/* Global variables ------------------------------------------------------------*/
unsigned char relay_state = 0;
unsigned char last_edge;

unsigned char led_state = 0;
unsigned char blink = 0;

/* Module Functions ------------------------------------------------------------*/
unsigned char Switch (void)
{
	if (sw_filter >= SW_THRESH)
		return 1;
	else
		return 0;
}

//resetea el estado del LED
void ResetLed (void)
{
	led_state = START_BLINKING;
}

//mueve el LED segun el estado del Pote
void UpdateLed (unsigned char Pote)
{
	switch (led_state)
	{
		case START_BLINKING:
			blink = Pote;

			if (blink)
			{
				LED_ON;
				led_timer = 200;
				led_state++;
				blink--;
			}
			break;

		case WAIT_TO_OFF:
			if (!led_timer)
			{
				LED_OFF;
				led_timer = 200;
				led_state++;
			}
			break;

		case WAIT_TO_ON:
			if (!led_timer)
			{
				if (blink)
				{
					blink--;
					led_timer = 200;
					led_state = WAIT_TO_OFF;
					LED_ON;
				}
				else
				{
					led_state = WAIT_NEW_CYCLE;
					led_timer = 2000;
				}
			}
			break;

		case WAIT_NEW_CYCLE:
			if (!led_timer)
			{
				led_state = START_BLINKING;
			}
			break;

		default:
			led_state = START_BLINKING;
			break;
	}
}

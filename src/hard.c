/*
 * hard.c
 *
 *  Created on: 28/03/2017
 *      Author: Mariano
 */

#include "hard.h"
#include "tim.h"

/* Externals variables ---------------------------------------------------------*/
extern unsigned short timer_relay;
extern volatile unsigned short led_timer;


/* Global variables ------------------------------------------------------------*/
unsigned char relay_state = 0;
unsigned char last_edge;

unsigned char led_state = 0;
unsigned char blink = 0;

/* Module Functions ------------------------------------------------------------*/

//Pide conectar el relay
void RelayOn (void)
{
#ifdef HARD_2_0
	if (!RelayIsOn())
	{
		relay_state = ST_WAIT_ON;
		timer_relay = TT_RELAY;
	}
#endif
#ifdef HARD_1_0
	RELAY_ON;
#endif
}

//Pide desconectar el relay
void RelayOff (void)
{
#ifdef HARD_2_0
	if (!RelayIsOff())
	{
		relay_state = ST_WAIT_OFF;
		timer_relay = TT_RELAY;
	}
#endif
#ifdef HARD_1_0
	RELAY_OFF;
#endif

}

//Revisa el estado del relay
unsigned char RelayIsOn (void)
{
#ifdef HARD_2_0
	if ((relay_state == ST_WAIT_ON) ||
			(relay_state == ST_DELAYED_ON) ||
			(relay_state == ST_ON))
		return 1;
	else
		return 0;
#endif
#ifdef HARD_1_0
	if (RELAY)
		return 1;
	else
		return 0;
#endif
}

//Revisa el estado del relay
unsigned char RelayIsOff (void)
{
#ifdef HARD_2_0
	if ((relay_state == ST_WAIT_OFF) ||
			(relay_state == ST_DELAYED_OFF) ||
			(relay_state == ST_OFF))
		return 1;
	else
		return 0;
#endif
#ifdef HARD_1_0
	if (!RELAY)
		return 1;
	else
		return 0;
#endif

}

#ifdef HARD_2_0
//chequeo continuo del estado del relay
void UpdateRelay (void)
{
	unsigned char edge = 0;

	if ((!last_edge) && (EDGE_PIN))		//flanco ascendente detector
	{									//senoidal arriba
//		edge = 1;
		last_edge = 1;
		SYNC_ON;
	}

	if ((last_edge) && (!EDGE_PIN))		//flanco descendente detector
	{									//senoidal abajo
		edge = 1;
		last_edge = 0;
		SYNC_OFF;
	}

	switch (relay_state)
	{
		case ST_OFF:

			break;

		case ST_WAIT_ON:
			if (edge)
			{
				edge = 0;
				relay_state = ST_DELAYED_ON;
				TIM16->CNT = 0;
			}

			if (!timer_relay)		//agoto el timer y no encontro sincro, pega igual
			{
				RELAY_ON;
				relay_state = ST_ON;
			}
			break;

		case ST_DELAYED_ON:
			if (TIM16->CNT > TT_DELAYED_ON)
			{
				RELAY_ON;
				relay_state = ST_ON;
			}
			break;

		case ST_ON:

			break;

		case ST_WAIT_OFF:
			if (edge)
			{
				edge = 0;
				relay_state = ST_DELAYED_OFF;
				TIM16->CNT = 0;
			}

			if (!timer_relay)		//agoto el timer y no encontro sincro, despega igual
			{
				RELAY_OFF;
				relay_state = ST_OFF;
			}

			break;

		case ST_DELAYED_OFF:
			if (TIM16->CNT > TT_DELAYED_OFF)
			{
				RELAY_OFF;
				relay_state = ST_OFF;
			}
			break;

		default:
			RELAY_OFF;
			relay_state = ST_OFF;
			break;
	}
}
#endif

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

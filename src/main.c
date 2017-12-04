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
#include "stdio.h"
#include "adc.h"
#include "core_cm0.h"
#include "hard.h"
#include "tim.h"
#include "dsp.h"
#include "uart.h"


#ifdef SETPOINT_PLUS_HYST
#define HYST	7
#endif

//--- VARIABLES EXTERNAS ---//
volatile unsigned char timer_1seg = 0;
//volatile unsigned short timer_standby = 0;
//volatile unsigned short timer_led_comm = 0;
volatile unsigned char buffrx_ready = 0;
volatile unsigned char *pbuffrx;
volatile unsigned short timer_relay = 0;
volatile unsigned short wait_ms_var;
volatile unsigned short led_timer = 0;

//--- VARIABLES GLOBALES ---//
volatile unsigned char door_filter;
volatile unsigned char move_relay;

volatile unsigned short take_sample_pote_timer;
volatile unsigned short take_sample_temp_timer;
unsigned char pote_filter_ready = 0;
unsigned char temp_filter_ready = 0;


volatile unsigned char secs = 0;
volatile unsigned short minutes = 0;
volatile unsigned short bips_minutes_timeout = 0;
#ifdef DATALOGGER
volatile unsigned short timer_for_debug = 0;
#endif


#ifdef TEMP_BY_PWM
volatile unsigned short pwm_total_min = 0;
volatile unsigned short pwm_current_min = 0;
#endif

//--- FUNCIONES DEL MODULO ---//
unsigned char Door_Open (void);
unsigned short Get_Temp (void);
unsigned short Get_Pote (void);
void TimingDelay_Decrement(void);
void UpdatePote (void);
void UpdateTemp (void);
unsigned char GetPoteRange (unsigned short);
unsigned short AjustePuntas (unsigned short);


//--- FILTROS DE SENSORES ---//
#ifdef SOFT_2_0
#define LARGO_FILTRO_POTE 16
#define DIVISOR_POTE      4   //2 elevado al divisor = largo filtro
#define LARGO_FILTRO_TEMP 32
#define DIVISOR_TEMP      5   //2 elevado al divisor = largo filtro
unsigned short vtemp [LARGO_FILTRO_TEMP + 1];
unsigned short vpote [LARGO_FILTRO_POTE + 1];
#endif

#ifdef SOFT_2_1
unsigned char index_pote = 0;
unsigned char index_temp = 0;
unsigned short vtemp [LARGO_FILTRO_TEMP];
unsigned short vpote [LARGO_FILTRO_POTE];
#endif


//--- FIN DEFINICIONES DE FILTRO ---//
									 // MAX		<0	<3		<6	<9		<12		NUNCA
//const unsigned short vpote_ranges [] = {3584, 3072, 2560, 2048, 1536, 1024, 512, 0};
//const unsigned short vtemp_ranges [] = {3584, 3072, 2560, 2048, 1536, 1024, 512, 0};
const unsigned short vpote_ranges [] = {3510, 2925, 2340, 1755, 1170, 585, 0};
//const unsigned short vtemp_ranges [] = {2000, 1337, 1224, 1100, 930, 819, 0};
//const unsigned short vtemp_ranges [] = {1337, 713, 685, 657, 628, 600, 572};	//ajuste puntas 26-02
#ifdef SIMPLE_VECTOR_TEMP
//const unsigned short vtemp_ranges [] = {880, 816, 750, 687, 623, 559, 495};	//ajuste puntas 10-3-15 (mediciones 3-3-15)
const unsigned short vtemp_ranges [] = {796, 769, 742, 715, 688, 661, 495};	//ajuste puntas 10-3-15 (mediciones 3-3-15)
#endif



#ifdef TEMP_BY_PWM_AND_SENSE
#define B_INIT						0
#define B_CALENTANDO				1
#define B_ENFRIANDO_PWM			2
#define B_ENFRIANDO_SENSE		3
#define B_MIDIENDO				4
#endif


//-------------------------------------------//
// @brief  Main program.
// @param  None
// @retval None
//------------------------------------------//
int main(void)
{
	unsigned char i = 0;

	enum Parts Pote_Range;

	unsigned short new_sample;
	unsigned short temp_filtered = 0;
	unsigned short pote_filtered = 0;
	unsigned char stop_state = 0;
	unsigned char relay_was_on = 0;
#if (defined TEMP_BY_PWM_AND_SENSE)
	unsigned char bips_in_state = 0;
	unsigned char last_pote_range = 0;
#endif
	#ifdef SOFT_2_0
	unsigned char led_state = 0;
	unsigned char blink = 0;
	#endif

#ifdef DATALOGGER
	char s_to_send [100];
	unsigned short ts_cal1, ts_cal2;
#endif

//	unsigned char relay_was_on = 0;
#ifdef RELAY_OFF_WITH_DOOR_OPEN
	unsigned char door_is_open = 0;
#endif
	//!< At this stage the microcontroller clock setting is already configured,
    //   this is done through SystemInit() function which is called from startup
    //   file (startup_stm32f0xx.s) before to branch to application main.
    //   To reconfigure the default setting of SystemInit() function, refer to
    //   system_stm32f0xx.c file

	//GPIO Configuration.
	GPIO_Config();

	//TIM Configuration.
//	TIM_3_Init();
	TIM_16_Init();			//lo uso en HARD_2_0 para el sync de relay
	//Timer_2_Init();
	//Timer_3_Init();
	//Timer_4_Init();

	//UART configuration.
	//USART_Config()

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
	//SENSAR TEMPERATURA 	((OK))
	//ENVIAR ONE WIRE		((OK))

	//PRUEBA DE SYSTICK
	 /*
	 while(1)
	 {
		 if (LED_COMM)
			 LED_COMM_OFF;
		 else
			 LED_COMM_ON;

		 Delay(1);
	 }
	 */
	 //FIN PRUEBA DE SYSTICK

	//ADC configuration.
	AdcConfig();

	TIM16Enable ();

	LED_ON;
   Wait_ms(1000);
   LED_OFF;

#ifdef DATALOGGER
	USART1Config();
	//Activo sensor de temp lo hago en el config de ADC
	//calibracion de fabrica del sensor
	ts_cal1 = *((uint16_t*)0x1FFFF7B8);
	ts_cal2 = *((uint16_t*)0x1FFFF7C2);

#endif

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
#ifdef HARD_2_0
    //3 segundos muestro sincro
    timer_relay = 3000;
    while (timer_relay)
    {
//    	if (EDGE_PIN)
//    		LED_ON;
//    	else
//    		LED_OFF;

    	if ((!temp_filtered) && (EDGE_PIN))		//flanco ascendente detector
    	{									//senoidal arriba
    		temp_filtered = 1;
    		SYNC_ON;
    	}

    	if ((temp_filtered) && (!EDGE_PIN))		//flanco descendente detector
    	{									//senoidal abajo
    		temp_filtered = 0;
    		SYNC_OFF;
    		if (LED)
    			LED_OFF;
    		else
    			LED_ON;
    	}
    }
    LED_OFF;
#endif

	//--- New Main loop ---//
#ifdef SOFT_2_1
	new_sample = ReadADC1_SameSampleTime (CH_IN_POTE);
	for (i = 0; i < LARGO_FILTRO_POTE; i++)
		vpote[i] = new_sample;

	pote_filtered = MA16 (vpote);
	Pote_Range = GetPoteRange (pote_filtered);

	new_sample = ReadADC1_SameSampleTime (CH_IN_TEMP);
	for (i = 0; i < LARGO_FILTRO_TEMP; i++)
		vtemp[i] = new_sample;

	temp_filtered = MA32 (vtemp);
	stop_state = NORMAL;

	while (1)
	{
		//PROGRAMA DE PRODUCCION
		if (pote_filter_ready)
		{
			pote_filter_ready = 0;
			pote_filtered = MA16 (vpote);
			Pote_Range = GetPoteRange (pote_filtered);
		}

		switch (stop_state)
		{
			case NORMAL:		//se toman muestras de temp y se prende o apaga el relay
				if (temp_filter_ready)
				{
					temp_filter_ready = 0;
					temp_filtered = MA32 (vtemp);

#ifdef			MINIBAR
					//Ajustar Temperaturas
					temp_filtered = AjustePuntas (temp_filtered);
#endif

#if (defined TEMP_BY_PWM_AND_SENSE)
					if (Pote_Range != last_pote_range)
					{
						last_pote_range = Pote_Range;
						bips_in_state = B_INIT;
					}
#endif

					//Mover relay en funcion de posicion del Pote
					switch (Pote_Range)
					{
						case ZERO_BIPS:
							RelayOff();
							break;

						case ONE_BIP:
#ifdef TEMP_BY_NTC_INTERNO
							if (temp_filtered > TEMP_10_NTC_INTERNO)	//mide al reves menos temp mas tension
								RelayOff();

							if (temp_filtered < TEMP_12_NTC_INTERNO)
								RelayOn();
#endif
#ifdef TEMP_BY_PWM
							if (pwm_current_min < PWM_1BIP_ON)
								RelayOn();
							else
								RelayOff();

							if (pwm_current_min >= PWM_1BIP_PERIOD)
								pwm_current_min = 0;
#endif
#ifdef TEMP_BY_PWM_AND_SENSE
							switch (bips_in_state)
							{
								case B_INIT:
									RelayOn();
									bips_in_state = B_ENFRIANDO_PWM;
									// bips_minutes_timeout = PWM_1BIP_ON;
									bips_minutes_timeout = PWM_STARTING;
									break;

								case B_CALENTANDO:
									if (!bips_minutes_timeout)
									{
										RelayOn();
										bips_in_state = B_ENFRIANDO_PWM;
										// bips_minutes_timeout = PWM_1BIP_ON;
										bips_minutes_timeout = PWM_STARTING;
									}
									break;

								case B_ENFRIANDO_PWM:
									if (!bips_minutes_timeout)
									{
										bips_in_state = B_ENFRIANDO_SENSE;
										bips_minutes_timeout = 3 * PWM_1BIP_ON;
									}
									break;

								case B_ENFRIANDO_SENSE:
									if ((temp_filtered > TEMP_10)	|| (!bips_minutes_timeout)) //mide al reves menos temp mas tension
									{
										RelayOff();
										bips_in_state = B_CALENTANDO;
										bips_minutes_timeout = PWM_1BIP_OFF;
									}
									break;

								default:
									bips_in_state = B_INIT;
									break;
							}
#endif
							break;

						case TWO_BIPS:
#ifdef TEMP_BY_NTC_INTERNO
							if (temp_filtered > TEMP_08_NTC_INTERNO)	//mide al reves menos temp mas tension
								RelayOff();

							if (temp_filtered < TEMP_10_NTC_INTERNO)
								RelayOn();
#endif
#ifdef TEMP_BY_PWM
							if (pwm_current_min < PWM_2BIPS_ON)
								RelayOn();
							else
								RelayOff();

							if (pwm_current_min >= PWM_2BIPS_PERIOD)
								pwm_current_min = 0;
#endif
#ifdef TEMP_BY_PWM_AND_SENSE
							switch (bips_in_state)
							{
								case B_INIT:
									RelayOn();
									bips_in_state = B_ENFRIANDO_PWM;
									// bips_minutes_timeout = PWM_2BIPS_ON;
									bips_minutes_timeout = PWM_STARTING;
									break;

								case B_CALENTANDO:
									if (!bips_minutes_timeout)
									{
										RelayOn();
										bips_in_state = B_ENFRIANDO_PWM;
										// bips_minutes_timeout = PWM_2BIPS_ON;
										bips_minutes_timeout = PWM_STARTING;
									}
									break;

								case B_ENFRIANDO_PWM:
									if (!bips_minutes_timeout)
									{
										bips_in_state = B_ENFRIANDO_SENSE;
										bips_minutes_timeout = 3 * PWM_2BIPS_ON;
									}
									break;

								case B_ENFRIANDO_SENSE:
									if ((temp_filtered > TEMP_08)	|| (!bips_minutes_timeout)) //mide al reves menos temp mas tension
									{
										RelayOff();
										bips_in_state = B_CALENTANDO;
										bips_minutes_timeout = PWM_2BIPS_OFF;
									}
									break;

								default:
									bips_in_state = B_INIT;
									break;
							}
#endif
							break;

						case THREE_BIPS:
#ifdef TEMP_BY_NTC_INTERNO
							if (temp_filtered > TEMP_06_NTC_INTERNO)	//mide al reves menos temp mas tension
								RelayOff();

							if (temp_filtered < TEMP_08_NTC_INTERNO)
								RelayOn();
#endif
#ifdef TEMP_BY_PWM
							if (pwm_current_min < PWM_3BIPS_ON)
								RelayOn();
							else
								RelayOff();

							if (pwm_current_min >= PWM_3BIPS_PERIOD)
								pwm_current_min = 0;
#endif
#ifdef TEMP_BY_PWM_AND_SENSE
							switch (bips_in_state)
							{
								case B_INIT:
									RelayOn();
									bips_in_state = B_ENFRIANDO_PWM;
									// bips_minutes_timeout = PWM_3BIPS_ON;
									bips_minutes_timeout = PWM_STARTING;
									break;

								case B_CALENTANDO:
									if (!bips_minutes_timeout)
									{
										RelayOn();
										bips_in_state = B_ENFRIANDO_PWM;
										// bips_minutes_timeout = PWM_3BIPS_ON;
										bips_minutes_timeout = PWM_STARTING;
									}
									break;

								case B_ENFRIANDO_PWM:
									if (!bips_minutes_timeout)
									{
										bips_in_state = B_ENFRIANDO_SENSE;
										bips_minutes_timeout = 3 * PWM_3BIPS_ON;
									}
									break;

								case B_ENFRIANDO_SENSE:
									if ((temp_filtered > TEMP_06)	|| (!bips_minutes_timeout)) //mide al reves menos temp mas tension
									{
										RelayOff();
										bips_in_state = B_CALENTANDO;
										bips_minutes_timeout = PWM_3BIPS_OFF;
									}
									break;

								default:
									bips_in_state = B_INIT;
									break;
							}
#endif
							break;

						case FOUR_BIPS:
#ifdef TEMP_BY_NTC_INTERNO
							if (temp_filtered > TEMP_04_NTC_INTERNO)	//mide al reves menos temp mas tension
								RelayOff();

							if (temp_filtered < TEMP_06_NTC_INTERNO)
								RelayOn();
#endif
#ifdef TEMP_BY_PWM
							if (pwm_current_min < PWM_4BIPS_ON)
								RelayOn();
							else
								RelayOff();

							if (pwm_current_min >= PWM_4BIPS_PERIOD)
								pwm_current_min = 0;
#endif
#ifdef TEMP_BY_PWM_AND_SENSE
							switch (bips_in_state)
							{
								case B_INIT:
									RelayOn();
									bips_in_state = B_ENFRIANDO_PWM;
									// bips_minutes_timeout = PWM_4BIPS_ON;
									bips_minutes_timeout = PWM_STARTING;
									break;

								case B_CALENTANDO:
									if (!bips_minutes_timeout)
									{
										RelayOn();
										bips_in_state = B_ENFRIANDO_PWM;
										// bips_minutes_timeout = PWM_4BIPS_ON;
										bips_minutes_timeout = PWM_STARTING;
									}
									break;

								case B_ENFRIANDO_PWM:
									if (!bips_minutes_timeout)
									{
										bips_in_state = B_ENFRIANDO_SENSE;
										bips_minutes_timeout = 3 * PWM_4BIPS_ON;
									}
									break;

								case B_ENFRIANDO_SENSE:
									if ((temp_filtered > TEMP_04)	|| (!bips_minutes_timeout)) //mide al reves menos temp mas tension
									{
										RelayOff();
										bips_in_state = B_CALENTANDO;
										bips_minutes_timeout = PWM_4BIPS_OFF;
									}
									break;

								default:
									bips_in_state = B_INIT;
									break;
							}
#endif
							break;

						case FIVE_BIPS:
#ifdef TEMP_BY_NTC_INTERNO
							if (temp_filtered > TEMP_02_NTC_INTERNO)	//mide al reves menos temp mas tension
								RelayOff();

							if (temp_filtered < TEMP_04_NTC_INTERNO)
								RelayOn();
#endif
#ifdef TEMP_BY_PWM
							if (pwm_current_min < PWM_5BIPS_ON)
								RelayOn();
							else
								RelayOff();

							if (pwm_current_min >= PWM_5BIPS_PERIOD)
								pwm_current_min = 0;
#endif
#ifdef TEMP_BY_PWM_AND_SENSE
							switch (bips_in_state)
							{
								case B_INIT:
									RelayOn();
									bips_in_state = B_ENFRIANDO_PWM;
									// bips_minutes_timeout = PWM_5BIPS_ON;
									bips_minutes_timeout = PWM_STARTING;
									break;

								case B_CALENTANDO:
									if (!bips_minutes_timeout)
									{
										RelayOn();
										bips_in_state = B_ENFRIANDO_PWM;
										// bips_minutes_timeout = PWM_5BIPS_ON;
										bips_minutes_timeout = PWM_STARTING;
									}
									break;

								case B_ENFRIANDO_PWM:
									if (!bips_minutes_timeout)
									{
										bips_in_state = B_ENFRIANDO_SENSE;
										bips_minutes_timeout = 3 * PWM_5BIPS_ON;
									}
									break;

								case B_ENFRIANDO_SENSE:
									if ((temp_filtered > TEMP_02)	|| (!bips_minutes_timeout)) //mide al reves menos temp mas tension
									{
										RelayOff();
										bips_in_state = B_CALENTANDO;
										bips_minutes_timeout = PWM_5BIPS_OFF;
									}
									break;

								default:
									bips_in_state = B_INIT;
									break;
							}
#endif
							break;

						case SIX_BIPS:
							RelayOn();
							break;
					}
				}

#ifndef TEMP_BY_PWM_AND_SENSE
				if (minutes >= TT_MINUTES_DAY_ON)
					stop_state = GO_TO_STOP;
#endif

				break;

			case GO_TO_STOP:
				//tengo que apagar el rele durante 25 minutos
				minutes = 0;
				RelayOff();
				stop_state = STOPPED;
				LED_ON;
				break;

			case STOPPED:
				if (minutes >= TT_MINUTES_DAY_OFF)
				{
					stop_state = NORMAL;
					minutes = 0;
				}
				break;
		}

		//Cuestiones generales que no tienen que ver con el modo de stop
#ifdef RELAY_ALWAYS_ON
		if (stop_state == NORMAL)
			UpdateLed((unsigned char) Pote_Range);

		if (Door_Open())
			LIGHT_ON;
		else
			LIGHT_OFF;


#elif defined RELAY_OFF_WITH_DOOR_OPEN

		if (Door_Open())
		{
			if (RelayIsOn())
			{
				RelayOff();
				relay_was_on = 1;
			}

			LED_OFF;
			door_is_open = 1;
			LIGHT_ON;
		}
		else
		{
			if (door_is_open)
				ResetLed();

			if (relay_was_on)
			{
				relay_was_on = 0;
				RelayOn();
			}
			LIGHT_OFF;
			door_is_open = 0;
		}

		if ((stop_state == NORMAL) && (!door_is_open))		//si a puerta no esta abierta muevo LED
			UpdateLed((unsigned char) Pote_Range);

#else
#error "Falta elegir Type of Program hard.h"
#endif

		//Cuestiones generales a todos los programas
		UpdatePote();
		UpdateTemp();

#ifdef HARD_2_0
		UpdateRelay();
#endif
#ifdef DATALOGGER
		if (!timer_for_debug)
		{
			timer_for_debug = 10000;
			new_sample = ReadADC1_SameSampleTime (CH_IN_INTERNAL_TEMP);
			if (RelayIsOn())
				sprintf(s_to_send, "%04d,%04d,1,\r\n",temp_filtered,new_sample);
			else
				sprintf(s_to_send, "%04d,%04d,0,\r\n",temp_filtered,new_sample);

			Usart1Send(s_to_send);
		}
#endif
	}	//End of while 1
#endif		//SOFT_2_1

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

	//--- Main loop ---//
#ifdef SOFT_2_0
	while(1)
	{
		//PROGRAMA DE PRODUCCION
		if (!take_sample_pote_timer)	//tomo muestras cada 10ms
		{
			take_sample_pote_timer = 10;
			pote_filtered = Get_Pote();

			//determino los rangos del pote
			if (pote_filtered > vpote_ranges[0])
				Pote_Range = SIX_BIPS;
			else if (pote_filtered > vpote_ranges[1])
				Pote_Range = FIVE_BIPS;
			else if (pote_filtered > vpote_ranges[2])
				Pote_Range = FOUR_BIPS;
			else if (pote_filtered > vpote_ranges[3])
				Pote_Range = THREE_BIPS;
			else if (pote_filtered > vpote_ranges[4])
				Pote_Range = TWO_BIPS;
			else if (pote_filtered > vpote_ranges[5])
				Pote_Range = ONE_BIP;
			else
				Pote_Range = ZERO_BIPS;
		}

		if (!take_sample_temp_timer)	//tomo muestras cada 100ms
		{
			take_sample_temp_timer = 10;
			temp_filtered = Get_Temp();

#ifdef SIMPLE_VECTOR_TEMP
			//determino los rangos de temperatura
			if (temp_filtered > vtemp_ranges[0])		//1337
				Temp_Range = SIX_BIPS;
			else if (temp_filtered > vtemp_ranges[1])	//713
				Temp_Range = FIVE_BIPS;
			else if (temp_filtered > vtemp_ranges[2])	//685
				Temp_Range = FOUR_BIPS;
			else if (temp_filtered > vtemp_ranges[3])	//657
				Temp_Range = THREE_BIPS;
			else if (temp_filtered > vtemp_ranges[4])	//628
				Temp_Range = TWO_BIPS;
			else if (temp_filtered > vtemp_ranges[5])	//600
				Temp_Range = ONE_BIP;
			else
				Temp_Range = ZERO_BIPS;				//572
#endif


		}

#ifdef RELAY_OFF_WITH_DOOR_OPEN
		if (Door_Open())
		{
			RelayOff();
			LED_OFF;
			door_is_open = 1;
			LIGHT_ON;
		}
		else
		{
			LIGHT_OFF;
			door_is_open = 0;
		}
#else
		if (Door_Open())
		{
			//LED_OFF;
			LIGHT_ON;
		}
		else
		{
			LIGHT_OFF;
		}
#endif

#ifdef RELAY_OFF_WITH_DOOR_OPEN
		if (!door_is_open)
		{
#endif
			switch (stop_state)
			{
				case NORMAL:
					if (move_relay == 0)		//el RELE lo muevo cada 10 segundos
					{
						move_relay = 10;

	#ifdef SIMPLE_VECTOR_TEMP
						//Modificacion 10-3-15 pongo histeresis de 1 paso completo
						if (Temp_Range >= Pote_Range)
							RELAY_OFF;

						if (Pote_Range > ZERO_BIPS)
						{
							if (Temp_Range < (Pote_Range - 1))
								RELAY_ON;
						}
	#endif

	#ifdef OPEN_LOOP
						//Modificacion 23-5-15 pongo setpoint + hysteresis
						switch (Pote_Range)
						{
							case SIX_BIPS:		//lo resuelvo en otra parte
								break;

							case FIVE_BIPS:
								if (pwm_current_min < vpwm_ranges[FIVE_BIPS])
									RelayOn();
								else
									RelayOff();

								if (pwm_current_min >= PWM_MIN_MAX)
								{
									pwm_current_min = 0;
								}
								break;

							case FOUR_BIPS:
								if (pwm_current_min < vpwm_ranges[FOUR_BIPS])
									RelayOn();
								else
									RelayOff();

								if (pwm_current_min >= PWM_MIN_MAX)
								{
									pwm_current_min = 0;
								}
								break;

							case THREE_BIPS:
								if (pwm_current_min < vpwm_ranges[THREE_BIPS])
									RelayOn();
								else
									RelayOff();

								if (pwm_current_min >= PWM_MIN_MAX)
								{
									pwm_current_min = 0;
								}
								break;

							case TWO_BIPS:
								if (pwm_current_min < vpwm_ranges[TWO_BIPS])
									RelayOn();
								else
									RelayOff();

								if (pwm_current_min >= PWM_MIN_MAX)
								{
									pwm_current_min = 0;
								}
								break;

							case ONE_BIP:
								if (pwm_current_min < vpwm_ranges[ONE_BIP])
									RelayOn();
								else
									RelayOff();

								if (pwm_current_min >= PWM_MIN_MAX)
								{
									pwm_current_min = 0;
								}
								break;

							case ZERO_BIPS:		//lo resuelvo en otra parte
								break;
						}
	#endif //OPEN_LOOP
					}	//end move_relay

					if (minutes >= TT_MINUTES_DAY_ON)
						stop_state = GO_TO_STOP;

					//si se apago la heladera
					if (Pote_Range == ZERO_BIPS)
						stop_state = TO_NEVER;

					//si se prende siempre
					if (Pote_Range == SIX_BIPS)
						stop_state = TO_ALWAYS;

					break;

				case GO_TO_STOP:
					//tengo que apagar el rele durante 25 minutos
					minutes = 0;
					RelayOff();
					stop_state = STOPPED;
					break;

				case STOPPED:
					if (minutes >= TT_MINUTES_DAY_OFF)
					{
						stop_state = NORMAL;
						minutes = 0;
						pwm_current_min = 0;
					}
					break;

				case TO_NEVER:
					//apago el motor
					RelayOff();
					stop_state = NEVER;
					break;

				case NEVER:
					//mantengo motor apagado mientras este en NEVER
//					if (RELAY)
//						RelayOff();

					if (Pote_Range != ZERO_BIPS)
					{
						minutes = 0;
						stop_state = NORMAL;
						pwm_current_min = 0;
					}
					break;

				case TO_ALWAYS:
					RelayOn();
					stop_state = ALWAYS;
					break;

				case ALWAYS:
					if (Pote_Range != SIX_BIPS)
					{
						stop_state = NORMAL;
					}

#ifdef RELAY_OFF_WITH_DOOR_OPEN
					if (!RelayIsOn())		//agregado pos si abren la puerta
						RelayOn();
#endif

					if (minutes >= TT_MINUTES_DAY_ON)
						stop_state = GO_TO_STOP;

					break;

				default:
					stop_state = NORMAL;
					break;
			}
#ifdef RELAY_OFF_WITH_DOOR_OPEN
		}
#endif

#ifdef RELAY_OFF_WITH_DOOR_OPEN
		if (!door_is_open)
		{
#endif
			switch (led_state)
			{
				case START_BLINKING:
					blink = (unsigned char) Pote_Range;

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
#ifdef RELAY_OFF_WITH_DOOR_OPEN
		}
#endif
		//Cuestiones generales
#ifdef HARD_2_0
		UpdateRelay();
#endif

	}	//End of while (1)
#endif	//SOFT_2_0
	return 0;
}
//--- End of file ---//

unsigned short Get_Temp (void)
{
	unsigned int total_ma;
	unsigned char j;

	//Kernel mejorado ver 2
	//si el vector es de 0 a 7 (+1) sumo todas las posiciones entre 1 y 8, acomodo el nuevo vector entre 0 y 7
	total_ma = 0;
	//vtemp[LARGO_FILTRO_TEMP] = ReadADC1 (CH_IN_TEMP);
	vtemp[LARGO_FILTRO_TEMP] = ReadADC1_SameSampleTime (CH_IN_TEMP);
    for (j = 0; j < (LARGO_FILTRO_TEMP); j++)
    {
    	total_ma += vtemp[j + 1];
    	vtemp[j] = vtemp[j + 1];
    }

    return total_ma >> DIVISOR_TEMP;
}

unsigned short Get_Pote (void)
{
	unsigned int total_ma;
	unsigned char j;

	//Kernel mejorado ver 2
	//si el vector es de 0 a 7 (+1) sumo todas las posiciones entre 1 y 8, acomodo el nuevo vector entre 0 y 7
	total_ma = 0;
	//vpote[LARGO_FILTRO_POTE] = ReadADC1 (CH_IN_POTE);
	vpote[LARGO_FILTRO_POTE] = ReadADC1_SameSampleTime (CH_IN_POTE);
    for (j = 0; j < (LARGO_FILTRO_POTE); j++)
    {
    	total_ma += vpote[j + 1];
    	vpote[j] = vpote[j + 1];
    }

    return total_ma >> DIVISOR_POTE;
}

unsigned char Door_Open (void)
{
	if (door_filter >= DOOR_THRESH)
		return 1;
	else
		return 0;
}

void UpdatePote (void)
{
	unsigned short new_sample;

	if (!take_sample_pote_timer)	//tomo muestras cada UPDATE_FILTRO_POTE
	{
		take_sample_pote_timer = UPDATE_FILTRO_POTE;
		new_sample = ReadADC1_SameSampleTime (CH_IN_POTE);

		if (index_pote < LARGO_FILTRO_POTE)
		{
			vpote[index_pote] = new_sample;
			index_pote++;
		}
		else
		{
			vpote[0] = new_sample;
			index_pote = 1;
			pote_filter_ready = 1;
		}
	}
}

void UpdateTemp (void)
{
	unsigned short new_sample;

	if (!take_sample_temp_timer)	//tomo muestras cada UPDATE_FILTRO_TEMP
	{
		take_sample_temp_timer = UPDATE_FILTRO_TEMP;
		new_sample = ReadADC1_SameSampleTime (CH_IN_TEMP);

		if (index_temp < LARGO_FILTRO_TEMP)
		{
			vtemp[index_temp] = new_sample;
			index_temp++;
		}
		else
		{
			vtemp[0] = new_sample;
			index_temp = 1;
			temp_filter_ready = 1;
		}
	}
}

unsigned char GetPoteRange (unsigned short filtered_sample)
{
	pote_range_t pote;

	//determino los rangos del pote
	if (filtered_sample > vpote_ranges[0])
		pote = SIX_BIPS;
	else if (filtered_sample > vpote_ranges[1])
		pote = FIVE_BIPS;
	else if (filtered_sample > vpote_ranges[2])
		pote = FOUR_BIPS;
	else if (filtered_sample > vpote_ranges[3])
		pote = THREE_BIPS;
	else if (filtered_sample > vpote_ranges[4])
		pote = TWO_BIPS;
	else if (filtered_sample > vpote_ranges[5])
		pote = ONE_BIP;
	else
		pote = ZERO_BIPS;

	return (unsigned char) pote;
}

unsigned short AjustePuntas (unsigned short t_filtered)
{
	float aux = 1.0;
	aux = t_filtered * 2.99 - 893.0;
	return (unsigned short) aux;
	//ntc_ext = ntc_ext * 2.99 - 893.0	#contra eje x int eje y ext	PYTHON
}



/**
  * @brief  Decrements the TimingDelay variable.	ESTA ES LA QUE LLAMA SYSTICK CADA 1MSEG
  * @param  None
  * @retval None
  */
volatile unsigned short relay_dumb = 0;

void TimingDelay_Decrement(void)
{
	if (wait_ms_var)
		wait_ms_var--;

	//filtro de ruido para la puerta
	if (DOOR)
	{
		if (door_filter < DOOR_ROOF)
			door_filter++;
	}
	else if (door_filter)
		door_filter--;

	//indice para el filtro del pote
	if (take_sample_pote_timer)
		take_sample_pote_timer--;

	//indice para el filtro de la temp
	if (take_sample_temp_timer)
		take_sample_temp_timer--;

	if (relay_dumb)		//entro cada 1 segundo
		relay_dumb--;
	else
	{
		relay_dumb = 1000;
		if (move_relay)
			move_relay--;

		if (secs < 60)
			secs++;
		else
		{
			secs = 0;
			minutes++;
#ifdef TEMP_BY_PWM
			pwm_current_min++;		//contador de minutos
#endif

#ifdef TEMP_BY_PWM_AND_SENSE
			if (bips_minutes_timeout)
				bips_minutes_timeout--;
#endif
		}
	}

	if (timer_relay)
		timer_relay--;

	if (led_timer)
		led_timer--;

#ifdef DATALOGGER
	if (timer_for_debug)
		timer_for_debug--;
#endif
}

/**
  ******************************************************************************
  * @file    Template_2/stm32f0_uart.c
  * @author  Nahuel
  * @version V1.0
  * @date    22-August-2014
  * @brief   UART functions.
  ******************************************************************************
  * @attention
  *
  * Use this functions to configure serial comunication interface (UART).
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hard.h"
#include "stm32f0xx.h"
#include "uart.h"
// #include "dmx_transceiver.h"

#ifdef USE_HLK_WIFI
#include "HLK_RM04.h"
#endif
#ifdef USE_ESP_WIFI
#include "ESP8266.h"
#endif

#include <string.h>

#define SIZEOF_DATA	128


//--- Private typedef ---//
//--- Private define ---//
//--- Private macro ---//

//#define USE_USARTx_TIMEOUT



//--- VARIABLES EXTERNAS ---//
volatile unsigned char tx1buff[SIZEOF_DATA];
volatile unsigned char rx1buff[SIZEOF_DATA];

volatile unsigned char tx2buff[SIZEOF_DATA];
volatile unsigned char rx2buff[SIZEOF_DATA];

//--- Private variables ---//
volatile unsigned char * ptx1;
volatile unsigned char * ptx1_pckt_index;

volatile unsigned char * ptx2;
volatile unsigned char * ptx2_pckt_index;

//Reception buffer.

//Transmission buffer.

//--- Private function prototypes ---//
//--- Private functions ---//


void USART1_IRQHandler(void)
{
	unsigned char dummy;

	/* USART in mode Receiver --------------------------------------------------*/
	if (USART1->ISR & USART_ISR_RXNE)
	{

		//RX DMX
		dummy = USART1->RDR & 0x0FF;

	}

	/* USART in mode Transmitter -------------------------------------------------*/
	if (USART1->CR1 & USART_CR1_TXEIE)
	{
		if (USART1->ISR & USART_ISR_TXE)
		{
			if ((ptx1 < &tx1buff[SIZEOF_DATA]) && (ptx1 < ptx1_pckt_index))
			{
				USART1->TDR = *ptx1;
				ptx1++;
			}
			else
			{
				ptx1 = tx1buff;
				ptx1_pckt_index = tx1buff;
				USART1->CR1 &= ~USART_CR1_TXEIE;
			}
		}
	}

	if ((USART1->ISR & USART_ISR_ORE) || (USART1->ISR & USART_ISR_NE) || (USART1->ISR & USART_ISR_FE))
	{
		USART1->ICR |= 0x0e;
		dummy = USART1->RDR;
	}
}

void USART2_IRQHandler(void)
{
	unsigned char dummy;

	/* USART in mode Receiver --------------------------------------------------*/
	if (USART2->ISR & USART_ISR_RXNE)
	{
		//RX WIFI
		dummy = USART2->RDR & 0x0FF;

	}
	/* USART in mode Transmitter -------------------------------------------------*/

	if (USART2->CR1 & USART_CR1_TXEIE)
	{
		if (USART2->ISR & USART_ISR_TXE)
		{
			if ((ptx2 < &tx2buff[SIZEOF_DATA]) && (ptx2 < ptx2_pckt_index))
			{
				USART2->TDR = *ptx2;
				ptx2++;
			}
			else
			{
				ptx2 = tx2buff;
				ptx2_pckt_index = tx2buff;
				USART2->CR1 &= ~USART_CR1_TXEIE;
			}
		}
	}

	if ((USART2->ISR & USART_ISR_ORE) || (USART2->ISR & USART_ISR_NE) || (USART2->ISR & USART_ISR_FE))
	{
		USART2->ICR |= 0x0e;
		dummy = USART2->RDR;
	}
}

void Usart1Send (char * send)
{
	unsigned char i;

	i = strlen(send);
	Usart1SendUnsigned((unsigned char *) send, i);
}

void Usart1SendUnsigned(unsigned char * send, unsigned char size)
{
	if ((ptx1_pckt_index + size) < &tx1buff[SIZEOF_DATA])
	{
		memcpy((unsigned char *)ptx1_pckt_index, send, size);
		ptx1_pckt_index += size;
		USART1->CR1 |= USART_CR1_TXEIE;
	}
}

void Usart2Send (char * send)
{
	unsigned char i;

	i = strlen(send);
	Usart2SendUnsigned((unsigned char *) send, i);
}

void Usart2SendUnsigned(unsigned char * send, unsigned char size)
{
	if ((ptx2_pckt_index + size) < &tx2buff[SIZEOF_DATA])
	{
		memcpy((unsigned char *)ptx2_pckt_index, send, size);
		ptx2_pckt_index += size;
		USART2->CR1 |= USART_CR1_TXEIE;
	}
}

void USART2Config(void)
{
	if (!USART2_CLK)
		USART2_CLK_ON;

	GPIOA->AFR[0] |= 0x0001100;	//PA2 -> AF1 PA3 -> AF1

	USART2->BRR = USART_9600;
#ifdef DATALOGGER
	//doy vuelta el pin TX
	USART2->CR2 |= USART_CR2_TXINV;
#endif
	USART2->CR1 = USART_CR1_RXNEIE | USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;

	ptx2 = tx2buff;
	ptx2_pckt_index = tx2buff;
	//prx2 = rx2buff;
	NVIC_EnableIRQ(USART2_IRQn);
	NVIC_SetPriority(USART2_IRQn, 7);
}

void USART1Config(void)
{
	if (!USART1_CLK)
		USART1_CLK_ON;

	GPIOA->AFR[1] |= 0x00000010;	//PA9 -> AF1

	USART1->BRR = USART_9600_8MHZ;

	//doy vuelta el pin TX
	USART1->CR2 |= USART_CR2_TXINV;


//	USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
//	USART1->CR1 = USART_CR1_RXNEIE | USART_CR1_RE | USART_CR1_UE;	//SIN TX
	USART1->CR1 = USART_CR1_TE | USART_CR1_UE;	//para pruebas TX

	ptx1 = tx1buff;
	ptx1_pckt_index = tx1buff;

	NVIC_EnableIRQ(USART1_IRQn);
	NVIC_SetPriority(USART1_IRQn, 5);
}


//--- end of file ---//

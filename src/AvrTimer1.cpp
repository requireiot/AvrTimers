/**
 * @file 		  AvrTimer1.cpp
 * Author		: Bernd Waldmann
 * Created		: 7-Mar-2020
 * Tabsize		: 4
 *
 * This Revision: $Id: AvrTimer1.cpp 1199 2021-07-24 10:25:25Z  $
 *
 * @brief  Abstraction for 16-bit AVR Timer/Counter 1. 
 */

/*
   Copyright (C)2021 Bernd Waldmann

   This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
   If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

   SPDX-License-Identifier: MPL-2.0
*/

#include <util/atomic.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

#include "stdpins.h"
#include "debugstream.h"
#include "AvrTimers.h"

#define T1WGM 14

#define PIN_OC1A	_OC1A(ACTIVE_HIGH)
#define PIN_OC1B	_OC1B(ACTIVE_HIGH)

//---------------------------------------------------------------------------

AvrTimer1* AvrTimer1::theInstance = NULL;

ISR(TIMER1_OVF_vect)
{
	sei();
	AvrTimer1::theInstance->call_tasks();
}

//---------------------------------------------------------------------------

/** 
 * @addtogroup AvrTimers 
 * @{ 
 */

/**
 * @brief Constructor, initialize timer variables
 */
AvrTimer1::AvrTimer1(void) : AvrTimerBase(), 
	m_enableA(false), m_enableB(false)
{
	AvrTimer1::theInstance = this;
}

//---------------------------------------------------------------------------

#if 0
/** 
 * @brief Initialize TC1 registers for periodic interrupt, but do not start. 
 * 
 * @param rate = desired interrupt rate in Hz
 * @param polA = desired polarity of OCR1A pin (active high or low or disabled)
 * @param polB = desired polarity of OCR1B pin (active high or low or disabled)
 * @return actual interrupt rate in Hz
*/
uint32_t AvrTimer1::init(uint32_t rate, Polarity polA, Polarity polB )
{
	const uint32_t fclk = F_CPU;
	uint32_t top;
	// prescaler values for TC1, indexed by CS
	static uint16_t prescalers[] = { 0,1,8,64,256,1024 };
	#define lengthof(x) (sizeof((x))/sizeof((x)[0]))

	uint8_t cs = 0;
	do {
		top = fclk / (rate * (long)prescalers[++cs]);
	} while (cs < lengthof(prescalers)-1 && top > 65535uL );
	if (top > 65535uL) {
		top = 65535uL;
		cs=0;
		DEBUG_PRINTF("T1 rate too low: %lu",rate);
	}
	
	return init(cs,top,polA,polB);
}
#endif 

/**
 * @brief Initialize TC1 registers for periodic interrupt, but do not start.
 * Typically called from begin()
 * @return uint32_t  actual rate [Hz] or 0 if the rate can't be achieved
 */
uint32_t AvrTimer1::init(
	uint8_t cs, 	///< clock select, 1..5 or 0 for error
	uint16_t ocr, 	///< value for OCR0A, 0..65535
	Polarity polA, 	///< polarity of PWM output OC1A
	Polarity polB 	///< polarity of PWM output OC1B
	)
{
	const uint32_t fclk = F_CPU;

	if (cs==0) {	// T1 rate too low
		return 0;
	}

    PRR &= ~_BV(PRTIM1);

	m_polA = polA;
	m_polB = polB;
	m_comA = polA ? ((polA==ActiveHigh) ? 2 : 3 ) : 0;
	m_comB = polB ? ((polB==ActiveHigh) ? 2 : 3 ) : 0;

	static const uint32_t T1_div[] = { 0,1,8,64,256,1024 };
	uint32_t arate = fclk / (T1_div[cs] * ocr);
	
	TIMSK1 = 0;						// disable all T1 interrupts

	// Control Register A for Timer/Counter-1 (Timer/Counter-1 is configured using two registers: A and B)
	// TCCR1A is [COM1A1:COM1A0:COM1B1:COM1B0:unused:unused:WGM01:WGM00]
	TCCR1A	= m_comA << COM1A0		 // COM1A[1:0]=2 : clear OC1A on compare-match, and sets OC1A at BOTTOM
			| m_comB << COM1B0
			| (T1WGM & 3) << WGM10
			;

	// Control Register B for Timer/Counter-0 (Timer/Counter-0 is configured using two registers: A and B)
	// TCCR0B is [FOC0A:FOC0B:unused:unused:WGM02:CS02:CS01:CS00]
	TCCR1B	= (T1WGM>>2) << WGM12
			| 1 << CS10				// CS1[2:0]=1  clock is ClkIO/1 = 1 MHz = 100 Hz PWM reload rate
			;
	ICR1 = m_top = ocr-1;                // initial state is 100% on

	TIFR1  = 0xFF;	// clear all interrupts

#if DEBUG_AVRTIMERS

	if (arate <= 1000) {
		m_MillisPerTick = 1000 / arate;
		m_TicksPerMilli = 1;
	} else {
		m_TicksPerMilli = arate / 1000;
		m_MillisPerTick = 0;
	}

	DEBUG_PRINTF(" T1: F=%lu, CS=%u, TOP=%u, rate %lu, ",
		fclk, cs, m_top, arate);
	DEBUG_PRINTF("%u %s\r\n",
		m_MillisPerTick ? m_MillisPerTick : m_TicksPerMilli,
		m_MillisPerTick ? "ms/t" : "t/ms" );

#endif // DEBUG_AVRTIMERS

	return arate;	
}


/** @brief start TC1 interrupts. */
void AvrTimer1::start(void)
{
	TIFR1  = _BV(TOIE1); 		// clear interrupt
	TIMSK1 = _BV(TOIE1);	    	// enable overflow interrupt
}


/** @brief stop TC1 interrupts. */
void AvrTimer1::stop(void)
{
	TIMSK1 &= ~_BV(TOIE1);						// enable overflow interrupt
}


void AvrTimer1::setCR()
{
	TCCR1A	= (m_enableA ? m_comA : 0) << COM1A0		 // COM1A[1:0]=2 : clear OC1A on compare-match, and sets OC1A at BOTTOM
			| (m_enableB ? m_comB : 0) << COM1B0
			| (T1WGM & 3) << WGM10
			;
}


/** @brief set PWM duty cycle on OCR1A
 * @param pwm  duty cycle between 0 and `top`
 * @param top  range of values for `pwm`
 */
void AvrTimer1::setPWM_A(uint16_t pwm, uint16_t top)
{
	if (pwm) {
		m_enableA = true;
		uint16_t ocr = ((uint32_t)pwm * m_top) / top;
		OCR1A = ocr;
	} else {
		m_enableA = false;
		SET( PIN_OC1A, m_comA==3);
	}
	setCR();
}


/** @brief set PWM duty cycle on OCR1B
 * @param pwm  duty cycle between 0 and `top`
 * @param top  range of values for `pwm`
 */
void AvrTimer1::setPWM_B(uint16_t pwm, uint16_t top )
{
	if (pwm) {
		m_enableB = true;
		uint16_t ocr = ((uint32_t)pwm * m_top) / top;
		OCR1B = ocr;
	} else {
		m_enableB = false;
		SET( PIN_OC1B, m_comB==3);
	}
	setCR();
}

/** @} */

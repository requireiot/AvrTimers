/**
 * @file 		  AvrTimer0.cpp
 * Author		: Bernd Waldmann
 * Created		: 03-Oct-2019
 * Tabsize		: 4
 *
 * This Revision: $Id: AvrTimer0.cpp 1199 2021-07-24 10:25:25Z  $
 *
 * @brief  Abstraction for AVR Timer/Counter 0, for periodic interrupts and PWM.
 */ 

/*
   Copyright (C) 2019,2021 Bernd Waldmann

   This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
   If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

   SPDX-License-Identifier: MPL-2.0
*/

#include <util/atomic.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "debugstream.h"
#include "stdpins.h"
#include "AvrTimers.h"


AvrTimer0* AvrTimer0::theInstance = NULL;

#define T0WGM 7

#define PIN_OC0A	_OC0A(ACTIVE_HIGH)
#define PIN_OC0B	_OC0B(ACTIVE_HIGH)

//---------------------------------------------------------------------------

ISR (TIMER0_COMPA_vect)
{
	sei();
	AvrTimer0::theInstance->call_tasks();
}

/** 
 * @addtogroup AvrTimers
 * @{ 
 */

//---------------------------------------------------------------------------

AvrTimer0::AvrTimer0(void) : AvrTimerBase()
{
	AvrTimer0::theInstance = this;
}

//---------------------------------------------------------------------------

/** @brief start TC0 interrupts. */
void AvrTimer0::start(void)
{
	TIFR0  = _BV(OCF0A);						// clear interrupt
	TIMSK0 = _BV(OCIE0A);						// enable output compare match A interrupt
}

//---------------------------------------------------------------------------

/** @brief stop TC0 interrupts. */
void AvrTimer0::stop(void)
{
	TIMSK0 &= ~_BV(OCIE0A);						// enable overflow interrupt
}

//---------------------------------------------------------------------------

/** 
 * @brief set PWM duty cycle on OCR0B
 * @param pwm  duty cycle between 0 and `top`
 * @param top  range of values for `pwm`
 */
void AvrTimer0::setPWM_B(uint8_t pwm, uint8_t top )
{
	if (pwm) {
		m_enableB = true;
		uint16_t ocr = ((uint16_t)pwm * (uint16_t) m_ocr) / (uint16_t) top;
		OCR0B = ocr;
        //DEBUG_PRINTF("  T0: pwm=%d, ocrA=%d is %d, ocrB=%d is %d\r\n",pwm,m_ocr,OCR0A,ocr,OCR0B);
	} else {
		m_enableB = false;
		SET( PIN_OC0B, m_comB==3 );
	}
	setCR();
}

//---------------------------------------------------------------------------

/** 
 * @brief Initialize TC0 registers for periodic interrupt, but do not start. 
 * Typically called from begin()
 * @return actual rate [Hz] or 0 if the rate can't be achieved
*/
uint32_t AvrTimer0::init(
	uint8_t cs,		///< clock select, 1..5 or 0 for error
	uint8_t ocr,	///< value for OCR0A, 0..255
	Polarity polB	///< polarity of PWM output OC0B
	)
{
	if (cs==0) {	// T0 rate too low
		return 0;
	}

    PRR &= ~_BV(PRTIM0);

	m_polB = polB;
	m_comB = polB ? ((polB==ActiveHigh) ? 2 : 3 ) : 0;

	const uint32_t fclk = F_CPU;
	static const uint32_t T0_div[] = { 0,1,8,64,256,1024 };
	uint32_t arate = fclk / (T0_div[cs] * ocr);

	ocr--;
	uint8_t wgm = (m_comB==0) ? 2 : 7;
	
	TIMSK0 = 0;							// disable all T0 interrupts

	TCCR0A	= (0 << COM0A0)				// OC0A disconnected: TCCR0A.COM0A[1:0]=00
			| (m_comB << COM0B0)		// OC0B disconnected: TCCR0A.COM0B[1:0]=00
			| ((wgm & 3) << WGM00)		// CTC mode 2 (count from 0 to OCR2A), TCCR2A.WGM2[1:0]=10, TCCR2B.WGM22=0
			;
	TCCR0B	= (cs << CS00)		// clock source = CLKio/x
			| ((wgm>>2) << WGM02)
			;
	OCR0A  = m_ocr = ocr;

	TIFR0  = _BV(TOV0)|_BV(OCF0A)|_BV(OCF0B);	// clear interrupts

#if DEBUG_AVRTIMERS

	if (arate <= 1000) {
		m_MillisPerTick = 1000 / arate;
		m_TicksPerMilli = 1;
	} else {
		m_TicksPerMilli = arate / 1000;
		m_MillisPerTick = 0;
	}

	DEBUG_PRINTF(" T0: F=%lu, CS=%u, OCR=%u, rate is %lu, ",
		fclk, (unsigned)cs, (unsigned)ocr, arate );
	DEBUG_PRINTF("%d %s\r\n", 
		m_MillisPerTick ? m_MillisPerTick : m_TicksPerMilli,
		m_MillisPerTick ? "ms/t" : "t/ms" );

#endif // DEBUG_AVRTIMERS

	return arate;
}

//---------------------------------------------------------------------------

/** @brief program timer for current pulse width */
void AvrTimer0::setCR()
{
	TCCR0A	= 0 << COM0A0		 
			| (m_enableB ? m_comB : 0) << COM0B0
			| (T0WGM & 3) << WGM00
			;
}

/** @} */

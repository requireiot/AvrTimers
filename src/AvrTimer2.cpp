/**
 * @file 		  AvrTimer2.cpp
 * Project		: --
 * @author		: Bernd Waldmann
 * Created		: 03-Oct-2019
 * Tabsize		: 4
 *
 * This Revision: $Id: AvrTimer2.cpp 1087 2021-06-09 09:32:32Z  $
 *
 * @brief  Abstraction for AVR Timer/Counter 2, for periodic interrupts, also supports async mode.
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
#include "AvrTimers.h"

AvrTimer2* AvrTimer2::theInstance = NULL;

//---------------------------------------------------------------------------

ISR (TIMER2_COMPA_vect)
{
	sei();
	AvrTimer2::theInstance->call_tasks();
}

/** 
 * @addtogroup AvrTimers
 * @{ 
 */

//---------------------------------------------------------------------------

AvrTimer2::AvrTimer2(void) : AvrTimerBase()
{
	AvrTimer2::theInstance = this;
}

//---------------------------------------------------------------------------
#if 0
/** Initialize TC2 registers for periodic interrupt, but do not start. 
 * @return actual rate [Hz]
*/
uint32_t AvrTimer2::init(
	uint32_t rate,		///< desired interrupt rate in Hz
	uint32_t tickrate,	///< desired rate to call task functions, in Hz, or 0 for "same as rate"
	isr_t isr,			///< call this function for every interrupt
	uint32_t fclk,		///< T2 clock rate in Hz, default is CPU clock
	bool async			///< set T2 to async mode (external crystal on TOSC1/2), default is false
	)
{
	//memset((void*)m_tasks,0,sizeof(m_tasks));
	if (tickrate==0) tickrate=rate;
	
	// prescaler values for TC2, indexed by CS
	static uint16_t prescalers[] = { 0,1,8,32,64,128,256,1024 };
	#define lengthof(x) (sizeof((x))/sizeof((x)[0]))

	uint8_t cs = 0;
	uint32_t ocr;
	do {
		cs++;
		ocr = fclk / (rate * (long)prescalers[cs]);
	} while (cs < lengthof(prescalers) && ocr > 256uL );
	if (ocr > 256uL) {
		ocr = 255;
		DEBUG_PRINTF("Timer2 rate too low: %lu",rate);
	}
	
	return init(cs,ocr,rate / tickrate,isr,fclk,async);
}
#endif 

/**
 * @brief  Initialize TC2 registers for periodic interrupt, but do not start.
 * @return uint32_t  actual rate [Hz] or 0 if the rate can't be achieved
*/
uint32_t AvrTimer2::init(
	uint8_t cs,		///< clock select, see datasheet
	uint8_t ocr,	///< timer TOP value, goes to ICR1
	uint8_t pre,	///< one tick every `pre` interrupts
	isr_t isr,		///< call this from every interrupt
	uint32_t fclk,	///< T2 clock rate in Hz, default is CPU clock
	bool async		///< set T2 to async mode (external crystal on TOSC1/2), default is false
	)
{
	if (cs==0) {	// T2 rate too low
		return 0;
	}

    PRR &= ~_BV(PRTIM2);
    
	m_prescale = pre;
	m_async = async;
	m_isr = isr;
	
	TIMSK2 = 0;						// disable all T2 interrrupts
	if (async)
		ASSR |= _BV(AS2);

	TCCR2A	= (0 << COM2A0)		// OC2A disconnected: TCCR0A.COM0A[1:0]=00
			| (0 << COM2B0)		// OC2B disconnected: TCCR0A.COM0B[1:0]=00
			| (2 << WGM20)		// CTC mode 2 (count from 0 to OCR2A), TCCR2A.WGM2[1:0]=10, TCCR2B.WGM22=0
			;
	TCCR2B	= (cs << CS20)	// clock source = CLKio/1024: TCCR2B.CS2[2:0]=111
			| (0 << WGM22)
			;
	OCR2A  = m_ocr = ocr-1;
	if (async)
		while (ASSR & (_BV(TCN2UB)|_BV(OCR2AUB)|_BV(TCR2AUB))) {}

	TIFR2  = _BV(TOV2)|_BV(OCF2A)|_BV(OCF2B);	// clear interrupts

	uint32_t arate = fclk / (AvrTimer2::T2_div[cs] * ocr);
	uint32_t atick = arate / m_prescale;

#if DEBUG_AVRTIMERS

	if (atick <= 1000) {
		m_MillisPerTick = 1000 / atick;
		m_TicksPerMilli = 1;
	} else {
		m_TicksPerMilli = atick / 1000;
		m_MillisPerTick = 0;
	}

	DEBUG_PRINTF("\r\nT2: F=%ld, CS=%u, OCR=%u, rate %lu Hz, %lu t/ms, ",
		fclk, (unsigned)cs, (unsigned)ocr, arate, atick );
	DEBUG_PRINTF("%d %s\r\n",
		m_MillisPerTick ? m_MillisPerTick : m_TicksPerMilli,
		m_MillisPerTick ? "ms/t" : "t/ms" );

#endif // DEBUG_AVRTIMERS

	return arate;
}

/** @brief start TC2 interrupts. */
void AvrTimer2::start(void)
{
	TIFR2  = _BV(OCF2A);						// clear interrupt
	TIMSK2 = _BV(OCIE2A);						// enable overflow interrupt
}


/** @brief stop TC2 interrupts. */
void AvrTimer2::stop(void)
{
	TIMSK2 &= ~_BV(OCIE2A);						// enable overflow interrupt
}

//---------------------------------------------------------------------------

/// @brief Update `millis` etc counters, and call all registered callback functions 
void AvrTimer2::call_tasks(void)
{
	static uint8_t precount=1;
	
	if (m_async)
		OCR2A = m_ocr;

	if (m_isr) m_isr();
	if (--precount == 0) {
		precount = m_prescale;
		AvrTimerBase::call_tasks();
	}
	
	if (m_async) {
		while (ASSR & _BV(OCR2AUB)) {}
	}
}

/** @} */

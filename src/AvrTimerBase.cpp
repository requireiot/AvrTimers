/**
 * @file 		  AvrTimerBase.cpp
 * @author		  Bernd Waldmann
 * Created		: 03-Oct-2019
 * Tabsize		: 4
 *
 * This Revision: $Id: AvrTimerBase.cpp 1236 2021-08-16 09:24:37Z  $
 *
 * @brief  Abstraction for AVR Timer/Counters, for periodic interrupts
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

#include "AvrTimers.h"
#include "debugstream.h"

/** 
 * @addtogroup AvrTimers
 */

/** @{ */

//---------------------------------------------------------------------------

#ifdef ARDUINO
 extern volatile unsigned long timer0_millis;
#else
 volatile unsigned long timer0_millis = 0;

 unsigned long millis() {
	unsigned long t;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		t = timer0_millis;
	}
	return t;
 }
#endif

//---------------------------------------------------------------------------

AvrTimerBase::AvrTimerBase(void) : m_millis(0), m_handle_millis(false)
{
}

//---------------------------------------------------------------------------

/// @brief	Register a callback function.
void AvrTimerBase::add_task(
	uint16_t scale,	///< call every `scale` interrupt cycle 
	callback_t cb, ///< the callback function
	void* arg		///<  generic pointer argument to be passed to callback function (e.g. instance pointer)
	)
{
	if (m_nTasks < MAX_TIMER_TASKS) {
		m_tasks[m_nTasks].count = 0;
		m_tasks[m_nTasks].scale = scale;
		m_tasks[m_nTasks].callback = cb;
		m_tasks[m_nTasks].arg = arg;
		m_nTasks++;
	} else {
#if DEBUG_AVRTIMERS
		DEBUG_PRINT( "Too many timer tasks!\r\n" );
#endif
	}
}

//---------------------------------------------------------------------------

/// @brief	Update `millis` etc counters, and call all registered callback functions.
void AvrTimerBase::call_tasks(void)
{
	static uint8_t subms = 0;

	if (m_handle_millis) {
		timer0_millis += m_MillisPerTick;
	}
	if (m_MillisPerTick) {
		m_millis += m_MillisPerTick;
	} else {
		if (++subms >= m_TicksPerMilli) {
			subms = 0;
			m_millis++;
		}
	}
		
	uint8_t i; task_t* p;
	for (i=0, p=m_tasks; i<m_nTasks; i++, p++) {
		if (p->callback) {
			if (++(p->count) >= (p->scale)) {
				(p->callback)(p->arg);
				p->count = 0;
			}
		} else break;
	}
}

//---------------------------------------------------------------------------

/// @brief	Return milliseconds since start of timer.
uint32_t AvrTimerBase::get_millis()
{
	uint32_t temp;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		temp = m_millis;
	}
	return temp;
}

/** @} */

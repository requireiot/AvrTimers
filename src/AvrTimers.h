/**
 * @file 		  AvrTimers.h
 * @author		  Bernd Waldmann
 * Created		: 07-Mar-2020
 * Tabsize		: 4
 *
 * This Revision: $Id: AvrTimers.h 1236 2021-08-16 09:24:37Z  $
 */ 

/*
   Copyright (C) 2020,2021 Bernd Waldmann

   This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
   If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

   SPDX-License-Identifier: MPL-2.0
*/

#ifndef AVRTIMERS_H_
#define AVRTIMERS_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stddef.h>
#include <math.h>

#ifndef F_CPU	// keep syntax checker happy
 #define F_CPU 8000000
#endif

/**
 @defgroup AvrTimers  <AvrTimers.h>: Abstraction of ATmega Timer/Counters.
 
 @brief Abstraction for timers with multiple event handlers, and PWM channels.
 
 For each timer, the interrupt rate is specified in Hertz, and then the appropriate
 prescaler and divider values are calculated in constexpr functions, i.e. at compile time, 
 without a computational burden on the microcontroller side.

 All timers support
 - regular interrupts, with the rate specified in Hz
 - maintaining a milliceconds counter, similar to Arduino millis()
 - calling multiple event handler functions for every interrupt or every N interrupts

 Timer0 also supports
 - 8-bit PWM on 1 channel

 Timer1 also supports
 - 16-bit PWM on 2 channels
 
 Timer2 also supports
 - async mode with a 32.768 kHz watch crystal

 Any timer can be designated to simulate the Arduino `millis()`counter
 by calling `handle_millis()` once

 @note This code requires C++14, it will not compile with C++11.
 Therefore, include in platformio.ini the following settings
	build_unflags = -std=gnu++11
	build_flags = -std=gnu++14

 @{ 
 */

// if this is defined !=0, initialization routines print messages via debugprint library
#ifndef DEBUG_AVRTIMERS
 #define DEBUG_AVRTIMERS 0
#endif 

#ifndef ARDUINO
 unsigned long millis();
#endif

/**
 * @brief Base class for all Timers: call multiple event handlers
 * 
 */
class AvrTimerBase {
public:
    /// polarity of PWM output
	enum Polarity { ActiveHigh=1, Disabled=0, ActiveLow=-1 };
	/// callback function, called once per interrupt
	typedef void (*isr_t)(void);
	/// callback function, called once per tick
	typedef void (*callback_t)(void*);
	/// parameters that define a callback task.
	typedef struct _task_t {
		callback_t callback;
		uint16_t scale;
		uint16_t count;
		void*	arg;
	} task_t;
	static const int MAX_TIMER_TASKS = 4;
	// pointer to singleton instance, used by ISR
	//static AvrTimerBase* theInstance;

	AvrTimerBase(void);

	uint32_t get_millis();
	void add_task(uint16_t scale, callback_t cb, void* arg=NULL);
	void call_tasks(void);
	void handle_millis() { m_handle_millis=true; }
protected:
	volatile uint32_t m_millis;		
	task_t      m_tasks[MAX_TIMER_TASKS];
	uint8_t     m_nTasks;
	uint8_t     m_MillisPerTick;
	uint8_t     m_TicksPerMilli;
	bool        m_handle_millis;
};



/**
 * @brief Abstraction of Timer/Counter 0, supports 1 PWM channel (OC0B)
 * 
 */
class AvrTimer0 : public AvrTimerBase 
{
protected:
	uint8_t     m_ocr;
	bool        m_enableB;
	Polarity    m_polB;
	uint8_t     m_comB;

	/// prescaler per clock-select value (see datasheet)
	static constexpr uint32_t T0_div[] = { 1,1,8,64,256,1024 };

	static constexpr uint8_t calc_cs( uint32_t rate );
	static constexpr uint8_t calc_ocr( uint32_t rate );

	void setCR();
	uint32_t init(uint8_t cs, uint8_t ocr, Polarity polB );
public:
	/// pointer to singleton instance, used by ISR
	static AvrTimer0* theInstance;

	AvrTimer0(void);
	void start(void);
	void stop(void);
    void setPWM_B(uint8_t pwm, uint8_t top=UINT8_MAX);

	/**
	 * @brief Initialize Timer0, but don't start interrupts yet
	 * @param rate  desired interrupt rate [Hz]
	 * @param polB  polarity of OC0B 
	 */
	void begin(uint32_t rate, Polarity polB=Disabled )	
		{ init( calc_cs(rate), calc_ocr(rate), polB ); }
};


/**
 * @brief Abstraction of Timer/Counter 1, supports 2 channels hires-PWM
 * 
 */
class AvrTimer1 : public AvrTimerBase
{
protected:
	uint16_t    m_top;
	bool        m_enableA, m_enableB;
	Polarity    m_polA, m_polB;
	uint8_t     m_comA;
	uint8_t     m_comB;

	/// prescaler per clock-select value (see datasheet)
	static constexpr uint32_t T1_div[] = { 1,1,8,64,256,1024 };

	static constexpr uint8_t calc_cs( uint32_t rate );
	static constexpr uint16_t calc_ocr( uint32_t rate );

	void setCR();
	uint32_t init(uint8_t cs, uint16_t ocr, Polarity polA, Polarity polB );
public:
	static const uint16_t OCR_MAX = 10000;
	/// pointer to singleton instance, used by ISR
	static AvrTimer1* theInstance;

	AvrTimer1(void);
	void start(void);
	void stop(void);
    void setPWM_A(uint16_t pwm, uint16_t top=INT16_MAX);
    void setPWM_B(uint16_t pwm, uint16_t top=INT16_MAX);

	/**
	 * @brief Initialize Timer1, but don't start interrupts yet
	 * 
	 * @param rate 	desired interrupt rate [Hz]
	 * @param polA 	polarity of OC1A
	 * @param polB  polarity of OC1B
	 */
	void begin(uint32_t rate, Polarity polA=Disabled, Polarity polB=Disabled )
		{ init( calc_cs(rate), calc_ocr(rate), polA, polB ); }
};


/** @brief Abstraction of Timer/Counter 2 with async mode.
 * 
 * The interrupts also maintain a `millis` counter like Arduino 
 * ... but this one survives sleep power save, if the clock is async
 */
class AvrTimer2 : public AvrTimerBase 
{
protected:
	bool        m_async;
	uint8_t     m_ocr;
	uint8_t     m_prescale;
	isr_t 	    m_isr;
	
	/// prescaler per clock-select value (see datasheet)
	static constexpr uint32_t T2_div[] = { 1,1,8,32,64,128,256,1024 };

	static constexpr uint8_t calc_cs( uint32_t fclk, uint32_t rate );
	static constexpr uint8_t calc_ocr( uint32_t fclk, uint32_t rate );
	static constexpr uint8_t calc_pre( uint32_t rate, uint32_t tickrate );

	uint32_t init(uint8_t cs, uint8_t ocr, uint8_t prescaler, isr_t isr=NULL, uint32_t fclk=F_CPU, bool async=false);
public:
	/// pointer to singleton instance, used by ISR
	static AvrTimer2* theInstance;
	
	AvrTimer2(void);

	void start(void);
	void stop(void);
	void isr(void);
	
	/**
	 * @brief initialize Timer2, but don't start interrupts yet
	 * 
	 * @param rate 		desired interrupt rate [Hz]
	 * @param tickrate 	desired rate to call the callback functions [Hz]
	 * @param isr 		function to call for each interrupt, from ISR
	 * @param fclk 		clock frequency [Hz], default is CPU clock (F_CLK)
	 * @param async 	set timer to async mode with watch crystal 
	 */
	void begin(uint32_t rate, uint32_t tickrate=0, isr_t isr=NULL, uint32_t fclk=F_CPU, bool async=false)
		{ init( calc_cs(fclk,rate), calc_ocr(fclk,rate), calc_pre(rate,tickrate?tickrate:rate), isr, fclk, async ); }
};


/////////////////////////////////////////////////////////////////////////////
// private inline functions

/**
 * @brief Calculate clock select value CSn[2:0], at compile time if possible 
 * 
 * @param rate  desired interrupt rate [Hz]
 * @return constexpr uint8_t  value for CS0[2:0] in TCCR0B
 */
constexpr 
uint8_t AvrTimer0::calc_cs( uint32_t rate )
{
	const uint32_t constexpr fclk = F_CPU;

	if      (fclk / (rate * T0_div[1]) < 256uL) return 1;
	else if (fclk / (rate * T0_div[2]) < 256uL) return 2;
	else if (fclk / (rate * T0_div[3]) < 256uL) return 3;
	else if (fclk / (rate * T0_div[4]) < 256uL) return 4;
	else if (fclk / (rate * T0_div[5]) < 256uL) return 5;
	else {
		return 0;
	}
}


/**
 * @brief Calculate OCR value, at compile time if possible 
 * 
 * @param rate  desired interrupt rate [Hz]
 * @return constexpr uint8_t  divider ( 1 + value to write to OCR0A )
 */
constexpr 
uint8_t AvrTimer0::calc_ocr( uint32_t rate )
{
	const uint32_t constexpr fclk = F_CPU;
	
	const uint8_t cs = calc_cs( rate );
	return cs ? fclk / (rate * T0_div[cs]) : 0;
}

/////////////////////////////////////////////////////////////////////////////

constexpr 
uint8_t AvrTimer1::calc_cs( uint32_t rate )
{
	const uint32_t constexpr fclk = F_CPU;
	
	if (fclk / (rate * T1_div[1]) < 32767uL) return 1;
	else if (fclk / (rate * T1_div[2]) < 32767uL) return 2;
	else if (fclk / (rate * T1_div[3]) < 32767uL) return 3;
	else if (fclk / (rate * T1_div[4]) < 32767uL) return 4;
	else if (fclk / (rate * T1_div[5]) < 32767uL) return 5;
	else return 0;
}


constexpr 
uint16_t AvrTimer1::calc_ocr( uint32_t rate )
{
	const uint32_t constexpr fclk = F_CPU;
	
	const uint8_t cs = calc_cs( rate );
	return cs ? fclk / (rate * T1_div[cs]) : 0;
}

/////////////////////////////////////////////////////////////////////////////

constexpr 
uint8_t AvrTimer2::calc_cs( uint32_t fclk, uint32_t rate )
{
	if (fclk / (rate * T2_div[1]) < 256uL) return 1;
	else if (fclk / (rate * T2_div[2]) < 256uL) return 2;
	else if (fclk / (rate * T2_div[3]) < 256uL) return 3;
	else if (fclk / (rate * T2_div[4]) < 256uL) return 4;
	else if (fclk / (rate * T2_div[5]) < 256uL) return 5;
	else if (fclk / (rate * T2_div[6]) < 256uL) return 6;
	else return 0;
}


constexpr 
uint8_t AvrTimer2::calc_ocr( uint32_t fclk, uint32_t rate )
{
	const uint8_t cs = calc_cs( fclk, rate );
    /*
	return cs ? fclk / (rate * T2_div[cs]) : 0;
    */
    const double _fclk = fclk;
    const double _pre = T2_div[cs];
    const double _tclk = _fclk / _pre;
    const double _rate = rate;
    const double _ocr = _tclk / _rate + 0.5;
    return cs ? floor(_ocr) : 0;
}

/**
 * @brief Calculate soft prescaler, at compile time if possible 
 * 
 * @param rate 		desired interrupt rate [Hz]
 * @param tickrate  desired rate for calling event functions
 * @return constexpr uint8_t   # of interrupts per call to event functions
 */
constexpr 
uint8_t AvrTimer2::calc_pre( uint32_t rate, uint32_t tickrate )
{
	return rate / tickrate;
}

#endif // AvrTIMERS_H_
/** @} */

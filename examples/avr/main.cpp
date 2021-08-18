#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <avr/pgmspace.h>

#include "AvrUART.h"
#include "AvrTimers.h"


extern AvrUART0 uart0;


AvrTimer0 timer0;
AvrTimer1 timer1;
AvrTimer2 timer2;

volatile unsigned long my_millis = 0L;
volatile unsigned long my_seconds = 0L;


// callback function
void my_cb( void* )
{
    my_seconds++;
}


// interrupt service function, called every 10ms
void my_isr(void)
{
    my_millis += 10;
}


int main()
{
    // debug outputs
#ifndef NO_DEBUG
    uart0.begin(9600);
#endif

    // Timer 0: 1000 Hz interrupt, no PWM
    timer0.begin( 1000uL );   
    timer0.add_task( 1000uL, my_cb );
    timer0.start();

    // Timer1: 500 Hz rate, use PWN channel A
    timer1.begin( 500, AvrTimerBase::ActiveHigh, AvrTimerBase::Disabled );
    timer1.start();
    timer1.setPWM_A( 1024, 2048 );  // 50% duty cycle

    // Timer2: 100 Hz rate, async mode, driven by watch crystal
    timer2.begin( 100, 0, my_isr, 32768uL, true );
    timer2.start();
}

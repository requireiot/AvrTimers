AvrTimers

---
 
- [Overview](#overview)
- [Callbacks](#callbacks)
- [Timer0](#timer0)
- [Timer1](#timer1)
  - [Example](#example)
- [Timer2](#timer2)
- [Notes](#notes)
- [Dependencies](#dependencies)

--- 
## Overview
Abstraction for ATmega and ATtiny timers with multiple event handlers, and PWM channels.

This can be used in "classic" AVR projects as well as in Arduino projects. 
 
 For each timer, the appropriate prescaler and divider values are calculated by the code in *constexpr* functions, i. e. at compile time, without a computational burden on the microcontroller side.

 All timers support
 - regular interrupts, with the rate specified in Hz
 - maintaining a milliseconds counter, similar to Arduino millis()
 - calling multiple event handler functions for every interrupt or every N interrupts (a.k.a. "poor man's tasks scheduler")

 Timer0 also supports
 - 8-bit PWM on 1 channel

 Timer1 also supports
 - 16-bit PWM on 2 channels
 
 Timer2 also supports
 - async mode with a 32.768 kHz watch crystal

 Any timer can be designated to simulate the Arduino `millis()` counter by calling `handle_millis()` once.

This was developed for use with ATmega328 and similar controllers. If you work with a controller that has more timers, such as ATmega2560, then feel free to write additional classes bwAvrTimer3, bwAvrTimer4, ...

## Callbacks

Each timer can be configured to call one or more callback functions from its interrupt service routine. The callback function takes a `void *` argument (see below), and returns nothing:
```C++
typedef void (*callback_t)(void*);
```
You register your callback function with the timer class by calling `add_task()`
```C++
void add_task(uint16_t scale, callback_t cb, void* arg=NULL);
```
Your function `cb` will be called once every `scale` interrupts. The `arg` argument is passed on to your callback function, you can use it to pass a reference to a class instance, for example.

These callback functions are called from an interrupt service routine (ISR) context, so don't do anything in them that you wouldn't be comfortable doing directly in an ISR: no long calculations, no fiddling with interrupts, no waiting for something, etc.

Periodic interrupts are started with `start()`, and stopped with `stop()`.

## Timer0

In an Arduino project, you may want to stay away from Timer0, which is used by the Arduino libraries, and the `millis()` function depends on it.

In a non-Arduino project, you can emulate the Arduino milliseconds counter by calling ' handle_millis()' once. The timer ISR will then increment a counter, and you can call `millis()` to get the number of milliseconds elapsed since program start, just like in an Arduino project.

## Timer1

This timer is often used for PWM generation, because of its higher resolution (16bit). It can also be used for generating interrupts. This library uses Timer1 in "Mode 14" (see Atmel datasheet). Two PWM channels are available, A and B.

The timer is configured by calling `begin()`. You define the desired interrupt or PWM frequency in Hertz, and the desired polarity of PWM A and B.

You define the PWM duty cycle 
```C++
void setPWM_A(uint16_t pwm, uint16_t top=INT16_MAX);
void setPWM_B(uint16_t pwm, uint16_t top=INT16_MAX);
```
The PWM output is active for `pwm` time units out of every `top` time units. 

### Example

```C++
// declare class instance
AvrTimer1 timer1;
// initialize to 1000 Hz interrupt rate, OC1A active high, OC1B not used
timer1.begin( 1000, AvrTimerBase::ActiveHigh, AvrTimerBase::Disabled);
// set 25% duty cycle on OCR1A
timer1.setPWM_A( 250,1000 );
```

## Timer2

On some ATmega controllers, Timer2 can be used in "asynchronous mode", clocked by a 32.768 kHz watch crystal rather than by the CPU clock. In this mode, it keeps counting during certain sleep modes of the processor, which can be useful in battery-powered applications where the processor is in a low-power sleep state most of the time.


## Notes

This code uses features of the C++14 language standard, it will not compile with C++11.  Therefore, include in `platformio.ini` the following settings
```
build_unflags = -std=gnu++11
build_flags = -std=gnu++14
```

## Dependencies

The library uses the debug logging functions from `<debugstream.h>` (see separate repository). Each call to <code>AvrTimer<i>N</i>::begin()</code> will report the actual interrupt rate, if your application defines a `debug_printf()` function (see debugstream.h documentation for details). 

The library also relies on my `stdpins.h` device-dependent macro library for accessing the correct OCnA, OCnB pins for each AVR model.
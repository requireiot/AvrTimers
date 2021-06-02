Requirements for AvrTimers library

---
- [Use Cases](#use-cases)
- [Requirements](#requirements)
  - [Scope](#scope)
  - [Timekeeping](#timekeeping)
  - [Periodic execution of code](#periodic-execution-of-code)
  - [PWM](#pwm)
  - [API](#api)
  - [Resources and priorities](#resources-and-priorities)

---
## Use Cases
- use timers for scheduling periodic execution of code, e.g. to poll buttons or switches or for IR remote control
- use timers for PWM, e.g. to drive LEDs at variable brightness

## Requirements

### Scope
- [x] `R01` Control timers 0,1,2 as used in ATmega328, with possible expansion to other models such as ATmega1284
- [x] `R02` support async mode of Timer2, i.e. use of 32768 Hz watch crystal for time keeping
- [x] `R03` the library is usable in non-Arduino (AtmelStudio) projects
- [x] `R04` the library is usable in Arduino projects
- [x] `R05` the library is usable in C++ projects

### Timekeeping
- [x] `R10` in non-Arduino projects, the library can provide a `millis()` function that is substantially equivalent to the Arduino function, so the same application code or code snippets can be used in Arduino and non-Arduino projects

### Periodic execution of code
- [x] `R20` schedule periodic execution of callback functions defined by application 
- [x] `R21` call multiple callback functions from one timer ISR
- [x] `R22` schedule periodic execution of callback functions at different rates (example: fast IR polling at 16000 Hz and slow button debouncing at 100 Hz), from the same timer
- [x] `R23` Functions can be called between 16000/s and 1/s, for an 8 MHz or 16 MHz CPU clock
- [x] `R24` at least 4 callback functions can be called from each timer ISR
- [x] `R25` a callback function can be a globally visible function
- [x] `R26` a callback function can be a non-static member function of a class instance, i.e. the library must be able to pass an instance pointer to the callback function

### PWM
- [x] `R30` the selection of clock select and overflow count is made such that the duty cycle resolution is maximized
- [x] `R31` support 8-bit PWM on Timer0 OC0B
- [x] `R32` acceptable not to support PWM on Timer0 OC0A (because OCR0A is needed to define overflow count for timer, to set rate)
- [x] `R33` support 16-bit PWM on Timer1 OC1A and OC1B
- [x] `R34` acceptable not to support PWM on Timer2 OC2A (because OCR2A is needed to define overflow count for timer, to set rate)
- [x] `R35` acceptable not to support PWM on Timer2 OC2B (shared pin with INT1, often used for alternative function)

### API
- [x] `R40` Interrupt rate or function call rate can be specified in Hz, calculation of clock select and overflow count is done by the library
- [x] `R41` call rate of less frequently called functions can be specified as scale factor relative to base interrupt rate

### Resources and priorities
- [x] `R50` minimize memory footprint of library by doing calculations at compile time, where possible
- [x] `R51` accuracy of interrupt rate is more important than accuracy of PWM duty cycle 

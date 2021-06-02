#include <stdbool.h>

#include <Arduino.h>
#include <AvrTimers.h>
#define IMPLEMENT_ARDUINO_DEBUGSTREAM
#include "debugstream.h"

const int LED = A0; // PC0

// LED will be on when both these variables are true
volatile bool on_long;
volatile bool on_short;


void my_callback_slow( void* )
{
  on_long = !on_long;
}


void my_callback_fast( void* )
{
  on_short = !on_short;
  
  if (on_short && on_long) {
    digitalWrite(LED,HIGH);
  } else {
    digitalWrite(LED,LOW);
  }
}


AvrTimer1 timer;

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(57600uL);
  pinMode(LED,OUTPUT);

  timer.begin( 1000 );
  timer.add_task(  100, my_callback_fast );
  timer.add_task( 1000, my_callback_slow );
  timer.start();

  Serial.println(F("Ok.\r\n"));
}

void loop() {
  // put your main code here, to run repeatedly:
}
# Name		: Makefile
# Project	: test bwAvrTimer
# Author	: Bernd Waldmann
# Created	: 23.04.2021
# Tabsize	: 4
#
# This Revision: $Id: Makefile 1060 2021-06-02 08:58:18Z  $

## ----- General Flags

PROJECT = test_bwAvrTimer
MCU = atmega328p
F_CPU = 8000000
ROMSIZE = 30000
RAMSIZE = 2048

## ----- source files
CPPSOURCES = main.cpp AvrUART.cpp AvrTimerBase.cpp AvrTimer0.cpp AvrTimer1.cpp AvrTimer2.cpp
CSOURCES = 
ASOURCES = 

SHAREDPATH = ../../
include ../../mk.d/bw-avr-defines.mk
#COMMON_Cxx += -Wa,-adhln=$(*F).s -g -fverbose-asm
include ../../mk.d/fuses-ATmega328p-intRC.mk
include ../../mk.d/bw-avr-rules.mk

# Name		: Makefile
# Project	: test AvrTimer
# Author	: Bernd Waldmann
# Created	: 23.04.2021
# Tabsize	: 4
#
# This Revision: $Id: Makefile 1239 2021-08-18 08:09:26Z  $

## ----- General Flags

PROJECT = test_AvrTimer
MCU = atmega328p
F_CPU = 8000000
ROMSIZE = 30000
RAMSIZE = 2048

## ----- source files
CPPSOURCES = main.cpp AvrUART.cpp AvrTimerBase.cpp AvrTimer0.cpp AvrTimer1.cpp AvrTimer2.cpp
CSOURCES = 
ASOURCES = 

SHAREDPATH = ../../../
include $(SHAREDPATH)mk.d/bw-avr-defines.mk
# uncomment the next line if you want a detailed assembler listing
#COMMON_Cxx += -Wa,-adhln=$(*F).s -g -fverbose-asm
include $(SHAREDPATH)mk.d/fuses-ATmega328p-intRC.mk
include $(SHAREDPATH)mk.d/bw-avr-rules.mk

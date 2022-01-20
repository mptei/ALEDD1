/**
 * @file dbg_print.h
 * 
 * Definitions for debug printing.
 * Define 
 * #define DBG_PRINT             <= for switching on/off
 * #define DBG_SERIAL SerialUSB  <= for serial to be used
 * #define DBG_SUSPEND           <= to wait until serial is connected
 * 
 */
#pragma once

#include <Arduino.h>
#include <stdarg.h>

void dbg_init()
{
#ifdef DBG_PRINT
    DBG_SERIAL.begin(115200);
#ifdef DBG_SUSPEND
    while (!DBG_SERIAL);
#endif
#endif
}
void dbg_print(const __FlashStringHelper *format, ...)
{
#ifdef DBG_PRINT
    if (true) {

        char buf[128]; // limit to 128chars
        va_list args;
        va_start(args, format);

#if defined(__AVR__) || defined(ESP8266) || defined(ARDUINO_ARCH_STM32)
        vsnprintf_P(buf, sizeof (buf), (const char *) format, args); // progmem for AVR and ESP8266
#else
        vsnprintf(buf, sizeof (buf), (const char *) format, args); // for rest of the world
#endif    

        va_end(args);
        DBG_SERIAL.println(buf);
    }
#endif
}

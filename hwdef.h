/**
 * @file hwdef.h
 * 
 * Definitions for used hardware
 * 
 */
#pragma once

#include <Arduino.h>

#include "wiring_private.h"

#if defined(HW_ARDUINO_ZERO)
#define LED_STRIP_PIN 22  //LED shield
#define POWER_SUPPLY_PIN 8 //active low
#define POWER_SUPPLY_PIN_ACTIVE true |  // switched off
#define PROG_LED_PIN LED_BUILTIN
#define PROG_LED_PIN_ACTIVE HIGH
#define PROG_BUTTON_PIN 2 
#define PROG_BUTTON_INT FALLING 
#elif defined(HW_ALEDD1)
#define LED_STRIP_PIN 22  //LED shield
#define POWER_SUPPLY_PIN 38 //active low
#define POWER_SUPPLY_PIN_ACTIVE true ^  // active low
#define PROG_LED_PIN 8 
#define PROG_LED_PIN_ACTIVE HIGH
#define PROG_BUTTON_PIN 2 
#define PROG_BUTTON_INT FALLING 
#else
#error "Unsupported hardware"
#endif


// Setup KNX serial on unusual pins (for ALEDD1 hardware)
class UartKNX : public Uart
{
public:
    UartKNX() : Uart(&sercom2, 3, 1, SERCOM_RX_PAD_1, UART_TX_PAD_2)
    {
    }
    void begin(unsigned long baudrate, uint16_t config)
    {
        Uart::begin(baudrate, config);
        pinPeripheral(3, PIO_SERCOM_ALT);
        pinPeripheral(1, PIO_SERCOM_ALT);
    }
};
UartKNX SerialKNX;
//Interrupt handler for SerialKNX
void SERCOM2_Handler()
{
    SerialKNX.IrqHandler();
}

void SERCOM0_Handler() __attribute__((weak));
void SERCOM0_Handler()
{
  Serial1.IrqHandler();
}

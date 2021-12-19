#include <Arduino.h>
#include <knx.h>

#include "wiring_private.h"

#if MASK_VERSION != 0x07B0 && (defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32)
#include <WiFiManager.h>
#endif

#include <DimmerControl.h>
#include <FlashAsEEPROM.h>
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <NeoPixelPainter.h>

//#include "kdevice_ALEDD1.h"
#define LED_STRIP_PIN 22  //LED shield
#define POWER_SUPPLY_PIN 8 //active low

#include "defines.h"

// create named references for easy access to group objects
#define goDimmer knx.getGroupObject(1)
#define goDimmerRel knx.getGroupObject(2)
#define goDimmerAbs knx.getGroupObject(3)
#define goDimmerStatus knx.getGroupObject(4)
#define goDimmerValueStatus knx.getGroupObject(5)
#define goScene knx.getGroupObject(6)
#define goRedVal knx.getGroupObject(7)
#define goSceneStatus knx.getGroupObject(17)

#define goPowerSupply knx.getGroupObject(34)

//global variables
bool initialized = false;
byte currentTask = TASK_IDLE; //0xFE - idle
byte lastTask = TASK_IDLE;
byte lastTaskBeforeMessage = 0; // all LEDs are off
byte lastStaticColor[4] = {0, 0, 0, 0};
bool staticColorReady = false;
byte sendSceneNumber = 0xFF;
byte lastDimmerValue = 0;
unsigned long lastAnimMillis = 0;

byte curveR[256];
byte curveG[256];
byte curveB[256];
byte curveW[256];

word longClickDurationBtn = 500;
unsigned long clickMillis = 0;
bool buttonPressed = false;
bool ledTestMode = false;
bool pixelsShow = false;

//XML group: LED
uint8_t ledType = 0xC6; // ~ NEO_RGBW, see Adafruit_NeoPixel.h for more infos
bool rgbw = true;
byte mixedWhite[] = {255,240,224};
uint16_t numberLeds = 600; //amount of leds on a stripe
uint8_t firstOnValue = 1;
uint8_t maxR = 255; // to match the same brightness on different colors
uint8_t maxG = 255; // reduce brightnes of some colors
uint8_t maxB = 255; // also usefull to make not "to blueisch" white
uint8_t maxW = 255; // recomended values: R:255,G:176,B:240,W:255
//uint8_t whiteType = 0; // if RGBW strip used, 0=warm, 1=neutral, 2=cold
float gammaCorrection = 1.0;
//XML group: Dimmer
byte softOnOffTimeList[] = {0, 3, 5, 7, 10, 15, 20}; //hundreds of milliseconds: 0,300,500...
byte relDimTimeList[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 20}; //seconds
byte valueMinDay;
byte valueMaxDay;
byte valueMinNight;
byte valueMaxNight;
//XML group: Scenes
byte scene[64];
//XML group: User color 1-5
byte ucRed[USERCOLORS];
byte ucGreen[USERCOLORS];
byte ucBlue[USERCOLORS];
byte ucWhite[USERCOLORS];
byte new3Byte[3] = {0,0,0};
byte new6Byte[6] = {0,0,0,0,0,0};
byte newRGBW[4] = {0,0,0,0};
byte valuesRGBW[4] = {0,0,0,0};
byte newHSV[3] = {0,0,0};
byte valuesHSV[3] = {0,0,0};
bool rgbwChanged = false;
bool hsvChanged = false;
bool acceptNewRGBW = false;
bool acceptNewHSV = false;
unsigned long rgbwChangedMillis = 0;
unsigned long hsvChangedMillis = 0;
word rgbwhsvChangedDelay = 50;

//XML group messages:
//Message 1
byte statusM = 0;   // a bit for every message, false = wait and do nothing, true = show message1Value
struct msg {
    byte newValue; //0 = all LEDs off, 1-255 corresponds to percentage of leds (255 = all LEDs are on, 127 = only 50% of LEDs are on)
    byte lastValue;
    word ledFirst;
    word ledLast;
    byte ledColor[4];
} msg[MESSAGES];

//XML group: Power supply control
bool allLedsOff = true;
bool powerSupplyState = false;
bool powerSupplyTurnOn = false;
bool powerSupplyTurnOff = false;
bool powerSupplyControl = false;
unsigned long powerSupplyOffDelay = 30000; 
unsigned long powerSupplyOffMillis = 0;
bool psStateChecked = false;
unsigned long psStateCheckedMillis = 0;
bool lastState = false;

//create some instances
Adafruit_NeoPixel_ZeroDMA *neopixels;
NeoPixelPainterCanvas *pixelcanvas;
NeoPixelPainterCanvas *pixelcanvas2;
NeoPixelPainterBrush *pixelbrush;
NeoPixelPainterBrush *pixelbrush2;
NeoPixelPainterBrush *pixelbrush3;
NeoPixelPainterBrush *pixelbrush4on2; //brush 4 on cancas 2
HSV brushcolor;
DimmerControl dimmer;

//make functions known
void showPixels ();

#include "hsvrgb.h"
#include "animations.h"
#include "led_functions.h"
#include "scenes.h"
#include "knx_events.h"

float currentValue = 0;
float maxValue = 0;
float minValue = RAND_MAX;
long lastsend = 0;

// Setup KNX serial on unusual pins
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

void setup()
{
#ifndef KDEBUG
    SerialUSB.begin(115200);
    while (!SerialUSB);
    ArduinoPlatform::SerialDebug = &SerialUSB;
#else
    Serial.begin(115200);
    ArduinoPlatform::SerialDebug = &Serial;
#endif

    randomSeed(millis());

#if MASK_VERSION != 0x07B0 && (defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32)
    WiFiManager wifiManager;
    wifiManager.autoConnect("knx-demo");
#endif

    // read adress table, association table, groupobject table and parameters from eeprom
    knx.readMemory();

    // print values of parameters if device is already configured
    if (knx.configured())
    {
        goDimmer.dataPointType(DPT_Switch);
        goDimmer.callback(dimmSwitchCallback);

        goDimmerRel.dataPointType(DPT_Control_Dimming);
        goDimmerRel.callback(dimmRelCallback);

        goDimmerAbs.dataPointType(DPT_Percent_U8);
        goDimmerAbs.callback(dimmAbsCallback);

        goDimmerStatus.dataPointType(DPT_Switch);
        goDimmerValueStatus.dataPointType(DPT_Percent_U8);

        goScene.dataPointType(DPT_SceneNumber);
        goScene.callback(sceneCallback);

        goSceneStatus.dataPointType(DPT_SceneNumber);
        goPowerSupply.dataPointType(DPT_Switch);

#define PARM_ledType            0
#define PARM_numbersLedsStrip   1
#define PARM_firstOnValue       5
#define PARAM_rCorrection       9      
#define PARAM_gCorrection       13      
#define PARAM_bCorrection       17
#define PARAM_wCorrection       21
#define PARAM_gammaCorrection   25
#define PARAM_wr                29
#define PARAM_wg                33
#define PARAM_wb                37

#define PARAM_timeSoft          41
#define PARAM_timeRel           42
#define PARAM_dayMin            43
#define PARAM_dayMax            47
#define PARAM_nightMin          51
#define PARAM_nightMax          55

#define PARAM_uc1r              59
#define PARAM_uc1g              63
#define PARAM_uc1b              67
#define PARAM_uc1w              71

#define PARAM_m1first          139
#define PARAM_m1last           143
#define PARAM_m1r              147
#define PARAM_m1g              151
#define PARAM_m1b              155
#define PARAM_m1w              159

#define PARAM_psControl        235
#define PARAM_psDelay          236

        //XML group: LED
        ledType = knx.paramByte(PARM_ledType);
        if(ledType == NEO_RGB || ledType == NEO_RBG || ledType == NEO_GRB || 
           ledType == NEO_GBR || ledType == NEO_BRG || ledType == NEO_BGR) rgbw = false;
        numberLeds = knx.paramInt(PARM_numbersLedsStrip);
        firstOnValue = knx.paramInt(5);
        maxR = knx.paramInt(PARAM_rCorrection);
        maxG = knx.paramInt(PARAM_gCorrection);
        maxB = knx.paramInt(PARAM_bCorrection);
        maxW = knx.paramInt(PARAM_wCorrection);
        gammaCorrection = knx.paramInt(PARAM_gammaCorrection) * 0.1;
        mixedWhite[0] = knx.paramInt(PARAM_wr);
        mixedWhite[1] = knx.paramInt(PARAM_wg);
        mixedWhite[2] = knx.paramInt(PARAM_wb);
        //XML group: Dimmer
        dimmer.setDurationAbsolute(softOnOffTimeList[knx.paramByte(PARAM_timeSoft)] * 100);
        dimmer.setDurationRelative(relDimTimeList[knx.paramByte(PARAM_timeRel)] * 1000);
        dimmer.setValueFunction(&setLeds);
        valueMinDay = knx.paramInt(PARAM_dayMin);
        valueMaxDay = knx.paramInt(PARAM_dayMax);
        valueMinNight = knx.paramInt(PARAM_nightMin);
        valueMaxNight = knx.paramInt(PARAM_nightMax);
        //set day values until we know if it is day or night
        setDayNightValues(false);
        //XML group: User colors
#define UCSIZE (4 * 4)
        for (byte uc = 0; uc < USERCOLORS; uc++) {
            ucRed[uc]   = knx.paramInt(PARAM_uc1r + UCSIZE * uc);
            ucGreen[uc] = knx.paramInt(PARAM_uc1g + UCSIZE * uc);
            ucBlue[uc]  = knx.paramInt(PARAM_uc1b + UCSIZE * uc);
            ucWhite[uc] = knx.paramInt(PARAM_uc1w + UCSIZE * uc);
        }
        //XML group: Messages:
#define MSGSIZE (6 * 4)
        for (byte mc = 0; mc < MESSAGES; mc++) {
            msg[mc].ledFirst    = knx.paramInt(PARAM_m1first + MSGSIZE * mc) - 1; //Code: count from 0.., Suite: Count from 1..
            msg[mc].ledLast     = knx.paramInt(PARAM_m1last + MSGSIZE * mc) - 1;
            msg[mc].ledColor[R] = knx.paramInt(PARAM_m1r + MSGSIZE * mc);
            msg[mc].ledColor[G] = knx.paramInt(PARAM_m1g + MSGSIZE * mc);
            msg[mc].ledColor[B] = knx.paramInt(PARAM_m1b + MSGSIZE * mc);
            msg[mc].ledColor[W] = knx.paramInt(PARAM_m1w + MSGSIZE * mc);
        }
        //XML group: power supply
        powerSupplyControl = knx.paramByte(PARAM_psControl);
        powerSupplyOffDelay = knx.paramInt(PARAM_psDelay) * 60000;
        if(!powerSupplyControl){
            powerSupplyState = true;
        }
        println(F("LED_Type: 0x%02x"), ledType);
        println(F("LED_Count: %d"), numberLeds);
        println(F("Gamma: %f"), gammaCorrection);

        setDimmingCurves();
        initStrip(numberLeds, ledType);
    }

    // pin or GPIO the programming led is connected to. Default is LED_BUILTIN
    knx.ledPin(LED_BUILTIN);
    // is the led active on HIGH or low? Default is LOW
    knx.ledPinActiveOn(HIGH);
    // pin or GPIO programming button is connected to. Default is 0
    // knx.buttonPin(0);
    knx.platform().knxUart(&SerialKNX);

    // start the framework.
    knx.start();

    if (!knx.configured()) {
        testStrip();

        // Go into programming mode by default
        knx.progMode(true);
    }
}

void powerSupply(){
    if(powerSupplyControl){
        if(!lastStaticColor[R] && !lastStaticColor[G] && !lastStaticColor[B] && !lastStaticColor[W] &&
           !msg[0].lastValue && !msg[1].lastValue && !msg[2].lastValue && !msg[3].lastValue && currentTask == TASK_IDLE){
            allLedsOff = true;
        }else{
            allLedsOff = false;
        }
        if(powerSupplyTurnOn && !powerSupplyState){
            goPowerSupply.value(true);
            println(F("Turn PS on!"));
            powerSupplyTurnOn = false;
        }
        if(powerSupplyState && allLedsOff && !powerSupplyTurnOff){//power supply is on and all LEDs are off => start to 'turn off power supply' routine
            powerSupplyTurnOff = true;
            powerSupplyOffMillis = millis();
            println(F("All LEDs are off, start 'turn off power supply' routine"));
        }
        if(powerSupplyState && !allLedsOff && powerSupplyTurnOff){//power supply is on and some LEDs are on => PS stays on
            powerSupplyTurnOff = false;
            println(F("Some LEDs are on, stop 'turn off power supply' routine"));
        }
        if(!powerSupplyState && allLedsOff && powerSupplyTurnOff){//stop "power off" routine, PS is already off
            println(F("All LEDs are off, Power Supply is also off, stop 'turn off power supply' routine"));
            powerSupplyTurnOff = false;
        }
        if(powerSupplyTurnOff && (millis() - powerSupplyOffMillis) >= powerSupplyOffDelay){
            println(F("Time is over, turn PS off"));
            goPowerSupply.value(false);
            powerSupplyTurnOff = false;
        }
    }
}


void loop()
{
    // don't delay here to much. Otherwise you might lose packages or mess up the timing with ETS
    knx.loop();

    // only run the application code if the device was configured with ETS
    if (!knx.configured())
        return;

    if (psStateChecked && millis() - psStateCheckedMillis >= 100)
    { //debounce
        psStateChecked = false;
    }
    if (!psStateChecked)
    {
        powerSupplyState = 1; //!digitalRead(POWER_SUPPLY_PIN); //low = ON, high = OFF
        psStateCheckedMillis = millis();
        psStateChecked = true;
        if (lastState != powerSupplyState)
        {
            lastState = powerSupplyState;
            println(F("PowerSupply state: %d"), powerSupplyState);
        }
    }

    if (powerSupplyState)
    { //wait until PS is on...
        dimmer.task();
        taskFunction();
    }
    powerSupply();
    if (rgbwChanged)
    {
        if (millis() - rgbwChangedMillis > rgbwhsvChangedDelay && !acceptNewRGBW)
        {
            println(F("apply new rgb(w) values"));
            println(F("newRGBW R: %d, G: %d, B: %d, W: %d"), newRGBW[R], newRGBW[G], newRGBW[B], newRGBW[W]);
            println(F("valuesRGBW R: %d, G: %d, B: %d, W: %d \n"), valuesRGBW[R], valuesRGBW[G], valuesRGBW[B], valuesRGBW[W]);

            valuesRGBW[R] = newRGBW[R];
            valuesRGBW[G] = newRGBW[G];
            valuesRGBW[B] = newRGBW[B];
            valuesRGBW[W] = newRGBW[W];
            acceptNewRGBW = true;
        }
    }
    if (hsvChanged)
    {
        if (millis() - hsvChangedMillis > rgbwhsvChangedDelay && !acceptNewHSV)
        {
            println(F("apply new hsv values"));

            valuesHSV[0] = newHSV[0];
            valuesHSV[1] = newHSV[1];
            valuesHSV[2] = newHSV[2];
            acceptNewHSV = true;
        }
    }
    if (dimmer.updateAvailable())
    {
        goDimmerStatus.value(dimmer.getCurrentValue());
        println(F("Send dimmer status: %d"), dimmer.getCurrentValue() != 0);
        if (!dimmer.getCurrentValue()) {
            sendSceneNumber = 0; //all off
        }
        goDimmerValueStatus.value(dimmer.getCurrentValue());
        println(F("Send dimmer value status: %d"), dimmer.getCurrentValue());
        lastDimmerValue = dimmer.getCurrentValue();
        dimmer.resetUpdateFlag();
    }
    if (sendSceneNumber < 64)
    {
        println(F("Send scene status: %d"), sendSceneNumber);
        goSceneStatus.value(sendSceneNumber);
        sendSceneNumber = 0xFF;
    }
}

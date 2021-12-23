#include <Arduino.h>

#include <stdarg.h>
// Activate debugging output
#define DBG_PRINT
#define DBG_SERIAL SerialUSB
void dbg_init()
{
#ifdef DBG_PRINT
    DBG_SERIAL.begin(115200);
    while (!DBG_SERIAL);
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
#define goGreenVal knx.getGroupObject(8)
#define goBlueVal knx.getGroupObject(9)
#define goWhiteVal knx.getGroupObject(10)

#define goRGBW knx.getGroupObject(12)

#define goSceneStatus knx.getGroupObject(17)

#define MSGGOSIZE 4
#define goMsgSwitch(NUM) knx.getGroupObject(18+NUM*MSGGOSIZE)
#define goMsgPercent(NUM) knx.getGroupObject(19+NUM*MSGGOSIZE)
#define goMsgRGB(NUM) knx.getGroupObject(20+NUM*MSGGOSIZE)
#define goMsgRGBW(NUM) knx.getGroupObject(21+NUM*MSGGOSIZE)

#define goPowerSupply knx.getGroupObject(34)

#define goDayNight knx.getGroupObject(35)

typedef union {
    uint32_t rgbw;
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t w;
    } c;
} color_t;
#define COLOR(R,G,B,W) {.c={(B),(G),(R),(W)}}
#define COLORMASK 0x00FFFFFFUL

const color_t black = COLOR(0,0,0,0);
const color_t white = COLOR(0,0,0,255);
const color_t red = COLOR(255,0,0,0);
const color_t yellow = COLOR(255,0,0,0);
const color_t green = COLOR(0,255,0,0);
const color_t cyan = COLOR(0,255,255,0);
const color_t blue = COLOR(0,0,255,0);
const color_t purple = COLOR(255,0,255,0);
const color_t orange = COLOR(255,81,0,0);

//global variables
bool initialized = false;
byte currentTask = TASK_IDLE; //0xFE - idle
byte lastTask = TASK_IDLE;
byte lastTaskBeforeMessage = 0; // all LEDs are off
color_t lastStaticColor = black;
bool staticColorReady = false;
byte sendSceneNumber = 0xFF;
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
color_t mixedWhite = COLOR(255,240,224,0);
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
// The currently used values
byte valueMin;
byte valueMax;
//XML group: User color 1-5
color_t userColors[USERCOLORS];
color_t newRGBW;
color_t valuesRGBW;
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
    color_t ledColor;
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

bool onMeansDay = false;
bool sendOnStartup = false;

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
    dbg_init();
#ifdef DBG_PRINT
    ArduinoPlatform::SerialDebug = &DBG_SERIAL;
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

        goRedVal.dataPointType(DPT_Percent_U8);
        goRedVal.callback(colorChannelCallback);
        goGreenVal.dataPointType(DPT_Percent_U8);
        goGreenVal.callback(colorChannelCallback);
        goBlueVal.dataPointType(DPT_Percent_U8);
        goBlueVal.callback(colorChannelCallback);
        goWhiteVal.dataPointType(DPT_Percent_U8);
        goWhiteVal.callback(colorChannelCallback);

        goRGBW.dataPointType(DPT_Colour_RGBW);
        goRGBW.callback(rgbwCallback);

        goSceneStatus.dataPointType(DPT_SceneNumber);
        goPowerSupply.dataPointType(DPT_Switch);

        for (byte mc = 0; mc < MESSAGES; mc++) {
            goMsgSwitch(mc).dataPointType(DPT_Switch);
            goMsgSwitch(mc).callback(msgCallback);
            goMsgPercent(mc).dataPointType(DPT_Percent_U8);
            goMsgPercent(mc).callback(msgCallback);
            goMsgRGB(mc).dataPointType(DPT_Colour_RGB);
            goMsgRGB(mc).callback(msgCallback);
            goMsgRGBW(mc).dataPointType(DPT_Colour_RGBW);
            goMsgRGBW(mc).callback(msgCallback);
        }

        goDayNight.dataPointType(DPT_Switch);
        goDayNight.callback(dayNightCallback);

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

#define PARAM_dayIsOn          240
#define PARAM_statusOnStart    241

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
        mixedWhite.c.r = knx.paramInt(PARAM_wr);
        mixedWhite.c.g = knx.paramInt(PARAM_wg);
        mixedWhite.c.b = knx.paramInt(PARAM_wb);
        //XML group: Dimmer
        dimmer.setDurationAbsolute(softOnOffTimeList[knx.paramByte(PARAM_timeSoft)] * 100);
        dimmer.setDurationRelative(relDimTimeList[knx.paramByte(PARAM_timeRel)] * 1000);
        //dimmer.setValueFunction(&setLeds);
        dimmer.setValueFunction(&setBrightness);
        valueMinDay = knx.paramInt(PARAM_dayMin);
        valueMaxDay = knx.paramInt(PARAM_dayMax);
        valueMinNight = knx.paramInt(PARAM_nightMin);
        valueMaxNight = knx.paramInt(PARAM_nightMax);
        onMeansDay = knx.paramByte(PARAM_dayIsOn);
        //set day values until we know if it is day or night
        setDayNightValues(false);
        //XML group: User colors
#define UCSIZE (4 * 4)
        for (byte uc = 0; uc < USERCOLORS; uc++) {
            userColors[uc].rgbw = 
                ((uint8_t)knx.paramInt(PARAM_uc1r + uc * UCSIZE)) << 16
                |((uint8_t)knx.paramInt(PARAM_uc1g + uc * UCSIZE)) << 8
                |((uint8_t)knx.paramInt(PARAM_uc1b + uc * UCSIZE))
                |((uint8_t)knx.paramInt(PARAM_uc1w + uc * UCSIZE) << 24)
                ;
        }
        //XML group: Messages:
#define MSGSIZE (6 * 4)
        for (byte mc = 0; mc < MESSAGES; mc++) {
            msg[mc].ledFirst    = knx.paramInt(PARAM_m1first + MSGSIZE * mc) - 1; //Code: count from 0.., Suite: Count from 1..
            msg[mc].ledLast     = knx.paramInt(PARAM_m1last + MSGSIZE * mc) - 1;
            msg[mc].ledColor.c.r = knx.paramInt(PARAM_m1r + MSGSIZE * mc);
            msg[mc].ledColor.c.g = knx.paramInt(PARAM_m1g + MSGSIZE * mc);
            msg[mc].ledColor.c.b = knx.paramInt(PARAM_m1b + MSGSIZE * mc);
            msg[mc].ledColor.c.w = knx.paramInt(PARAM_m1w + MSGSIZE * mc);
        }
        //XML group: power supply
        powerSupplyControl = knx.paramByte(PARAM_psControl);
        powerSupplyOffDelay = knx.paramInt(PARAM_psDelay) * 60000;
        if(!powerSupplyControl){
            powerSupplyState = true;
        }

        sendOnStartup = knx.paramByte(PARAM_statusOnStart);
        

        dbg_print(F("LED_Type: 0x%02x"), ledType);
        dbg_print(F("LED_Count: %d"), numberLeds);
        dbg_print(F("Gamma: %f"), gammaCorrection);

        setDimmingCurves();
        initStrip(numberLeds, ledType);
        // White by default
        setAll(white);
        // Off by default
        neopixels->setBrightness(0);
        pixelsShow = true;
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
        if(!lastStaticColor.rgbw &&
           !msg[0].lastValue && !msg[1].lastValue && !msg[2].lastValue && !msg[3].lastValue && currentTask == TASK_IDLE){
            allLedsOff = true;
        }else{
            allLedsOff = false;
        }
        if(powerSupplyTurnOn && !powerSupplyState){
            goPowerSupply.value(true);
            dbg_print(F("Turn PS on!"));
            powerSupplyTurnOn = false;
        }
        if(powerSupplyState && allLedsOff && !powerSupplyTurnOff){//power supply is on and all LEDs are off => start to 'turn off power supply' routine
            powerSupplyTurnOff = true;
            powerSupplyOffMillis = millis();
            dbg_print(F("All LEDs are off, start 'turn off power supply' routine"));
        }
        if(powerSupplyState && !allLedsOff && powerSupplyTurnOff){//power supply is on and some LEDs are on => PS stays on
            powerSupplyTurnOff = false;
            dbg_print(F("Some LEDs are on, stop 'turn off power supply' routine"));
        }
        if(!powerSupplyState && allLedsOff && powerSupplyTurnOff){//stop "power off" routine, PS is already off
            dbg_print(F("All LEDs are off, Power Supply is also off, stop 'turn off power supply' routine"));
            powerSupplyTurnOff = false;
        }
        if(powerSupplyTurnOff && (millis() - powerSupplyOffMillis) >= powerSupplyOffDelay){
            dbg_print(F("Time is over, turn PS off"));
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
            dbg_print(F("PowerSupply state: %d"), powerSupplyState);
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
            dbg_print(F("apply new rgb(w) values"));
            dbg_print(F("newRGBW %ld"), newRGBW.rgbw);
            dbg_print(F("valuesRGBW %ld \n"), valuesRGBW.rgbw);

            valuesRGBW.rgbw = newRGBW.rgbw;
            acceptNewRGBW = true;
        }
    }
    if (hsvChanged)
    {
        if (millis() - hsvChangedMillis > rgbwhsvChangedDelay && !acceptNewHSV)
        {
            dbg_print(F("apply new hsv values"));

            valuesHSV[0] = newHSV[0];
            valuesHSV[1] = newHSV[1];
            valuesHSV[2] = newHSV[2];
            acceptNewHSV = true;
        }
    }
    if (dimmer.updateAvailable())
    {
        byte dimmValue = dimmer.getCurrentValue();
        goDimmerStatus.value(dimmValue);
        dbg_print(F("Send dimmer status: %d"), dimmValue);
        if (!dimmValue) {
            // Switch LEDs off; a message might keep them on
            currentTask = ALL_OFF;
            sendSceneNumber = ALL_OFF; //all off
        }
        goDimmerValueStatus.value(dimmValue);
        dbg_print(F("Send dimmer value status: %d"), dimmValue);
        dimmer.resetUpdateFlag();
    }
    if (sendSceneNumber < 64)
    {
        dbg_print(F("Send scene status: %d"), sendSceneNumber);
        goSceneStatus.value(sendSceneNumber);
        sendSceneNumber = 0xFF;
    }
}

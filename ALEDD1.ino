#include <Arduino.h>


// Activate debugging output
#define DBG_PRINT
#define DBG_SERIAL SerialUSB
#include "dbg_print.h"

#include <knx.h>

#if MASK_VERSION != 0x07B0 && (defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32)
#include <WiFiManager.h>
#endif

#include <DimmerControl.h>
#include <FlashAsEEPROM.h>
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <NeoPixelPainter.h>

#include "hwdef.h"
#include "defines.h"

#include "generated/knx_defines.h"


// Number of group objects per message
#define MSGGOCNT 4

// A type to access the fields of RGBW in a different way
typedef union {
    uint32_t rgbw;
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t w;
    } c;
    uint8_t a[4];
} color_t;
bool operator!=(const color_t &a, const color_t &b)
{
    return a.rgbw != b.rgbw;
}
bool operator==(const color_t &a, const color_t &b)
{
    return !(a != b);
}
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
uint8_t ledType;
bool rgbw;
bool whiteOnly;
color_t mixedWhite;
uint16_t numberLeds; //amount of leds on a stripe
uint8_t firstOnValue;
uint8_t maxR; // to match the same brightness on different colors
uint8_t maxG; // reduce brightnes of some colors
uint8_t maxB; // also usefull to make not "to blueisch" white
uint8_t maxW; // recomended values: R:255,G:176,B:240,W:255
//uint8_t whiteType = 0; // if RGBW strip used, 0=warm, 1=neutral, 2=cold
float gammaCorrection;

//XML group: Dimmer
const byte softOnOffTimeList[] = {0, 3, 5, 7, 10, 15, 20}; //hundreds of milliseconds: 0,300,500...
const byte relDimTimeList[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 20}; //seconds
byte valueMinDay;
byte valueMaxDay;
byte valueMinNight;
byte valueMaxNight;
// The currently used values
byte valueMin;
byte valueMax;
byte dimmScene;
bool dimmLog;
byte lastDimmValue;

//XML group: User color 1-5
color_t userColors[USERCOLORS];
color_t newRGBW;
color_t valuesRGBW;

#define STATE_CHANGE_DELAY 100
unsigned long rgbwStateMillis;
color_t valuesRGBWState;

byte newHSV[3] = {0,0,0};
byte valuesHSV[3] = {0,0,0};
bool rgbwSingleChannelChanged;
bool hsvSingleValueChanged;
bool acceptNewRGBW;
bool acceptNewHSV;
unsigned long rgbwChangedMillis;
unsigned long hsvChangedMillis;
#define SINGLE_CHANNEL_CHANGE_DELAY 50

//XML group messages:
//Message 1
byte statusM = 0;   // a bit for every message, false = wait and do nothing, true = show message1Value
typedef struct msg {
    uint8_t newValue; //0 = all LEDs off, 1-255 corresponds to percentage of leds (255 = all LEDs are on, 127 = only 50% of LEDs are on)
    uint8_t lastValue;
    uint16_t ledFirst;
    int16_t ledCnt;
    color_t ledColor;
    bool blink;
} msg_t;
msg_t msg[MESSAGES];

//XML group: Power supply control
bool allLedsOff = true;
bool powerSupplyReady;
bool powerSupplyTurnOn;
bool powerSupplyTurnOff;
bool powerSupplyControl;
unsigned long powerSupplyOffDelay; 
unsigned long powerSupplyOffMillis;
unsigned long psStateCheckedMillis;
bool lastPowerSupplyReady;

bool onMeansDay;
bool sendOnStartup;

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


void setup()
{
    dbg_init();
#ifdef DBG_PRINT
    ArduinoPlatform::SerialDebug = &DBG_SERIAL;
#endif

    pinMode(POWER_SUPPLY_PIN, INPUT);

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
        go_Dimmer_Switch.dataPointType(DPT_Switch);
        go_Dimmer_Switch.callback(dimmSwitchCallback);

        go_Dimmer_Dimming_relativ.dataPointType(DPT_Control_Dimming);
        go_Dimmer_Dimming_relativ.callback(dimmRelCallback);

        go_Dimmer_Dimming_absolute.dataPointType(DPT_Percent_U8);
        go_Dimmer_Dimming_absolute.callback(dimmAbsCallback);

        go_Dimmer_Switch_status.dataPointType(DPT_Switch);
        go_Dimmer_Dimm_status.dataPointType(DPT_Percent_U8);

        go_DimmScene_Dimm_Scene.dataPointType(DPT_SceneNumber);
        go_DimmScene_Dimm_Scene.callback(dimmSceneCallback);

        go_Scene_Value.dataPointType(DPT_SceneNumber);
        go_Scene_Value.callback(sceneCallback);

        go_Red_Value.dataPointType(DPT_Percent_U8);
        go_Red_Value.callback(colorChannelCallback);
        go_Green_Value.dataPointType(DPT_Percent_U8);
        go_Green_Value.callback(colorChannelCallback);
        go_Blue_Value.dataPointType(DPT_Percent_U8);
        go_Blue_Value.callback(colorChannelCallback);
        go_White_Value.dataPointType(DPT_Percent_U8);
        go_White_Value.callback(colorChannelCallback);

        go_RGB_RGB_Value.dataPointType(DPT_Colour_RGB);
        go_RGB_RGB_Value.callback(rgbCallback);

        go_RGBW_RGBW_Value.dataPointType(DPT_Colour_RGBW);
        go_RGBW_RGBW_Value.callback(rgbwCallback);

        go_Scene_Scene_status.dataPointType(DPT_SceneNumber);
        go_Power_supply_Switch.dataPointType(DPT_Switch);

        for (byte mc = 0; mc < MESSAGES; mc++) {
            knx.getGroupObject(NUM_Message_1_Switch + mc*MSGGOCNT).dataPointType(DPT_Switch);
            knx.getGroupObject(NUM_Message_1_Switch + mc*MSGGOCNT).callback(msgCallback);
            knx.getGroupObject(NUM_Message_1_Percentage + mc*MSGGOCNT).dataPointType(DPT_Percent_U8);
            knx.getGroupObject(NUM_Message_1_Percentage + mc*MSGGOCNT).callback(msgCallback);
            knx.getGroupObject(NUM_Message_1_Color_RGB + mc*MSGGOCNT).dataPointType(DPT_Colour_RGB);
            knx.getGroupObject(NUM_Message_1_Color_RGB + mc*MSGGOCNT).callback(msgCallback);
            knx.getGroupObject(NUM_Message_1_Color_RGBW + mc*MSGGOCNT).dataPointType(DPT_Colour_RGBW);
            knx.getGroupObject(NUM_Message_1_Color_RGBW + mc*MSGGOCNT).callback(msgCallback);
        }

        go_DayNight_Day_Night.dataPointType(DPT_Switch);
        go_DayNight_Day_Night.callback(dayNightCallback);

        //XML group: LED
        ledType = PARMVAL_ledType();
        if (ledType == 0)
        {
            rgbw = false;
            whiteOnly = true;
            ledType = NEO_GRB;
        }
        else
        {
            const byte RGBTypes[] = {NEO_RGB, NEO_RBG, NEO_GRB, NEO_GBR, NEO_BRG, NEO_BGR};
            rgbw = true;
            for (byte idx = 0; idx < sizeof(RGBTypes)/sizeof(*RGBTypes); idx ++)
            {
                if (ledType == RGBTypes[idx])
                {
                    rgbw = false;
                    break;
                }
            }
        }
        
        
        numberLeds = PARMVAL_numbersLedsStrip();
        firstOnValue = PARMVAL_firstOnValue();
        maxR = PARMVAL_rCorrection();
        maxG = PARMVAL_gCorrection();
        maxB = PARMVAL_bCorrection();
        maxW = PARMVAL_wCorrection();
        gammaCorrection = PARMVAL_gammaCorrection() * 0.1;
        mixedWhite.c.r = PARMVAL_wr();
        mixedWhite.c.g = PARMVAL_wg();
        mixedWhite.c.b = PARMVAL_wb();
        //XML group: Dimmer
        dimmer.setDurationAbsolute(softOnOffTimeList[PARMVAL_timeSoft()] * 100);
        dimmer.setDurationRelative(relDimTimeList[PARMVAL_timeRel()] * 1000);
        dimmer.setValueFunction(&setBrightness);
        valueMinDay = PARMVAL_dayMin();
        valueMaxDay = PARMVAL_dayMax();
        valueMinNight = PARMVAL_nightMin();
        valueMaxNight = PARMVAL_nightMax();
        onMeansDay = PARMVAL_dayIsOn();
        //set day values until we know if it is day or night
        setDayNightValues(false);
        dimmScene = PARMVAL_dimmScene();

        //XML group: User colors
#define UCSIZE (4 * 4)
        for (byte uc = 0; uc < USERCOLORS; uc++) {
            userColors[uc].rgbw = 
                ((uint8_t)knx.paramInt(PARM_uc1r + uc * UCSIZE)) << 16
                |((uint8_t)knx.paramInt(PARM_uc1g + uc * UCSIZE)) << 8
                |((uint8_t)knx.paramInt(PARM_uc1b + uc * UCSIZE))
                |((uint8_t)knx.paramInt(PARM_uc1w + uc * UCSIZE) << 24)
                ;
        }
        //XML group: Messages:
#define MSGSIZE (6 * 4 + 1)
        for (byte mc = 0; mc < MESSAGES; mc++) {
            msg[mc].ledFirst     = knx.paramInt(PARM_m1first + MSGSIZE * mc) - 1; //Code: count from 0.., Suite: Count from 1..
            msg[mc].ledCnt       = (int32_t)knx.paramInt(PARM_m1cnt + MSGSIZE * mc);
            msg[mc].ledColor.c.r = knx.paramInt(PARM_m1r + MSGSIZE * mc);
            msg[mc].ledColor.c.g = knx.paramInt(PARM_m1g + MSGSIZE * mc);
            msg[mc].ledColor.c.b = knx.paramInt(PARM_m1b + MSGSIZE * mc);
            msg[mc].ledColor.c.w = knx.paramInt(PARM_m1w + MSGSIZE * mc);
            msg[mc].blink        = knx.paramByte(PARM_m1blink + MSGSIZE * mc);
        }
        //XML group: power supply
        powerSupplyControl = PARMVAL_psControl();
        powerSupplyOffDelay = knx.paramInt(PARM_psDelay) * 60000;
        if(!powerSupplyControl){
            powerSupplyReady = true;
        }

        sendOnStartup = PARMVAL_statusOnStart();
        

        dbg_print(F("LED_Type: 0x%02x"), ledType);
        dbg_print(F("LED_Count: %d"), numberLeds);
        dbg_print(F("Gamma: %f"), gammaCorrection);

        setDimmingCurves();
        initStrip(numberLeds, ledType);
        // Black by default
        setAll(black);
        // Off by default
        neopixels->setBrightness(0);
        pixelsShow = true;
    }
    else
    {
        testStrip();
    }

    // pin or GPIO the programming led is connected to. Default is LED_BUILTIN
    knx.ledPin(PROG_LED_PIN);
    // is the led active on HIGH or low? Default is LOW
    knx.ledPinActiveOn(HIGH);
    // pin or GPIO programming button is connected to. Default is 0
    knx.buttonPin(PROG_BUTTON_PIN);
    // RISING or FALLING edge to react
    knx.buttonPinInterruptOn(PROG_BUTTON_INT);
    // The UART to use
    knx.platform().knxUart(&SerialKNX);

    // start the framework.
    knx.start();

    if (!knx.configured())
    {
        knx.progMode(true);
    }

}

void powerSupply(){

    {
        bool currentPowerReady = POWER_SUPPLY_PIN_ACTIVE digitalRead(POWER_SUPPLY_PIN);
        if (lastPowerSupplyReady != currentPowerReady)
        {
            psStateCheckedMillis = millis();
            lastPowerSupplyReady = currentPowerReady;
            dbg_print(F("Pin changed: %d"), currentPowerReady);
        }
        if (psStateCheckedMillis && (millis() - psStateCheckedMillis >= 100))
        {
            psStateCheckedMillis = 0;
            if (currentPowerReady != powerSupplyReady)
            {
                powerSupplyReady = currentPowerReady;
                dbg_print(F("PowerSupply ready: %d"), powerSupplyReady);
                if (!powerSupplyReady)
                {
                    // No power anymore => switch off
                    dimmer.taskOff();
                    dimmer.task();
                    dimmer.task();
                    setAll(black);
                    pixelsShow = true;
                    showPixels();
                }
            }
        }
    }

    if(powerSupplyControl){
        if(!lastStaticColor.rgbw &&
           !msg[0].lastValue && !msg[1].lastValue && !msg[2].lastValue && !msg[3].lastValue && currentTask == TASK_IDLE){
            allLedsOff = true;
        }else{
            allLedsOff = false;
        }
        if(powerSupplyTurnOn && !powerSupplyReady){
            go_Power_supply_Switch.value(true);
            dbg_print(F("Turn PS on!"));
            powerSupplyTurnOn = false;
        }
        if(powerSupplyReady && allLedsOff && !powerSupplyTurnOff){//power supply is on and all LEDs are off => start to 'turn off power supply' routine
            powerSupplyTurnOff = true;
            powerSupplyOffMillis = millis();
            dbg_print(F("All LEDs are off, start 'turn off power supply' routine"));
        }
        if(powerSupplyReady && !allLedsOff && powerSupplyTurnOff){//power supply is on and some LEDs are on => PS stays on
            powerSupplyTurnOff = false;
            dbg_print(F("Some LEDs are on, stop 'turn off power supply' routine"));
        }
        if(!powerSupplyReady && allLedsOff && powerSupplyTurnOff){//stop "power off" routine, PS is already off
            dbg_print(F("All LEDs are off, Power Supply is also off, stop 'turn off power supply' routine"));
            powerSupplyTurnOff = false;
        }
        if(powerSupplyTurnOff && (millis() - powerSupplyOffMillis) >= powerSupplyOffDelay){
            dbg_print(F("Time is over, turn PS off"));
            go_Power_supply_Switch.value(false);
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

    if (!isDimmerOff() || powerSupplyReady)
    {
        // Handle dimming
        dimmer.task();
        // Run tasks 
        taskFunction();
    }
    powerSupply();
    if (rgbwSingleChannelChanged)
    {
        if (millis() - rgbwChangedMillis > SINGLE_CHANNEL_CHANGE_DELAY && !acceptNewRGBW)
        {
            dbg_print(F("apply new rgb(w) values"));
            dbg_print(F("newRGBW 0x%08lx"), newRGBW.rgbw);
            dbg_print(F("valuesRGBW 0x%08lx"), valuesRGBW.rgbw);

            valuesRGBW.rgbw = newRGBW.rgbw;
            acceptNewRGBW = true;
            rgbwSingleChannelChanged = false;
        }
    }
    if (hsvSingleValueChanged)
    {
        if (millis() - hsvChangedMillis > SINGLE_CHANNEL_CHANGE_DELAY && !acceptNewHSV)
        {
            dbg_print(F("apply new hsv values"));

            valuesHSV[0] = newHSV[0];
            valuesHSV[1] = newHSV[1];
            valuesHSV[2] = newHSV[2];
            acceptNewHSV = true;
            hsvSingleValueChanged = false;
        }
    }
    if (dimmer.updateAvailable())
    {
        dimmer.resetUpdateFlag();

        byte dimmValue = dimmer.getCurrentValue();

        if ((lastDimmValue != 0) != (dimmValue != 0))
        {
            go_Dimmer_Switch_status.value(dimmValue != 0);
            dbg_print(F("Send dimmer status: %d"), dimmValue != 0);
        }
        if (lastDimmValue != dimmValue)
        {
            lastDimmValue = dimmValue;
            go_Dimmer_Dimm_status.value(dimmValue);
            dbg_print(F("Send dimmer value status: %d"), dimmValue);
        }
        
        if (!dimmValue) {
            // Switch LEDs off; a message might keep them on
            currentTask = ALL_OFF;
            sendSceneNumber = ALL_OFF; //all off
        }
    }
    if (sendSceneNumber < 64)
    {
        dbg_print(F("Send scene status: %d"), sendSceneNumber);
        go_Scene_Scene_status.value(sendSceneNumber);
        sendSceneNumber = 0xFF;
    }
    if (millis() - rgbwStateMillis > STATE_CHANGE_DELAY && valuesRGBW.rgbw != valuesRGBWState.rgbw)
    {
        rgbwStateMillis = millis();
        valuesRGBWState.rgbw = valuesRGBW.rgbw;
        go_RGBW_RGBW_Status.valueNoSend((byte)15, Dpt(251, 600, 1));
        go_RGBW_RGBW_Status.value(valuesRGBW.rgbw, Dpt(251, 600, 0));
    }
}

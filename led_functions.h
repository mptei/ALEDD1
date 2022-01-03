/*
byte mapByte(byte x, byte in_min, byte in_max, byte out_min, byte out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
*/

word getLogValue(byte index, float gamma, byte startValue, word maxValue, word steps){
    if (index >0){
        word result = round(pow((float)index / (float)(steps - 1.0), gamma) * (float)(maxValue - startValue) + startValue);
        if(result > maxValue)
            return maxValue;
        else
            return result;
    }else{
        return 0;
    }
}


void setDimmingCurves(){
    word maxSteps = 256;
    for(word i = 0; i < maxSteps; i++){
        curveR[i] = getLogValue(i, gammaCorrection, firstOnValue, maxR, maxSteps);
        curveG[i] = getLogValue(i, gammaCorrection, firstOnValue, maxG, maxSteps);
        curveB[i] = getLogValue(i, gammaCorrection, firstOnValue, maxB, maxSteps);
        curveW[i] = getLogValue(i, gammaCorrection, firstOnValue, maxW, maxSteps);
    }
}

//test witch strip we have
/* choose what you see:
 *  LED 1 2 3 4 5 6 (x = OFF)
 *      R G B W x x -> RGBW
 *      G R B W x x -> GRBW
 *      R B x G x R -> RGB
 *      G B x R x G -> GRB
 */
void testStrip(){
    //there are only 600 LEDs alowed -> hardcoded
    if(neopixels) delete neopixels;
    neopixels = new Adafruit_NeoPixel_ZeroDMA(600, LED_STRIP_PIN, NEO_RGBW);
    if (!neopixels->begin(&sercom4, SERCOM4, SERCOM4_DMAC_ID_TX, LED_STRIP_PIN, SPI_PAD_0_SCK_3, PIO_SERCOM_ALT)) {
        dbg_print(F("NeoPixels begin failed"));
    }
    neopixels->setPixelColor(0, 255 , 0, 0, 0);
    neopixels->setPixelColor(1, 0, 255, 0, 0);
    neopixels->setPixelColor(2, 0, 0, 255, 0);
    neopixels->setPixelColor(3, 0, 0, 0, 255);
    for(int i = 4; i < numberLeds; i++){
        if((i % 10 == 0) && (i % 50 != 0)) neopixels->setPixelColor(i-1, 0, 255, 0, 0); //each 10.(10,20,30,40,60,70...) is green
        if(i % 50 == 0) neopixels->setPixelColor(i-1, 0, 0, 255, 0); //each 50.(50,100,150...) is blue
    }
    neopixels->setPixelColor(numberLeds-1, 255,0,0,0); //last one is red
    neopixels->show();
}

void showProgrammedLeds(){
    neopixels->setPixelColor(0, 255, 0, 0, 0);
    neopixels->setPixelColor(1, 0, 255, 0, 0);
    neopixels->setPixelColor(2, 0, 0, 255, 0);
    neopixels->setPixelColor(3, 0, 0, 0, 255);
    //each tens LED is red (10,20,30,40,60,70...)
    for(int i = 4; i < numberLeds; i++){
        if((i % 10 == 0) && (i % 50 != 0)) neopixels->setPixelColor(i-1, 0, 255, 0, 0); //each 10. is red
        if(i % 50 == 0) neopixels->setPixelColor(i - 1, 0, 0, 255, 0); //each 50. is blue
    }
    neopixels->setPixelColor(numberLeds - 1, 255, 0, 0, 0); //last one is green
    neopixels->show();
}
    

//led-stripe initialisation
void initStrip(word pixel, byte type){
    if(neopixels) delete neopixels;
    neopixels = new Adafruit_NeoPixel_ZeroDMA(pixel, LED_STRIP_PIN, type);
    pixelcanvas = new NeoPixelPainterCanvas(neopixels);
    pixelcanvas2 = new NeoPixelPainterCanvas(neopixels);
    pixelbrush = new NeoPixelPainterBrush(pixelcanvas);
    pixelbrush2 = new NeoPixelPainterBrush(pixelcanvas); 
    pixelbrush3 = new NeoPixelPainterBrush(pixelcanvas);
    pixelbrush4on2 = new NeoPixelPainterBrush(pixelcanvas2);
#ifdef DEVELPMENT    
    neopixels->begin(&sercom5, SERCOM5, SERCOM5_DMAC_ID_TX,  6,  7, A5, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_0, PIO_SERCOM);
#else
    //neopixels->begin(&sercom4, SERCOM4, SERCOM4_DMAC_ID_TX, 22, 23, 24, SPI_PAD_0_SCK_3, SERCOM_RX_PAD_2, PIO_SERCOM_ALT);
    neopixels->begin(&sercom4, SERCOM4, SERCOM4_DMAC_ID_TX, LED_STRIP_PIN, SPI_PAD_0_SCK_3, PIO_SERCOM_ALT);
#endif
    neopixels->show();
    dbg_print(F("initPixel done"));
}

void setDayNightValues(bool night){
    if(night){
        //night values
        valueMin = valueMinNight;
        valueMax = valueMaxNight;
    }else{
        //day values
        valueMin = valueMinDay;
        valueMax = valueMaxDay;
    }
}

void taskSoftOnOff(byte value){
    if(value)
        dimmer.taskSoftOn();
    else
        dimmer.taskSoftOff();
}

void taskDimUpDownStop(bool up, byte step){
    dbg_print(F("step: %d, direction: %d"), step, up);
    //if step == 0 then stop
    if(step == 0)
        dimmer.taskStop();
    else
        if(up)
            dimmer.taskDimUp();
        else
            dimmer.taskDimDown();
}

void taskNewValue(byte value){
    dimmer.taskNewValue(value);
}

byte clipValue(byte w, byte color)
{
    if (color + w > 255) return 255;
    return color + w;
}

color_t colorCorrection(color_t color) 
{
    color_t corrected;
    //if we have RGB only, try to display mixed white
    if (!rgbw) 
    {
        // Convert white channel into RGB and add this to the color channels and clip them.
        color.c.r = clipValue(getLogValue(color.c.w, gammaCorrection, 1, mixedWhite.c.r, 256), color.c.r);
        color.c.g = clipValue(getLogValue(color.c.w, gammaCorrection, 1, mixedWhite.c.g, 256), color.c.g);
        color.c.b = clipValue(getLogValue(color.c.w, gammaCorrection, 1, mixedWhite.c.b, 256), color.c.b);
    }    
    corrected.c.r = curveR[color.c.r];
    corrected.c.g = curveG[color.c.g];
    corrected.c.b = curveB[color.c.b];
    corrected.c.w = curveW[color.c.w];
    return corrected;
}

void setAll(color_t color){
    currentTask = TASK_IDLE; //TASK_IDLE
    color_t corrected = colorCorrection(color);
    for(int i = 0; i < numberLeds; i++){
        neopixels->setPixelColor(i, corrected.rgbw);
    }
    //save color, for messages
    lastStaticColor = corrected;
    // Set static color value
    valuesRGBW = color;
    newRGBW = color;
    pixelsShow = true;
}

void setAllHsv(byte h, byte s, byte v){
    dbg_print(F("setAllHsv H: %d, S: %d, V: %d"), h, s, v);
    currentTask = TASK_IDLE; //TASK_IDLE
    byte newRGB[3];
    hsvToRgb(h, s, v, newRGB);
 
    for(int i = 0; i < numberLeds; i++){
        neopixels->setPixelColor(i, newRGB[R], newRGB[G], newRGB[B]);
        pixelsShow = true;
    }
}

void setBrightness(byte value){
    byte dimmValue = (valueMax - valueMin) * value / 255 + valueMin;
    neopixels->setBrightness(dimmValue);
    pixelsShow = true;
}

/*neopixels->show() tranfers buffer data to physical LEDs
we can modify buffer before this step to show e.g. message-animation on top of normal animation
in addition we are setting ledsOn status.

*/

void showPixels (){
    if(pixelsShow){
  	    pixelsShow = false;
  	    neopixels->show();
    }
}

/*
this function overrides selected pixels with specific color
attention: if message color matches current stripe color, it's not possible to identify message state
*/

void setMessageLeds(msg_t &msg){
/*
  we can display up to 2 messages on a single strip
  each message has it own stripe range
  the range is defined in KONNEKTING Suite "from LED number x up to LED number y"
  message 2 will override (or part of) message 1 if message 2 range overlaps message 1 range

Examples:

LED strip with 20 LEDs (0..19):
 0   1   2   3   4
19               5 
18               6 
17               7 
16               8 
15               9 
14  13  12  11  10

Message 1 range: 10 - 4 => 7 LEDs (25%: 10,9; 50%: 10,9,8,7; 75%: 10,9,8,7,6)
Message 2 range: 14 - 0 => 7 LEDs, not possible in direct way. Please set LED 0 as LED 20, LED 1 as LED 21 and so on... (LED number from LED 0 + string length)
                 (25%: 14,15; 50%: 14,15,16,17; 75%: 14,15,16,17,18; 100%: 14,15,16,17,18,19,0)
                 if LED 0 will be set as 0, than the range will be 15 LEDs: 14,13,12...2,1,0

*/
    color_t corrected = colorCorrection(msg.ledColor);
    if(msg.newValue){ //if 0, do nothing, we've allready wiped with animation or static color
        if(msg.ledCnt > 0){
            uint16_t amount = (msg.ledCnt * msg.newValue + 254) / 255;
            int16_t led = msg.ledFirst;
            dbg_print(F("LED: %d, count: %d"), led, amount);
            while (amount-- > 0) {
                neopixels->setPixelColor(led++ % numberLeds, corrected.rgbw);
            }
        }else if (msg.ledCnt < 0) {
            uint16_t amount = (msg.ledCnt * -1 * msg.newValue + 254) / 255;
            int16_t led = msg.ledFirst;
            dbg_print(F("LED: %d, count: %d"), led, amount);
            while (amount-- > 0) {
                neopixels->setPixelColor((led-- + numberLeds) % numberLeds, corrected.rgbw);
            }
        }
    }
}

void showMessage(){
    //set color only if we are in NOT in WAIT state 
    if(statusM){
        //just overlay with messages, animation will do "wipe"
        if(RAINBOW <= currentTask && currentTask <= WHIREMIDDLEOFF){
            for (byte mc = 0; mc < MESSAGES; mc++) {
                setMessageLeds(msg[mc]);
                msg[mc].lastValue = msg[mc].newValue;
            }
            pixelsShow = true; //show result 
        }
        //turn Message LEDs on/off if it's not an animation (static color)
        if((ALL_OFF  <= lastTaskBeforeMessage && lastTaskBeforeMessage <= USER_COLOR_5) || 
           (TASK_RGBW <= lastTaskBeforeMessage && lastTaskBeforeMessage <= TASK_HSV )){
            //"wipe" messages if we will show any less message LEDs as before
            for (byte mc = 0; mc < MESSAGES; mc++) {
                if (msg[mc].lastValue > msg[mc].newValue) {
                    setAll(lastStaticColor);
                    dbg_print(F("Message %d: set last static color: WRGB: %08lx, statusM: %d"), mc+1, lastStaticColor.rgbw, statusM & (1<<mc));
                    break;
                }
            }            
            //and show messages on top of static color
            for (byte mc = 0; mc < MESSAGES; mc++) {
                setMessageLeds(msg[mc]);
                if(statusM & (1<<mc)) {
                    statusM &= ~(1<<mc); //set message WAIT state
                    msg[mc].lastValue = msg[mc].newValue;
                }
            }
            pixelsShow = true; //show result 
        }
    }
}

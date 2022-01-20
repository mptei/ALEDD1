/*
byte mapByte(byte x, byte in_min, byte in_max, byte out_min, byte out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
*/

uint16_t getLogValue(uint16_t index, float gamma, uint16_t startValue, uint16_t maxValue, uint16_t steps){
    if (0 == index) return 0;

    uint16_t result = round(pow((float)index / (float)(steps - 1.0), gamma) * (float)(maxValue - startValue) + startValue);
    if(result > maxValue)
        return maxValue;
    else
        return result;
}


void setDimmingCurves(){
    uint8_t whiteFac = whiteOnly ? 3 : 1;
    for(uint16_t i = 0; i < DIMMSTEPCOUNT; i++){
        curveR[i] = getLogValue(i, gammaCorrection, firstOnValue, maxR, DIMMSTEPCOUNT);
        curveG[i] = getLogValue(i, gammaCorrection, firstOnValue, maxG, DIMMSTEPCOUNT);
        curveB[i] = getLogValue(i, gammaCorrection, firstOnValue, maxB, DIMMSTEPCOUNT);
        curveW[i] = getLogValue(i, gammaCorrection, firstOnValue, maxW*whiteFac, DIMMSTEPCOUNT);
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
void initStrip(uint16_t pixelCount, uint8_t type){
    if(neopixels) delete neopixels;
    neopixels = new Adafruit_NeoPixel_ZeroDMA(pixelCount, LED_STRIP_PIN, type);
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

uint8_t mixAndClipValue(uint8_t w, uint8_t color)
{
    if (255 - color < w) return 255;
    return color + w;
}

color_t colorCorrection(color_t color) 
{
    if (whiteOnly)
    {
        // Transfer white into RGB channels
        color_t corrected;
        uint16_t whiteVal = curveW[color.c.w];
        dbg_print(F("whiteVal 0x%04x"), whiteVal);
        corrected.c.r = corrected.c.g = corrected.c.b = whiteVal / 3;
        uint8_t remain = whiteVal % 3;
        corrected.c.r += remain / 2;
        corrected.c.g += remain / 2;
        remain = remain % 2;
        corrected.c.r += remain;
        corrected.c.w = 0;
        return corrected;
    }

    //if we have RGB only mix in the white channel
    if (!rgbw) 
    {
        // Convert white channel into RGB and add this to the color channels and clip them.
        color.c.r = mixAndClipValue(getLogValue(color.c.w, gammaCorrection, 1, mixedWhite.c.r, 256), color.c.r);
        color.c.g = mixAndClipValue(getLogValue(color.c.w, gammaCorrection, 1, mixedWhite.c.g, 256), color.c.g);
        color.c.b = mixAndClipValue(getLogValue(color.c.w, gammaCorrection, 1, mixedWhite.c.b, 256), color.c.b);
        color.c.w = 0;
    }    

    color_t corrected;
    corrected.c.r = curveR[color.c.r];
    corrected.c.g = curveG[color.c.g];
    corrected.c.b = curveB[color.c.b];
    corrected.c.w = curveW[color.c.w];
    return corrected;
}

color_t setAllRaw(color_t color){
    color_t corrected = colorCorrection(color);
    for(int i = 0; i < numberLeds; i++){
        neopixels->setPixelColor(i, corrected.rgbw);
    }
    pixelsShow = true;
    return corrected;
}

void setAll(color_t color){
    dbg_print(F("setAll 0x%08lx"), color.rgbw);
    currentTask = TASK_IDLE; //TASK_IDLE
    setAllRaw(color);

    // Set static color value
    valuesRGBW = color;
    newRGBW = color;
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

bool isAnimationTask(uint8_t task) {
    return RAINBOW <= task && task <= WHIREMIDDLEOFF;
}

bool isAnimationRunning()
{
    return isAnimationTask(currentTask);
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

color_t dimmColor(color_t in, uint16_t dimmValue)
{
    if (dimmValue == 0) return in;
    
    color_t out;
    out.c.r = in.c.r * dimmValue / (DIMMSTEPCOUNT-1);
    out.c.g = in.c.g * dimmValue / (DIMMSTEPCOUNT-1);
    out.c.b = in.c.b * dimmValue / (DIMMSTEPCOUNT-1);
    out.c.w = in.c.w * dimmValue / (DIMMSTEPCOUNT-1);
    return out;
}

void setMessageLeds(msg_t &msg, uint16_t dimmValue){
    if(msg.val && msg.ledCnt) { //if 0, do nothing, we've allready wiped with animation or static color
        color_t corrected = colorCorrection(dimmColor(msg.ledColor, dimmValue));
        // How many LEDs should go on; + 254 to have all val > 0 give an amount > 0
        uint16_t amount = (abs(msg.ledCnt) * msg.val + 254) / 255;
        int16_t led = msg.ledFirst;
        if (msg.ledCnt < 0) {
            led = (led + 1 + numberLeds - amount) % numberLeds;
        }
        while (amount-- > 0) {
            neopixels->setPixelColor(led++ % numberLeds, corrected.rgbw);
        }
    }
}

void showMessage(uint16_t dimmValue){
    if (!isAnimationRunning()) {
        // repaint static color
        color_t color = dimmColor(valuesRGBW, dimmValue);
        setAllRaw(color);
    } else {
        // Animations using neopixel brightness => switch of dimming
        dimmValue = 0;
    }

    // Overwrite messages
    for (byte mc = 0; mc < MESSAGES; mc++) {
        setMessageLeds(msg[mc], dimmValue);
    }
}

void setBrightness(uint8_t value){
    uint8_t dimmValue = (valueMax - valueMin) * value / 255 + valueMin;
    if (0 == dimmValue) {
        neopixels->setBrightness(0);
    }
    else
    {
        if (isAnimationRunning())
        {
            // For animations use the global neopixel brightness
            uint16_t pixelVal = curveW[dimmValue];
            if (whiteOnly)
            {
                pixelVal /= 3;
            }
            neopixels->setBrightness(pixelVal);
        }
        else
        {
            // For static set the neopixel brightness to full
            neopixels->setBrightness(255);
            // and repaint the static color
            showMessage(dimmValue);
        }
    }
    pixelsShow = true;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t hueDisc(byte pos) {
    pos = 255 - pos;
    if(pos < 85) {
        return neopixels->Color(255 - pos * 3, 0, pos * 3);
    }
    if(pos < 170) {
        pos -= 85;
        return neopixels->Color(0, pos * 3, 255 - pos * 3);
    }
    pos -= 170;
    return neopixels->Color(pos * 3, 255 - pos * 3, 0);
}

void rainbow(){
    static uint16_t h = 0;
    if (initialized == false) {
        initialized = true;
        lastAnimMillis = 0;
        h = 0;
    }
    if(millis() - lastAnimMillis >= 50){
        for(uint16_t i = 0; i < numberLeds; i++) {
            neopixels->setPixelColor(numberLeds - 1 - i, hueDisc(((i * 256 / numberLeds) + h) & 255));
        }
        h++;
        if(h > 1280) h = 0;
        pixelsShow = true;
        lastAnimMillis = millis();
    }
}

void singlergb(){
    static byte h = 0;
    if (initialized == false) {
        initialized = true;
        lastAnimMillis = 0;
    }
    if(millis() - lastAnimMillis >= 100){
        for (uint16_t i = 0; i < numberLeds; i++) {
            neopixels->setPixelColor(i, hueDisc(h));
        }
        h++;
        pixelsShow = true;
        lastAnimMillis = millis();
    }
}

void sparkler() {
    if (initialized == false) {
        initialized = true;
        dbg_print(F("sparkler init"));
        pixelbrush->setSpeed(300 * numberLeds / 50); //original 600
        pixelbrush->setFadeout(true); //sparkles fade in
        pixelbrush->setFadein(true);  //and fade out immediately after reaching the set brightness
        pixelbrush->setBounce(true);
    }
  
    //set a new brush color in each loop
    brushcolor.h = random(255); //random color
    brushcolor.s = random(130); //random but low saturation, giving white-ish sparkles
    brushcolor.v = random(200); //random (peak) brighness
  
    pixelbrush->setColor(brushcolor);
    pixelbrush->setFadeSpeed(random(100) + 150); //set a new fadespeed with some randomness
  
    neopixels->clear();
  
    pixelbrush->paint(); //paint the brush to the canvas (and update the brush, i.e. move it a little)
    pixelcanvas->transfer(); //transfer (add) the canvas to the neopixels
  
    pixelsShow = true;
}

void twinkystars() {
    if (initialized == false) {
        initialized = true;
        dbg_print(F("twinkystars init"));
        neopixels->clear();
        pixelbrush->setSpeed(0); //do not move automatically
        pixelbrush->setFadein(true); //fade in
        pixelbrush->setFadeout(true); //and fade out
    }
  
    if (rand() % 100 == 0) {
        brushcolor.h = rand();
        brushcolor.s = random(40); //set low saturation, almost white
        brushcolor.v = random(200) + 20; //set random brightness
        pixelbrush->setColor(brushcolor);
        pixelbrush->moveTo(random(numberLeds)); //move the brush to a new, random pixel
        pixelbrush->setFadeSpeed(random(10) + 5); //set random fade speed, minimum of 5
    }
  
    //add a background color by setting all pixels to a color (instead of clearing all pixels):
    for (int i = 0; i < numberLeds; i++) {
        neopixels->setPixelColor(i, 1, 0, 6); //color in RGB: dark blue
    }
  
      pixelbrush->paint(); //paint the brush to the canvas
      pixelcanvas->transfer(); //transfer (add) the canvas to the neopixels
    
      pixelsShow = true;

}

void chaser() {
    if (initialized == false) {
        initialized = true;
        dbg_print(F("chaser init"));
        neopixels->clear();
        brushcolor.h = random(255); //choose random color once
        brushcolor.s = 255; //full staturation
        brushcolor.v = 150;
    
        //initialize the first brush to move and paint a color, no fading
        pixelbrush->setSpeed(8000 / numberLeds); //moving speed //900
        pixelbrush->setColor(brushcolor);
        pixelbrush->setFadeout(true);
        pixelbrush->setFadein(true);
        pixelbrush->setBounce(true);
        pixelbrush2->moveTo(0); //move the brush to pixel 0
        //initialize the second brush to move at the same speed but starting at a different position (default position is 0)
        brushcolor.v = 0; //zero intensity = black
        pixelbrush2->setSpeed(8000 / numberLeds);
        pixelbrush2->setColor(brushcolor);
        pixelbrush2->setFadeout(true);
        pixelbrush2->moveTo(2 * numberLeds / 3); //move the brush
        pixelbrush2->setBounce(true);
  
    }
  
    neopixels->clear();
    pixelbrush->paint(); //apply the paint of the first brush to the canvas (and update the brush)
    pixelbrush2->paint(); //apply the paint of the second brush to the canvas (and update the brush)
    pixelcanvas->transfer(); //transfer the canvas to the neopixels
    showPixels();

}
void huefader() {
    static unsigned int lastposition = 0;
    if (pixelbrush->getPosition() == 0 && lastposition > 0) initialized = false;
    lastposition = pixelbrush->getPosition();
    if (initialized == false) {
        initialized = true;
        dbg_print(F("huefader init"));
    
        HSV brushcolor;
    
        brushcolor.h = random(255); //random color
        brushcolor.s = 255; //full saturation
        brushcolor.v = 130; //medium brightness
    
        pixelbrush->setSpeed(random(150) + 150); //random movement speed
        pixelbrush->setFadeSpeed(random(10) + 20); //set random fading speed
        pixelbrush->setColor(brushcolor); //update the color of the brush
        pixelbrush->setFadeHueNear(true); //fade using the near path on the colorcircle
    
        //second brush paints on the same canvas
        brushcolor.h = random(255);
        pixelbrush2->setSpeed(random(150) + 150);
        pixelbrush2->setFadeSpeed(random(10) + 20);
        pixelbrush2->setColor(brushcolor);
        pixelbrush2->setFadeHueNear(true); //fade using the near path on the colorcircle
        //pixelbrush->setFadeHueFar(true); //fade using the far path on the colorcircle (if both are set, this path is chosen)
        pixelbrush2->setBounce(true); //bounce this brush at the end of the strip
    }
    neopixels->clear();
    pixelbrush->paint(); //apply the paint of the first brush to the canvas (and update the brush)
    pixelbrush2->paint(); //apply the paint of the second brush to the canvas (and update the brush)
    pixelcanvas->transfer(); //transfer the canvas to the neopixels
    pixelsShow = true;
}

void speedtrails() {
    if (initialized == false) {
        initialized = true;
        dbg_print(F("speedtrails init"));
        int brushspeed = 900;
        brushcolor.h = 0;
        brushcolor.s = 0; //make it white
        brushcolor.v = 150; //medium brightness
        pixelbrush->setColor(brushcolor);
        pixelbrush->setSpeed(brushspeed);
        pixelbrush->setFadeSpeed(250); //fast fading (255 is max.)
        pixelbrush->setFadeHueNear(false); //deactivate hue fading, was activated in last animation
        pixelbrush->setFadeout(true);
        pixelbrush->moveTo(0); //move brush to zero
    
        //second brush
        brushcolor.h = 0; //red
        brushcolor.s = 250;
        brushcolor.v = 150;
        pixelbrush2->setSpeed(brushspeed);
        pixelbrush2->setFadeSpeed(220);
        pixelbrush2->setFadeout(true);
        pixelbrush2->setColor(brushcolor);
        pixelbrush2->moveTo(numberLeds / 3); //move it up one third of the strip
    
        //third brush
        brushcolor.h = 28; //yellow
        brushcolor.s = 255;
        brushcolor.v = 100;
        pixelbrush3->setSpeed(brushspeed);
        pixelbrush3->setFadeSpeed(190);
        pixelbrush3->setFadeout(true);
        pixelbrush3->setColor(brushcolor);
        pixelbrush3->moveTo(2 * numberLeds / 3); //move it up two thirds of the strip
    }
  
    neopixels->clear();
    pixelbrush->paint(); //apply the paint of the first brush to the canvas (and update the brush)
    pixelbrush2->paint(); //apply the paint of the second brush to the canvas (and update the brush)
    pixelbrush3->paint(); //apply the paint of the third brush to the canvas (and update the brush)
    pixelcanvas->transfer(); //transfer the canvas to the neopixels (and update i.e. fade pixels)
    pixelsShow = true;
}

void bouncyballs() {
    static byte skipper = 0;
    if (initialized == false) {
        initialized = true;
        dbg_print(F("bouncyballs init"));
        brushcolor.h = 20; //orange
        brushcolor.s = 240; //almost full saturation
        brushcolor.v = 150; //medium brightness
    
        //first brush
        pixelbrush->setSpeed(0); //zero initial speed
        pixelbrush->setFadeSpeed(150);
        pixelbrush->setFadeout(true);
        pixelbrush->setColor(brushcolor);
        pixelbrush->moveTo(numberLeds - 1); //move to end of the strip
        pixelbrush->setBounce(true); //bounce if either end of the strip is reached
    
        //second brush
        brushcolor.h = 220; //pink
        pixelbrush2->setSpeed(0); //zero initial speed
        pixelbrush2->setFadeSpeed(190);
        pixelbrush2->setFadeout(true);
        pixelbrush2->setColor(brushcolor);
        pixelbrush2->moveTo(numberLeds / 3); //move to one third of the strip
        pixelbrush2->setBounce(true);
    
        //third brush
        brushcolor.h = 70; //green-ish (pure green is 85 or 255/3)
        pixelbrush3->setSpeed(0);
        pixelbrush3->setFadeSpeed(220);
        pixelbrush3->setFadeout(true);
        pixelbrush3->setColor(brushcolor);
        pixelbrush3->moveTo(2 * numberLeds / 3);
        pixelbrush3->setBounce(true);
  
    }
    //apply some gravity force that accelerates the painters (i.e. add speed in negative direction = towards zero pixel)
    if (skipper % 5 == 0) //only apply gravity at some interval to make it slower on fast processors
    {
        //read current speed of each brush and speed it up in negative direction (towards pixel zero)
        pixelbrush->setSpeed(pixelbrush->getSpeed() - 10);
        pixelbrush2->setSpeed(pixelbrush2->getSpeed() - 10);
        pixelbrush3->setSpeed(pixelbrush3->getSpeed() - 10);
  
    }
    skipper++;
    neopixels->clear();
    pixelbrush->paint(); //apply the paint of the first brush to the canvas (and update the brush)
    pixelbrush2->paint(); //apply the paint of the second brush to the canvas (and update the brush)
    pixelbrush3->paint(); //apply the paint of the third brush to the canvas (and update the brush)
    pixelcanvas->transfer(); //transfer the canvas to the neopixels (and update i.e. fade pixels)
    pixelsShow = true;
}

void twobrushcolormixing() {

    if (initialized == false) {
        initialized = true;
    
        brushcolor.s = 255; //full color saturation
        brushcolor.v = 100; //medium-low brightness
        brushcolor.h = 8;
    
        //setup the first brush
        pixelbrush->setSpeed(-750);
        pixelbrush->setSpeedlimit(1000);
        pixelbrush->setFadeSpeed(random(80) + 50);
        pixelbrush->setFadeout(true);
        pixelbrush->setFadein(true);
        pixelbrush->setColor(brushcolor);
        pixelbrush->moveTo(random(numberLeds));
        pixelbrush->setBounce(true);
    
        //setup the second brush
        brushcolor.h = 160;
        pixelbrush4on2->setSpeed(600);
        pixelbrush4on2->setSpeedlimit(1000);
        pixelbrush4on2->setFadeSpeed(random(80) + 50);
        pixelbrush4on2->setFadeout(true);
        pixelbrush4on2->setFadein(true);
        pixelbrush4on2->setColor(brushcolor);
        pixelbrush4on2->moveTo(random(numberLeds));
        pixelbrush4on2->setBounce(true);
    }
  
  
    if (rand() % 10) {
      brushcolor = pixelbrush->getColor();
      brushcolor.h += random(3) - 1; //randomly change hue a little ( ± random(1))
      pixelbrush->setColor(brushcolor);
  
      brushcolor = pixelbrush4on2->getColor();
      brushcolor.h += random(3) - 1; //randomly change hue a little ( ± random(1))
      pixelbrush4on2->setColor(brushcolor);
    }
  
    //slowly change speed of both brushes
    pixelbrush->setSpeed(pixelbrush->getSpeed() + random(6) - 3); //means speed = currentspeed ± random(3)
    pixelbrush4on2->setSpeed(pixelbrush4on2->getSpeed() + random(6) - 3); //means speed = currentspeed ± random(3)
  
    neopixels->clear(); //remove any previously applied paint
    pixelbrush->paint(); //apply the paint of the first brush to its assigned canvas (and update the brush)
    pixelbrush4on2->paint(); //apply the paint of the second brush to  its assigned canvas (and update the brush)
    pixelcanvas->transfer(); //transfer the first canvas to the neopixels
    pixelcanvas2->transfer(); //transfer the sedonc canvas to the neopixels (adding colors, rather than overwriting colors)
    pixelsShow = true;

}

#define WMSHIFT 4
#define WMSTEP 16
static uint32_t wmBrightness;
void whitemiddleon() {
    if (initialized == false) {
        initialized = true;
        wmBrightness = 0;
    }

    wmBrightness ++;

    byte middle = numberLeds / 2;
    uint32_t brightness = wmBrightness >> WMSHIFT;
    for (byte idx = 0; idx <= middle; ++idx) 
    {
        byte rgb = brightness >= 255 ? 255 : brightness;
        uint32_t ledNum;
        if (numberLeds - idx >= middle)
        {
            ledNum = middle + idx;
        }
        else
        {
            ledNum = numberLeds - 1;
        }
        neopixels->setPixelColor(ledNum, rgb, rgb, rgb);

        if (middle >= idx)
        {
            ledNum = middle - idx;
        }
        else
        {
            ledNum = 0;
        }
        neopixels->setPixelColor(ledNum, rgb, rgb, rgb);
        if (!ledNum)
        {
            if (brightness >= 255)
            {
                currentTask = TASK_IDLE;
                sendSceneNumber = WHITE;
            }
        }
        if (brightness > WMSTEP)
        {
            brightness -= WMSTEP;
        }
        else
        {
            brightness = 0;
        }
    }
  
    pixelsShow = true;
}

void whitemiddleoff() {
    if (initialized == false) {
        initialized = true;
        wmBrightness = (255 + numberLeds/2 * WMSTEP) << WMSHIFT;
    }

    wmBrightness --;

    byte middle = numberLeds / 2;
    uint32_t brightness = wmBrightness >> WMSHIFT;
    for (byte idx = 0; idx <= middle; ++idx) 
    {
        byte rgb = brightness >= 255 ? 255 : brightness;
        uint32_t ledNum;
        if (numberLeds - idx >= middle)
        {
            ledNum = middle + idx;
        }
        else
        {
            ledNum = numberLeds - 1;
        }
        neopixels->setPixelColor(ledNum, rgb, rgb, rgb);

        if (middle >= idx)
        {
            ledNum = middle - idx;
        }
        else
        {
            ledNum = 0;
        }
        neopixels->setPixelColor(ledNum, rgb, rgb, rgb);
        if (ledNum == middle)
        {
            if (!brightness)
            {
                currentTask = TASK_IDLE;
                sendSceneNumber = ALL_OFF;
            }
        }
        if (brightness > WMSTEP)
        {
            brightness -= WMSTEP;
        }
        else
        {
            brightness = 0;
        }
    }
  
    pixelsShow = true;

}

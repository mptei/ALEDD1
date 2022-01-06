static const color_t colorTable[] = { black, white, red, yellow, green, cyan, blue, purple, orange };

void taskFunction(){
    if (currentTask <= ORANGE)
    {
        valuesRGBW = colorTable[currentTask];
        currentTask = TASK_IDLE;
        pixelsShow = true;
    }
    else if (currentTask >= USER_COLOR_1 && currentTask < USER_COLOR_1 + USERCOLORS)
    {
        valuesRGBW = userColors[currentTask-USER_COLOR_1];
        currentTask = TASK_IDLE;
        pixelsShow = true;
    }
    else
    {
        switch(currentTask){
            case RAINBOW:
                rainbow();
                break;
            case SINGLERGB:
                singlergb();
                break;
            case TWINKYSTARS:
                twinkystars();
                break;
            case CHASER:
                chaser();
                break;
            case HUEFADER:
                huefader();
                break;
            case SPEEDTRAILS:
                speedtrails();
                break;
            case BOUNCYBALLS:
                bouncyballs();
                break;
            case TWOBRUSHCOLORMIXING:
                twobrushcolormixing();
                break;
            case SPARKLER:
                sparkler();
                break;
            case WHIREMIDDLEON:
                whitemiddleon();
                break;
            case WHIREMIDDLEOFF:
                whitemiddleoff();
                break;
            case TASK_RGBW:
                currentTask = TASK_IDLE;
                pixelsShow = true;
                break;
            case TASK_HSV:
                if(acceptNewHSV){
                    setAllHsv(valuesHSV[0], valuesHSV[1], valuesHSV[2]);
                    acceptNewHSV = false;
                    dbg_print(F("TASK_HSV done"));
                }
                break;
            
            case TASK_IDLE:
            default:
                break;
        }
    }
    if(pixelsShow){
        showMessage(dimmer.getCurrentValue());
        showPixels();
    }
}

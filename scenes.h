static const color_t colorTable[] = { black, white, red, yellow, green, cyan, blue, purple, orange };

void taskFunction(){
    if(currentTask != TASK_IDLE) {
        lastTaskBeforeMessage = currentTask;
        //exit WAIT state
        statusM = (0xFF >> (8-MESSAGES));
    }

    if (currentTask <= ORANGE)
    {
        valuesRGBW = colorTable[currentTask];
        currentTask = TASK_IDLE;
    }
    else if (currentTask >= USER_COLOR_1 && currentTask < USER_COLOR_1 + USERCOLORS)
    {
        valuesRGBW = userColors[currentTask-USER_COLOR_1];
        currentTask = TASK_IDLE;
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
                if(acceptNewRGBW){
                    setAll(valuesRGBW);
                    acceptNewRGBW = false;
                }
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
    showMessage(dimmer.getCurrentValue());
    if(pixelsShow){
        showPixels();
    }
}

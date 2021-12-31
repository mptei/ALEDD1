void taskFunction(){
    if(currentTask != TASK_IDLE) {
        lastTaskBeforeMessage = currentTask;
        //exit WAIT state
        statusM = (0xFF >> (8-MESSAGES));
    }

    switch(currentTask){
        case ALL_OFF:
            setAll(black);
            break;
        case WHITE:
            setAll(white);
            break;
        case RED:
            setAll(red);
            break;
        case YELLOW:
            setAll(yellow);
            break;
        case GREEN:
            setAll(green);
            break;
        case CYAN:
            setAll(cyan);
            break;
        case BLUE:
            setAll(blue);
            break;
        case PURPLE:
            setAll(purple);
            break;
        case ORANGE:
            setAll(orange);
            break;

        case USER_COLOR_1:
        case USER_COLOR_2:
        case USER_COLOR_3:
        case USER_COLOR_4:
        case USER_COLOR_5:
            setAll(userColors[currentTask-USER_COLOR_1]);
            break;
            
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
    showMessage();
    if(pixelsShow){
        showPixels();
    }
}

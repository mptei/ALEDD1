void taskFunction(){
//    Debug.println(F("currentTask: 0x%02X"), currentTask);
    if(currentTask != TASK_IDLE) {
        lastTaskBeforeMessage = currentTask;
        //exit WAIT state
        for (byte mc = 0; mc < MESSAGES; mc++) {
            if (!(statusM & (1<<mc))) statusM |= (1<<mc);
        }
    }
    

    switch(currentTask){
        case ALL_OFF:
            setLeds(0);
            break;
        case WHITE:
            setLeds(255);
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

        case TASK_RGB:
            if(acceptNewRGBW){
//                Debug.println(F("valuesRGBW R: %d, G: %d, B: %d, W: %d"),valuesRGBW[R],valuesRGBW[G],valuesRGBW[B],valuesRGBW[W]);
                setAll(valuesRGBW);
                acceptNewRGBW = false;
                rgbwChanged = false;
                //reset color because of time out (acceptNewRGBW)
                new3Byte[0] = 0;
                new3Byte[1] = 0;
                new3Byte[2] = 0;
                println(F("TASK_RGB done"));
            }
            break;
        case TASK_RGBW:
            if(acceptNewRGBW){
                //first 12 bits are not defined, 4 bits ignored
                println(F("valuesRGBW R: %d, G: %d, B: %d, W: %d"), valuesRGBW[R], valuesRGBW[G], valuesRGBW[B], valuesRGBW[W]);
                setAll(valuesRGBW);
                acceptNewRGBW = false;
                rgbwChanged = false;
                println(F("TASK_RGBW done"));
            }
            break;
        case TASK_HSV:
            if(acceptNewHSV){
                setAllHsv(valuesHSV[0], valuesHSV[1], valuesHSV[2]);
                acceptNewHSV = false;
                println(F("TASK_HSV done"));
            }
            break;
        case TASK_DIMMER:
            setLeds(lastDimmerValue);
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

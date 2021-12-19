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
            setAll(0,0,0,0);
            break;
        case WHITE:
            setAll(0,0,0,255);
            break;
        case RED:
            setAll(255,0,0,0);
            break;
        case YELLOW:
            setAll(255,255,0,0);
			      break;
        case GREEN:
            setAll(0,255,0,0);
            break;
        case CYAN:
            setAll(0,255,255,0);
            break;
        case BLUE:
            setAll(0,0,255,0);
            break;
        case PURPLE:
            setAll(255,0,255,0);
            break;
        case ORANGE:
            setAll(255,81,0,0);
            break;

        case USER_COLOR_1:
            setAll(ucRed[0],ucGreen[0],ucBlue[0],ucWhite[0]);
            break;
        case USER_COLOR_2:
            setAll(ucRed[1],ucGreen[1],ucBlue[1],ucWhite[1]);
            break;
        case USER_COLOR_3:
            setAll(ucRed[2],ucGreen[2],ucBlue[2],ucWhite[2]);
            break;
        case USER_COLOR_4:
            setAll(ucRed[3],ucGreen[3],ucBlue[3],ucWhite[3]);
            break;
        case USER_COLOR_5:
            setAll(ucRed[4],ucGreen[4],ucBlue[4],ucWhite[4]);
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
                setAll(valuesRGBW[R], valuesRGBW[G], valuesRGBW[B], valuesRGBW[W]);
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
                setAll(valuesRGBW[R], valuesRGBW[G], valuesRGBW[B], valuesRGBW[W]);
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
            setAll(0, 0, 0, lastDimmerValue);
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

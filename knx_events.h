void dimmSwitchCallback(GroupObject &go)
{
    println(F("dimmSwitchCallback"));
    lastTask = currentTask;
    bool tmpBool = false;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    tmpBool = (bool)go.value();
    taskSoftOnOff(tmpBool);
    println(F("taskSoftOnOff: %d"), tmpBool);
    // Switching on still turns into white
    if (tmpBool) {
        setLeds(255);
        sendSceneNumber = WHITE;
    }
}

void dimmRelCallback(GroupObject &go)
{
    println(F("dimmRelCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newValue = (byte)go.value();
    taskDimUpDownStop(newValue);
    println(F("taskDimUpDownStop: %d"), newValue);
}
      
void dimmAbsCallback(GroupObject &go)
{
    println(F("dimmAbsCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newValue = (byte)go.value();
    taskNewValue(newValue);
    println(F("taskNewValue: %d"), newValue);
}
      
void sceneCallback(GroupObject &go)
{
    println(F("sceneCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newTask = go.value();
    println(F("newTask: 0x%02X"), newTask);
    if (newTask != 0xFF) {
        currentTask = newTask;
        acceptNewRGBW = true;
        initialized = false;
    }
    sendSceneNumber = newTask;
}

static byte colorValChange(GroupObject &go) {
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!

    rgbwChanged = true;
    rgbwChangedMillis = millis();
    currentTask = TASK_RGBW;

    return (byte)go.value();
}

void redValCallback(GroupObject &go)
{
    println(F("redValCallback"));
    newRGBW[R] = colorValChange(go);
    println(F("new R: %d / 0x%02x"), newRGBW[R], newRGBW[R]);
}

void greenValCallback(GroupObject &go)
{
    println(F("greenValCallback"));
    newRGBW[G] = colorValChange(go);
    println(F("new G: %d / 0x%02x"), newRGBW[G], newRGBW[G]);
}

void blueValCallback(GroupObject &go)
{
    println(F("blueValCallback"));
    newRGBW[B] = colorValChange(go);
    println(F("new B: %d / 0x%02x"), newRGBW[B], newRGBW[B]);
}

void whiteValCallback(GroupObject &go)
{
    println(F("whiteValCallback"));
    newRGBW[W] = colorValChange(go);
    println(F("new W: %d / 0x%02x"), newRGBW[W], newRGBW[W]);
}

void rgbwCallback(GroupObject &go) // RGBW 251.600
{
    println(F("rgbwCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    acceptNewRGBW = true;
    uint32_t newValue = (uint32_t)go.value();
    valuesRGBW[R] = newValue >> 24;
    valuesRGBW[G] = newValue >> 16;
    valuesRGBW[B] = newValue >> 8;
    valuesRGBW[W] = newValue;
    println(F("valuesRGBW R: %d, G: %d, B: %d, W: %d \n"),valuesRGBW[R],valuesRGBW[G],valuesRGBW[B],valuesRGBW[W]);
    currentTask = TASK_RGBW;
    sendSceneNumber = TASK_RGBW;
}

void msgCallback(GroupObject &go)
{
    byte msgNum = (go.asap() - 18) / 4;
    byte msgFunc = (go.asap() - 18) % 4;
    println(F("msgCallback %d/%d"), msgNum, msgFunc);
    switch (msgFunc) {
        case 0:  // Switch
            msg[msgNum].newValue = (bool)go.value() ? 255 : 0;
            statusM |= (1<<msgNum);
            break;
        case 1: // Percentage
            msg[msgNum].newValue = (byte)go.value();
            statusM |= (1<<msgNum);
            break;
        case 2: // RGB
        {
            uint32_t newRGB = (uint32_t)go.value();
            msg[msgNum].ledColor[R] = newRGB >> 16;
            msg[msgNum].ledColor[G] = newRGB >> 8;
            msg[msgNum].ledColor[B] = newRGB;
            msg[msgNum].ledColor[W] = 0;
            msg[msgNum].newValue = newRGB ? 255 : 0;
            statusM |= (1<<msgNum);
        }
            break;
        case 3: // RGBW
        {
            uint32_t newRGBW = (uint32_t)go.value();
            msg[msgNum].ledColor[R] = newRGBW >> 24;
            msg[msgNum].ledColor[G] = newRGBW >> 16;
            msg[msgNum].ledColor[B] = newRGBW >> 8;
            msg[msgNum].ledColor[W] = newRGBW;
            msg[msgNum].newValue = newRGBW ? 255 : 0;
            statusM |= (1<<msgNum);
        }
            break;
    }
}

void dimmSwitchCallback(GroupObject &go)
{
    dbg_print(F("dimmSwitchCallback"));
    lastTask = currentTask;
    bool tmpBool = false;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    tmpBool = (bool)go.value();
    taskSoftOnOff(tmpBool);
    dbg_print(F("taskSoftOnOff: %d"), tmpBool);
    // Switching on still turns into white
    if (tmpBool) {
        currentTask = WHITE;
        sendSceneNumber = WHITE;
    }
}

void dimmRelCallback(GroupObject &go)
{
    dbg_print(F("dimmRelCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newValue = (byte)go.value();
    taskDimUpDownStop(newValue);
    dbg_print(F("taskDimUpDownStop: %d"), newValue);
}
      
void dimmAbsCallback(GroupObject &go)
{
    dbg_print(F("dimmAbsCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newValue = (byte)go.value();
    if (dimmer.getCurrentValue() ==0 || lastTask == ALL_OFF) {
        currentTask = WHITE;
        sendSceneNumber = WHITE;
    }
    taskNewValue(newValue);
    dbg_print(F("taskNewValue: %d"), newValue);
}
      
void sceneCallback(GroupObject &go)
{
    dbg_print(F("sceneCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newTask = go.value();
    dbg_print(F("newTask: 0x%02X"), newTask);
    if (newTask != 0xFF) {
        currentTask = newTask;
        acceptNewRGBW = true;
        initialized = false;
    }
    if (dimmer.getCurrentValue() == 0) {
        taskSoftOnOff(true);
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
    dbg_print(F("redValCallback"));
    newRGBW.c.r = colorValChange(go);
    dbg_print(F("new R: %d / 0x%02x"), newRGBW.c.r, newRGBW.c.r);
}

void greenValCallback(GroupObject &go)
{
    dbg_print(F("greenValCallback"));
    newRGBW.c.g = colorValChange(go);
    dbg_print(F("new G: %d / 0x%02x"), newRGBW.c.g, newRGBW.c.g);
}

void blueValCallback(GroupObject &go)
{
    dbg_print(F("blueValCallback"));
    newRGBW.c.b = colorValChange(go);
    dbg_print(F("new B: %d / 0x%02x"), newRGBW.c.b, newRGBW.c.b);
}

void whiteValCallback(GroupObject &go)
{
    dbg_print(F("whiteValCallback"));
    newRGBW.c.w = colorValChange(go);
    dbg_print(F("new W: %d / 0x%02x"), newRGBW.c.w, newRGBW.c.w);
}

void rgbwCallback(GroupObject &go) // RGBW 251.600
{
    dbg_print(F("rgbwCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    acceptNewRGBW = true;
    uint32_t newValue = (uint32_t)go.value();
    valuesRGBW.rgbw = newValue;
    dbg_print(F("valuesRGBW R: %d, G: %d, B: %d, W: %d \n"),valuesRGBW.c.r,valuesRGBW.c.g,valuesRGBW.c.b,valuesRGBW.c.w);
    currentTask = TASK_RGBW;
    sendSceneNumber = TASK_RGBW;
}

void msgCallback(GroupObject &go)
{
    byte msgNum = (go.asap() - 18) / 4;
    byte msgFunc = (go.asap() - 18) % 4;
    dbg_print(F("msgCallback %d/%d"), msgNum, msgFunc);
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
            msg[msgNum].ledColor.rgbw = newRGB;
            msg[msgNum].newValue = newRGB ? 255 : 0;
            statusM |= (1<<msgNum);
        }
            break;
        case 3: // RGBW
        {
            uint64_t newRGBW = (uint64_t)go.value();
            uint32_t mask = newRGBW >> 32;
            msg[msgNum].ledColor.rgbw &= ~mask;
            msg[msgNum].ledColor.rgbw |= newRGBW & mask;
            msg[msgNum].newValue = newRGBW ? 255 : 0;
            statusM |= (1<<msgNum);
        }
            break;
    }
    if (dimmer.getCurrentValue() == 0)
    {
        taskSoftOnOff(true);
    }
}

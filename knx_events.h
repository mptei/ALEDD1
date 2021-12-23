// Changes the task to the given newTask
void changeTask(byte newTask)
{
    lastTask = currentTask;
    currentTask = newTask;
    sendSceneNumber = newTask;
}

// Returns true if the dimmer state is off
boolean isDimmerOff()
{
    return dimmer.getCurrentValue() == 0;
}

void dimmSwitchCallback(GroupObject &go)
{
    dbg_print(F("dimmSwitchCallback"));
    bool tmpBool = false;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    tmpBool = (bool)go.value();
    taskSoftOnOff(tmpBool);
    dbg_print(F("taskSoftOnOff: %d"), tmpBool);
    // Switching on still turns into white
    if (tmpBool) {
        changeTask(WHITE);
    }
}

void dimmRelCallback(GroupObject &go)
{
    dbg_print(F("dimmRelCallback"));
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newValue = *go.valueRef();
    taskDimUpDownStop(newValue);
    dbg_print(F("taskDimUpDownStop: %d"), newValue);
}
      
void dimmAbsCallback(GroupObject &go)
{
    dbg_print(F("dimmAbsCallback"));
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newValue = (byte)go.value();
    if (isDimmerOff() || lastTask == ALL_OFF) {
        changeTask(WHITE);
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
        changeTask(newTask);
        acceptNewRGBW = true;
        initialized = false;
    }
    if (isDimmerOff()) {
        taskSoftOnOff(true);
    }
}


void colorChannelCallback(GroupObject &go)
{
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!

    rgbwChanged = true;
    rgbwChangedMillis = millis();
    changeTask(TASK_RGBW);

    byte idx = go.asap() - goRedVal.asap();  // 0 -> R .. 2 -> B, 3 -> W
    if (idx < 3)
    {
        idx = 2 - idx; // 2 -> R, 1 -> G, -> 0 -> B
    }
    uint32_t mask = 0xff << (idx*8);
    newRGBW.rgbw = (newRGBW.rgbw & ~mask) | (byte)go.value() << (idx*8);
    dbg_print(F("colorChannelCallback: channel %d set to 0x%02x"), idx, (byte)go.value());
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
            break;
        case 1: // Percentage
            msg[msgNum].newValue = (byte)go.value();
            break;
        case 2: // RGB
        {
            uint32_t newRGB = (uint32_t)go.value();
            msg[msgNum].ledColor.rgbw = newRGB;
            msg[msgNum].newValue = newRGB ? 255 : 0;
        }
            break;
        case 3: // RGBW
        {
            uint64_t newRGBW = (uint64_t)go.value();
            uint32_t mask = newRGBW >> 32;
            msg[msgNum].ledColor.rgbw &= ~mask;
            msg[msgNum].ledColor.rgbw |= newRGBW & mask;
            msg[msgNum].newValue = newRGBW ? 255 : 0;
        }
            break;
    }
    statusM |= (1<<msgNum);
    if (isDimmerOff())
    {
        // Switch stripe on when a message is set
        taskSoftOnOff(true);
    }
}

void dayNightCallback(GroupObject &go)
{
    dbg_print(F("dayNightCallback"));
    setDayNightValues(!(onMeansDay ^ (bool)go.value()));
    setBrightness(dimmer.getCurrentValue());
}

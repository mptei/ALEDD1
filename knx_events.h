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

void needPower()
{
    if (!powerSupplyReady)
    {
        dbg_print(F("turn Power on"));
        powerSupplyTurnOn = true;
    }
}


void dimmSwitchCallback(GroupObject &go)
{
    dbg_print(F("dimmSwitchCallback"));
    bool tmpBool = false;
    tmpBool = (bool)go.value();
    taskSoftOnOff(tmpBool);
    dbg_print(F("taskSoftOnOff: %d"), tmpBool);
    // Switching on still turns into white
    if (tmpBool) {
        needPower();
        changeTask(WHITE);
    }
}

void dimmRelCallback(GroupObject &go)
{
    dbg_print(F("dimmRelCallback"));
    needPower();
    taskDimUpDownStop(go.value(), go.value(Dpt(3, 7, 1)));
}
      
void dimmAbsCallback(GroupObject &go)
{
    dbg_print(F("dimmAbsCallback"));
    byte newValue = (byte)go.value();
    if (isDimmerOff() || lastTask == ALL_OFF) {
        changeTask(WHITE);
    }
    taskNewValue(newValue);
    if (newValue)
    {
        needPower();
    }
    dbg_print(F("taskNewValue: %d"), newValue);
}
      
void sceneCallback(GroupObject &go)
{
    dbg_print(F("sceneCallback"));
    lastTask = currentTask;
    byte newTask = go.value();
    dbg_print(F("newTask: 0x%02X"), newTask);
    if (newTask != 0xFF) {
        changeTask(newTask);
        acceptNewRGBW = true;
        initialized = false;
    }
    if (isDimmerOff()) {
        needPower();
        taskSoftOnOff(true);
    }
}


void colorChannelCallback(GroupObject &go)
{
    needPower();

    rgbwChanged = true;
    rgbwChangedMillis = millis();
    changeTask(TASK_RGBW);

    byte idx = go.asap() - go_Red_Value.asap();  // 0 -> R .. 2 -> B, 3 -> W
    if (idx < 3)
    {
        idx = 2 - idx; // 2 -> R, 1 -> G, -> 0 -> B
    }
    uint32_t mask = 0xff << (idx*8);
    newRGBW.rgbw = (newRGBW.rgbw & ~mask) | (byte)go.value() << (idx*8);
    dbg_print(F("colorChannelCallback: channel %d set to 0x%02x"), idx, (byte)go.value());
}

void rgbCallback(GroupObject &go) // RGB
{
    dbg_print(F("rgbCallback"));
    lastTask = currentTask;
    needPower();
    uint32_t newRGB = (uint32_t)go.value();
    acceptNewRGBW = (newRGB != (valuesRGBW.rgbw & 0xffffff));
    valuesRGBW.rgbw = newRGB | (valuesRGBW.rgbw & 0xff000000); // keep white channel
    dbg_print(F("valuesRGB: 0x%08x \n"),valuesRGBW.rgbw & 0xffffff);
    currentTask = TASK_RGB;
    sendSceneNumber = TASK_RGB;
}

static bool applyRGBW(color_t &in, GroupObject &go)
{
    uint64_t newValue = (uint64_t)go.value();
    uint32_t mask = newValue >> 32;
    dbg_print(F("applyRGBW value: 0x%08x, mask: 0x%08x\n"),(uint32_t)newValue, (uint32_t)(newValue >> 32));
    color_t out;
    out.rgbw = in.rgbw & ~mask;
    out.rgbw |= newValue & mask;
    bool changed = in.rgbw != out.rgbw;
    in = out;
    return changed;
}

void rgbwCallback(GroupObject &go) // RGBW 251.600
{
    dbg_print(F("rgbwCallback"));
    lastTask = currentTask;
    needPower();
    acceptNewRGBW = applyRGBW(valuesRGBW, go);
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
            msg[msgNum].newValue = applyRGBW(msg[msgNum].ledColor, go) ? 255 : 0;
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

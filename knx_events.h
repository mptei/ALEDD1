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
        changeTask(dimmScene);
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
    byte newValue = (byte)go.value();
    dbg_print(F("dimmAbsCallback: %d"), newValue);
    if (isDimmerOff() || lastTask == ALL_OFF) {
        changeTask(dimmScene);
    }
    taskNewValue(newValue);
    if (newValue)
    {
        needPower();
    }
    dbg_print(F("taskNewValue: %d"), newValue);
}

void dimmSceneCallback(GroupObject &go)
{
    dimmScene = go.value();
    dbg_print(F("dimmSceneCallback: %d"), dimmScene);
}

void sceneCallback(GroupObject &go)
{
    dbg_print(F("sceneCallback"));
    lastTask = currentTask;
    byte newTask = go.value();
    dbg_print(F("newTask: 0x%02X"), newTask);
    if (newTask != 0xFF) {
        changeTask(newTask);
        initialized = false;
    }
    if (isDimmerOff()) {
        needPower();
        taskSoftOnOff(true);
    }
}


// Called on any single channel RGBW change
// The change is delayed to perform the change in one go
void colorChannelCallback(GroupObject &go)
{

    uint8_t idx = go.asap() - NUM_Red_Value;  // 0 -> R .. 2 -> B, 3 -> W
    if (idx < 3)
    {
        idx = 2 - idx; // 2 -> R, 1 -> G, -> 0 -> B
    }
    const uint8_t newValue = (uint8_t)go.value();
    if (newRGBW.a[idx] != newValue)
    {
        // Value has changed
        if (newValue != 0)
        {
            needPower();
        }
        newRGBW.a[idx] = newValue;
        rgbwSingleChannelChanged = true;
        rgbwChangedMillis = millis();
        changeTask(TASK_RGBW);
    }
    dbg_print(F("colorChannelCallback: channel %d set to 0x%02x"), idx, newValue);
}

void rgbCallback(GroupObject &go) // RGB
{
    dbg_print(F("rgbCallback"));
    uint32_t newRGB = (uint32_t)go.value();
    acceptNewRGBW = (newRGB != (valuesRGBW.rgbw & 0x00ffffff));
    if (acceptNewRGBW)
    {
        if (newRGB != 0 && !isDimmerOff())
        {
            needPower();
        }
        changeTask(TASK_RGBW);
        valuesRGBW.rgbw = newRGB | (valuesRGBW.rgbw & 0xff000000); // keep white channel
    }
    dbg_print(F("valuesRGB: 0x%08x \n"),valuesRGBW.rgbw);
}

// Handle the masking of the RGBW channels
static color_t applyRGBW(const color_t &in, GroupObject &go)
{
    uint64_t newValue = (uint64_t)go.value();
    uint32_t mask = newValue >> 32;
    dbg_print(F("applyRGBW value: 0x%08lx, mask: 0x%08lx\n"),(uint32_t)newValue, (uint32_t)(newValue >> 32));
    color_t out;
    out.rgbw = in.rgbw & ~mask;
    out.rgbw |= newValue & mask;
    return out;
}

void rgbwCallback(GroupObject &go) // RGBW 251.600
{
    dbg_print(F("rgbwCallback"));
    color_t newRGBW = applyRGBW(valuesRGBW, go);
    acceptNewRGBW = (newRGBW.rgbw != valuesRGBW.rgbw);
    if (acceptNewRGBW)
    {
        if (newRGBW.rgbw != 0 && !isDimmerOff())
        {
            needPower();
        }
        valuesRGBW = newRGBW;
        changeTask(TASK_RGBW);
    }
    dbg_print(F("valuesRGBW: 0x%08lx\n"),valuesRGBW.rgbw);
}

void msgCallback(GroupObject &go)
{
    byte msgNum = (go.asap() - NUM_Message_1_Switch);
    byte msgFunc = msgNum % 4;
    msgNum /= 4;
    bool powerOn = false;
    switch (msgFunc) {
        case 0:  // Switch
            dbg_print(F("msgCallback msg%d: switch"), msgNum+1);
            msg[msgNum].newValue = (bool)go.value() ? 255 : 0;
            powerOn = msg[msgNum].newValue != 0;
            break;
        case 1: // Percentage
            dbg_print(F("msgCallback msg%d: percentage"), msgNum+1);
            msg[msgNum].newValue = (byte)go.value();
            powerOn = (msg[msgNum].newValue != 0);
            break;
        case 2: // RGB
            {
                dbg_print(F("msgCallback msg%d: RGB"), msgNum+1);
                uint32_t newRGB = (uint32_t)go.value();
                if (newRGB != msg[msgNum].ledColor.rgbw)
                {
                    powerOn = (newRGB != 0 && msg[msgNum].lastValue != 0);
                    msg[msgNum].ledColor.rgbw = newRGB;
                }
            }
            break;
        case 3: // RGBW
            {
                dbg_print(F("msgCallback msg%d: RGBW"), msgNum+1);
                color_t newRGBW = applyRGBW(msg[msgNum].ledColor, go);
                if (newRGBW != msg[msgNum].ledColor)
                {
                    powerOn = (newRGBW.rgbw != 0 && msg[msgNum].lastValue != 0);
                    msg[msgNum].ledColor = newRGBW;
                }
            }
            break;
    }
    statusM |= (1<<msgNum);
    if (powerOn && isDimmerOff())
    {
        // Switch stripe on when a message is set
        needPower();
        taskSoftOnOff(true);
    }
}

void dayNightCallback(GroupObject &go)
{
    dbg_print(F("dayNightCallback"));
    setDayNightValues(!(onMeansDay ^ (bool)go.value()));
    setBrightness(dimmer.getCurrentValue());
}

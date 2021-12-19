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
        setAll(0,0,0,255);
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

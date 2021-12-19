void dimmSwitchCallback(GroupObject &go)
{
    println(F("dimmSwitchCallback"));
    lastTask = currentTask;
    bool tmpBool = false;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    tmpBool = (bool)go.value();
    taskSoftOnOff(tmpBool);
    println(F("taskSoftOnOff: %d"), tmpBool);
    sendSceneNumber = TASK_DIMMER;
}

void dimmRelCallback(GroupObject &go)
{
    println(F("dimmRelCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newValue = (byte)go.value();
    taskDimUpDownStop(newValue);
    println(F("taskDimUpDownStop: %d"), newValue);
    sendSceneNumber = TASK_DIMMER;
}
      
void dimmAbsCallback(GroupObject &go)
{
    println(F("dimmAbsCallback"));
    lastTask = currentTask;
    powerSupplyTurnOn = true; //dirty solution: if PS is off and LEDs are off and next command is "turn all off" PS will go on... and after timeout off. Is this a real use case?!
    byte newValue = (byte)go.value();
    taskNewValue(newValue);
    println(F("taskNewValue: %d"), newValue);
    sendSceneNumber = TASK_DIMMER;
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

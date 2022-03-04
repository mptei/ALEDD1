# KNX to RGB(W) Stripe

Based on [Konnekting ALEDD1](https://github.com/KONNEKTING/ALEDD1)

## Behavior

### Dimm Part

The dimm part controls the state and the brightness of the LEDs. Any brightness 
The main function of the dimm part is to control the brightness of the LEDs.

### Day/Night

An day/night object with a choosable polarity can be used to define specific brightness ranges. The dimming value is then transformed into the given range. That means it's not possible to leave the range. Switching between day and night changes the transformation instantly. 

### Color

The color may be controlled via several objects. There are single objects for every color channel and the white channel.

Also there is an RGB and an RGBW object.

HSV is currently untested!
### Messages

Messages are values which are displayed via ranges of LEDs. They are supported via a boolean group object, which allows to switch the message LEDs on and off, or via percent object which determines how many of the message LEDs are lit.

The message LEDs are determined by a start and end number. The LED with the start number determines the lowest LED and the LED with the end number the highest LED. That means the end number doesn't need to be greater than the start number. Instead the LEDs will lit beginning from the start number going to end number the higher the percentage value goes.

The messages are ordered. If messages share LEDs the higher message number has precedence and will overwrite lower messages.

Messages are presented on top of a static color or animation.

If any message is set active the stripe is switched on. If all message are set inactive and the stripe was off before, the stripe goes off.

When a message switched the stripe on and the stripe is switched off, the stripe goes off.

### Scenes

There are several predefined scenes which can be activated via a scene object.

### Static color Scenes 1 - 10

1 - black; all LEDs off  
2 - white; all LEDs on  
3 - red  
4 - yellow  
5 - green  
6 - cyan  
7 - blue  
8 - purple  
9 - orange  

### Static user color scenes 21 - 25

These scenes uses the user defined colors from the configuration.

### Animation scenes 41 - 49

41 - rainbow  
42 - single RGB  
43 - sparkler  
44 - twinky stars  
45 - chaser  
46 - hue fader  
47 - speed trails  
48 - bouncy balls  
49 - two brush color mixing  


## Build

Clone repository https://github.com/mptei/ALEDD1.git

Switch to branch thelsing: git checkout thelsing

Connect ALEDD1 hardware via USB.

Build with platformio (in vscode): pio run -t upload -e arduino_ALEDD1

Download CreateKnxProd Release_24: https://github.com/thelsing/CreateKnxProd/releases/download/Release_24/CreateKnxProd.zip

Unzip and run CreateKnxProd.exe

(Perhaps you need to install Dotnet6 before; you should probably not have ETS6 installed, but you need ETS5)

Load ALEDD1.xml from ALEDD1 project and create knxprod

Import knxprod into ETS; insert device and configure, programm and use it.

Attention: By default ALEDD1 goes into programming mode after download firmware.
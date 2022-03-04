# KNX to RGB(W) Stripe

Based on [Konnekting ALEDD1](https://github.com/KONNEKTING/ALEDD1)

## Behavior

### Day/Night

It is possible to define a minimum and a maximum brightness for day and night. Switching between day and night is done via group object.

The brightness value from bus is converted into a brightness value in the given range. The brightness status otherwise is scaled back to the full range.

When switching between day and night the brightness is adjusted to the valid range.

### Messages

Messages are values which are displayed via ranges of LEDs. They are supported via a boolean group object, which allows to switch the message LEDs on and off, or via percent object which determines how many of the message LEDs are lit.

The message LEDs are determined by a start and end number. The LED with the start number determines the lowest LED and the LED with the end number the highest LED. That means the end number doesn't need to be greater than the start number. Instead the LEDs will lit beginning from the start number going to end number the higher the percentage value goes.

The messages are ordered. If messages share LEDs the higher message number has precedence and will overwrite lower messages.

Messages are presented on top of a static color or animation.

If any message is set active the stripe is switched on. If all message are set inactive and the stripe was off before, the stripe goes off.

When a message switched the stripe on and the stripe is switched off, the stripe goes off.
For my extension, first, I added two buttons in hardware connected to pins 2 and 3. The button is set up so that looking at the
prongs, the two next to each other coming out of the same side has one connected to ground and the other to 3.3V. Then, on a pin
on the other side, the button is connected to the raspberry pi set as an input pin. When depressed, the raspberry pi is will
be connected to 3.3 V and will thus read 1. When pressed, this switches to being connected to ground, so the raspberry pi will
read 0. No circuit is actually created through the button. The button connected to pin 2 will change the minutes on the clock,
and the button connected to pin 3 will change the hours on the clock. 

The rest of my extension turned to creating a working digital clock. This is similar to the timer, except it allows the user 
click the buttons to set the time both in hours and minutes. The dot point is turned on between the hours and minutes to
indicate the difference. The timer is set to change with each minute, and once the clock reaches 12 hours, it will roll back
over to 1. The buttons were debounced so that when they are quickly pressed, the user will not accidentally move up by too many
digits. Further, though, this allows the user to press and hold the button to set the time without having to click the buttons
over and over if moving up to a larger number. The speed is managable, but not too slow. This delay is kept internal to the 
buttons, so the time can still change and the LEDs are still lit up during this process.

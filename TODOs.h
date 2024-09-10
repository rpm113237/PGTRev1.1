//R. P. McClellan 27Jun2023; latest 9/9/23; latest 29Jun24
/*

NOTE!!!: 0.33(+/- 0.05)* TGT, not to far off from 0.33*TGT * .90 to 1.00

SWSTART is GPIO1--check for OK--Seems OK; has to be set pinmode = input for some reason.

TODO--currently, the PGT Starts when the charger is plugged in & battery discharged;(Why??) check reason for reset--if not wake, don't start.
TODO (Hardware--what happens when battery discharges?  Does current go low enough? Disable linear reg on battery below something??)

NOTE!!!  OLD Squeezer19 used 0.33 as tgt pcnt with hi/low at +-.05 of that!!!
initial impression is that is too tight.  Currently, 20 percent of tgt
TODO--use start switch to bypass wait for connect ???

Apparently, NIMBLE (with UART defintion?) sends whole 20? bytes

TODO--add in ADC read of batsns; new cmd E: (Done)

TODO--why in the hell does the Soundwakeup sound twice on first call?  

TODO--Consider making LED blinking a PWM?? Slowest PWM 1Hz.

Start Switch:  In this implementation, the start switch wakes up from deep sleep.  Optionally, goes into deep sleep on long hold?  Pause, restart, etc. goes away. need only long click for operator shutdown?  Send
notification to app of operator shutdown??  
TODO  Since it is entirely app driven, do we even need a power off function on the PGT?

Rev1 CPU has two fixed LED's--a blue and a red.  The blue is the connect led; the red is intended as a low batt indicator. 
  Battery good--red off.
  battery good and not connected--blue blinks, once per second
  battery good and connected--solid blue
  battery low and not connected--red blinks, blue off
  battery low and connected-- red solid, blue off.
TODO Question:  Since this system is entirely app driven--are these necessary?  

TODO Shut down--if no force above background dither of NN minutes, go to sleep.  What if App has disconnected; device
runs until batter runs down?

Scale:  Currently hard wired to 10 sps; 

TODO check ADC accuracy.

TODO:  !!!! on going into deep sleep, invoke scale .powerdown() see 
void HX711::power_down()
{
  //  at least 60 us HIGH
  digitalWrite(_clockPin, HIGH);
  delayMicroseconds(64);
}

Thge LED blinking code is messy.  LedSelect no longer relevant--only one LED.  Combine LEDSelect into LEDBlink.  Messey

***The BLE initial connect and the in loop reconnect seem redundant and awkward.

















*/
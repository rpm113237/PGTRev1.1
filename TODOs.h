//R. P. McClellan 27Jun2023; latest 9/9/23; latest 29Jun24
/*

N

Apparently, NIMBLE (with UART defintion?) sends whole 20? bytes

TODO--why in the hell does the Soundwakeup sound twice on first call?  



Start Switch:  In this implementation, the start switch wakes up from deep sleep.  Optionally, goes into deep sleep on long hold?  Pause, restart, etc. goes away. need only long click for operator shutdown?  Send
notification to app of operator shutdown??  
TODO  Since it is entirely app driven, do we even need a power off function on the PGT?

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

!!!! look at aclibrated ADC reads, also cal routine.

TODO???? Run cal routines off the serial monitor at setup?

Figure out why TxString needs cleared--or at least integrate it into bletx

















*/
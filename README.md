# easy-gpio
This code work on raspberry to used simply the gpio

Tested on Raspberry pi3 model B+

To install:
$ sudo chmod +x install.sh
$ ./install.sh

Now you can use the free software!

*****************************************Example*****************************************
modify GPIO
  if you want turn on the GPIO26:
    $ easy-gpio 26 1
    >No output
  if you want switch the value of GPIO26:
    $ easy-gpio 26 -t
    >Pin 26 now is LOW
  if you want read the value of GPIO26:
    $ easy-gpio 26
    >Value of pin is: 0
    >The direction is: OUT
    
SIMULATE PWM    
  if you want simulate a pwm
    $ easy-gpio pwm 0.5 26                   Where:   0.5 is the frequency. Range [0,1], step to 0.001
                                                      26  is the gpion
    >Frequency: 0.50 pin: 12 pid: 20340
    >                                         note: the shell is lock.
    To exit press ctrl + C
  if you want simulate a pwm but yuo don't want the lock to the shell:
    $ easy-gpio pwm 0.5 26 &
    >Frequency: 0.50 pin: 12 pid: 20350
  Now to turn-off the pwm:
    $ kill -9 20350                           Where the "20350" is a PID to the process PWM. 
  

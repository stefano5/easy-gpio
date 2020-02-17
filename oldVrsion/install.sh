#/bin/bash.sh

gcc ./src/easy-gpio.c -o easy-gpio 
ls -l easy-gpio
sudo cp easy-gpio /bin/

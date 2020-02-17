sudo cp -r lib/* /usr/include/

gcc easy-gpio.c -o easy-gpio
gcc daemon-easy-gpio.c -lpthread -o daemon-easy-gpio
sudo cp easy-gpio /usr/bin/
sudo cp daemon-easy-gpio /usr/bin/

sudo cp service/daemon-easy-gpio.service   /etc/systemd/system/daemon-easy-gpio.service

sudo systemctl enable daemon-easy-gpio.service
sudo systemctl start daemon-easy-gpio.service

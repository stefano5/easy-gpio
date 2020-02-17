echo "copy lib"
sudo cp -r lib/* /usr/include/

echo "remove old file"
sudo systemctl stop daemon-easy-gpio.service > /dev/null
sudo rm /usr/bin/easy-gpio /usr/bin/daemon-easy-gpio /usr/bin/daemon-easy-gpio.sh > /dev/null
sudo rm /etc/systemd/system/daemon-easy-gpio.service > /dev/null

echo "build ..."
gcc easy-gpio.c -o easy-gpio
gcc daemon-easy-gpio.c -lpthread -o daemon-easy-gpio
sudo cp easy-gpio /usr/bin/
sudo cp daemon-easy-gpio /usr/bin/

echo "prepare auto exe service"
sudo chmod +x service/daemon-easy-gpio.sh
sudo cp service/daemon-easy-gpio.sh /usr/bin/daemon-easy-gpio.sh
sudo cp service/daemon-easy-gpio.service /etc/systemd/system/daemon-easy-gpio.service

echo "enable & start service"
sudo systemctl enable daemon-easy-gpio.service
sudo systemctl start daemon-easy-gpio.service

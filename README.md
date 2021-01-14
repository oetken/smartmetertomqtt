# Details

This tool reads data from Sml (Smart Message Language) devices or M-Bus devices and publishes the data via MQTT.
Sml support is currently limited to reading.
MQTT is limited to 

Tested with:
* iskra MT681 (electricity)
* elster BK-G4 AT with M-Bus ACM Modul (gas)

Tested on:
* Debian Bullseye
* Raspberry Pi OS (Debian 10.7 based)

Depends on:
* https://github.com/dailab/libsml
* https://github.com/rscada/libmbus
* https://github.com/qt/qtmqtt

# Prerequisits

Libmbus Libsml and QtMQTT:

cd WHATEVERWORKSPACEYOUWANT
sudo apt install -y git cmake build-essential qt5-default qtbase5-dev cmake devscripts qtbase5-private-dev debhelper uuid-dev libqt5xmlpatterns5-dev
git clone https://github.com/rscada/libmbus
git clone https://github.com/dailab/libsml
git clone https://github.com/qt/qtmqtt

cd libsml 
sed -i 's/.*\s-C\stest$//g' Makefile
dpkg-buildpackage -b --no-sign
cd ..

cd libmbus
./build-deb.sh
cd ..

dpkg -i *.deb

cd qtmqtt 
git checkout v5.11.3
qmake
make
sudo make install

# Installation

git clone https://gitlab.com/smart-home-tools/smartmetertomqtt.git
cd smartmetertomqtt
mkdir build
cd build
cmake ..
make

# Configuration

* when run as user the config is stored in ~/.config/SmartHomeTools/SmartMeterToMqtt.conf
* when run as service the config is stored in /etc/smartmeter.ini

-> see etc/SmartMeterToMqtt.conf_example

# Installation as service

in smartmetertomqtt:
sudo make install
sudo systemctrl install smartmeter.service
sudo systemctrl enable smartmeter.service
sudo systemctrl start smartmeter.service

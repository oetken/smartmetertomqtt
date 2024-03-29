# Details

This tool reads data from Sml (Smart Message Language) devices or M-Bus devices and publishes the data via MQTT.

Sml support is currently limited to reading.

MQTT is limited to non-SSL (you may easily adopt this...)

Tested with:
* iskra MT681 (electricity)
* elster BK-G4 AT with M-Bus ACM Modul (gas)
* Weidman IR Schreib/Lesekopf USB https://shop.weidmann-elektronik.de/index.php?page=product&info=24
* No-name USB M-Bus Master device

Tested on:
* Debian Bullseye
* Raspberry Pi OS (Debian 10.7 based)
* Raspberry Pi Zero W / Pi 3b

Goals:
* simple and very few effort!

Depends on:
* https://github.com/volkszaehler/libsml
* https://github.com/rscada/libmbus
* https://github.com/qt/qtmqtt

# Installation

Check [Releases](https://github.com/oetken/smartmetertomqtt/releases) for a link to amd64 Debian builds.
Or build from source:

## Prerequisits

Libmbus Libsml and QtMQTT:
```
cd WHATEVERWORKSPACEYOUWANT
sudo apt install -y git cmake build-essential qtbase5-dev cmake devscripts qtbase5-private-dev debhelper uuid-dev libqt5xmlpatterns5-dev libqt5serialport5-dev dh-make dh-exec
git clone https://github.com/rscada/libmbus
git clone https://github.com/volkszaehler/libsml
git clone https://github.com/qt/qtmqtt

cd libsml 
sed -i 's/.*\s-C\stest$//g' Makefile
dpkg-buildpackage -b --no-sign
cd ..

cd libmbus
./build-deb.sh
cd ..

cd qtmqtt;
QT_VERSION="$(qmake --version | sed -n  's/.*version\s*\([0-9]*\.[0-9]*\.[0-9]*\)\s*.*/\1/p')";
git tag -l | grep -E "${QT_VERSION}$" || QT_VERSION=$(git tag -l | grep -oP "$(qmake --version | sed -n  's/.*version\s*\([0-9]*\.[0-9]*\.\)[0-9]*\s*.*/\1/p')[0-9]+$" | tail -n 1);
git checkout v$QT_VERSION;
cd .. && mv qtmqtt "qtmqtt-$QT_VERSION"; cd "qtmqtt-$QT_VERSION";
qmake;
dh_make -s -c gpl -e none@none.de --createorig -y
dpkg-buildpackage -b --no-sign
cd ..

sudo dpkg -i *.deb

# alternative for mqtt:
#git checkout v$(qmake --version | sed -n  's/.*version\s*\([0-9]*\.[0-9]*\.[0-9]*\)\s*.*/\1/p')
#if no suitable version is found check tags and pick a good one
#qmake
#make
#sudo make install
```


## Compile

```
git clone https://github.com/oetken/smartmetertomqtt.git
cd smartmetertomqtt
mkdir build
cd build
cmake ..
# for Release mode use: cmake .. -DCMAKE_BUILD_TYPE=Release
cpack
sudo dpkg -i *.deb
```

# Configuration

* when run as user without argument the config is located in ~/.config/SmartHomeTools/SmartMeterToMqtt.json
* when run as service the config is located in /etc/smartmeter.json
* you can pass the config file location via "-f"
* There are several configuration options like mean filtering or skipping of values. Check the example config.

-> see [SmartMeterToMqtt.json_example](etc/SmartMeterToMqtt.json_example)

# Installation as service

Done by the debian package.


Copyright 2021 - 2021, Andreas Oetken and the smartmetertomqtt contributors.

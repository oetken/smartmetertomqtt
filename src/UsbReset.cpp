/*  Copyright 2023 - 2023, Fabian Hassel and the smartmetertomqtt contributors.

    This file is part of SmartMeterToMqtt.

    SmartMeterToMqtt is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SmartMeterToMqtt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SmartMeterToMqtt.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QDebug>
#include "UsbReset.hpp"

#ifdef Q_OS_LINUX
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/usbdevice_fs.h>
#include <libudev.h>
#endif

UsbReset::UsbReset(void) : device_(QString()){
    
}

UsbReset::UsbReset(const QString device) : device_(device) {

}

void UsbReset::doReset(const QString device) {
    bool success = false;

    #ifdef Q_OS_LINUX
        if (geteuid() != 0){
            qCritical() << "USB-Reset requires root privileges!";
        }else{
            QString rawDevice = resolveDevice(device);

            if (!rawDevice.isEmpty()){
                int fd = open(device.toStdString().c_str(), O_WRONLY);
                if (fd >= 0)
                {
                    qDebug() << "Try to execute USBDEVFS_RESET on" << rawDevice;
                    int rc = ioctl(fd, USBDEVFS_RESET, 0);
                    if (rc == 0){
                        qInfo() << "Sucessfully reseted USB device" << device << "via" << rawDevice;
                        success = true;
                    }
                    close(fd);
                }
                if (!success){
                    qCritical() << "Unable to reset USB device" << device;
                }
            }
        }
    #else
        qCritical() << "USB-Reset is only supportet on Linux!";
    #endif

    if (success){
        emit resetSuccess(device);
    }else{
        emit resetFailed(device);
    }

    emit resetDone(success, device);
}

QString UsbReset::resolveDevice(const QString device) const{
    #if defined(Q_OS_LINUX)

    int bus;
    int address;
    const char *path;
    const char *sysattr;
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    qDebug() << "Trying to resolve symlink of" << device;
    QString ttyDevice = realpath(device.toStdString().c_str(), NULL);
    if (ttyDevice != device){
        qInfo() << "Symlink resolved:" << device << "is originally located at" << ttyDevice;
    }

    qDebug() << "Trying to resolve raw device of" << device;

    bus = -1;
    address = -1;

    udev = udev_new();
    if (udev){
        enumerate = udev_enumerate_new(udev);
        udev_enumerate_add_match_subsystem(enumerate, "tty");
        udev_enumerate_scan_devices(enumerate);

        devices = udev_enumerate_get_list_entry(enumerate);
        udev_list_entry_foreach(dev_list_entry, devices) {
            path = udev_list_entry_get_name(dev_list_entry);
            dev = udev_device_new_from_syspath(udev, path);

            if(strcmp(udev_device_get_devnode(dev), device.toStdString().c_str()) == 0)
            {
                dev = udev_device_get_parent_with_subsystem_devtype(
                        dev,
                        "usb",
                        "usb_device");
                sysattr = udev_device_get_sysattr_value(dev, "busnum");
                if(sysattr != NULL)
                {
                    bus = strtol(sysattr, NULL, 10);
                }
                sysattr = udev_device_get_sysattr_value(dev, "devnum");
                if(sysattr != NULL)
                {
                    address = strtol(sysattr, NULL, 10);
                }
                udev_device_unref(dev);
                break;
            }
            udev_device_unref(dev);
        }
        udev_enumerate_unref(enumerate);
        udev_unref(udev);
    }else{
        qCritical() << "UDEV ERROR";
    }

    QString rawDevice = QString();
    if (bus > 0 && address > 0){
        rawDevice = QString("/dev/bus/usb/%1/%2").arg(bus,3,10,QChar('0')).arg(address,3,10,QChar('0'));
        qDebug() << "Found:" << device << "has raw device at" << rawDevice;
    }else{
        qCritical() << "Unable to resolve raw device of" << device;
    }
    #endif

    return rawDevice;
}
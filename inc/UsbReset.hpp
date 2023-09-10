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
#ifndef SMARTMETERTOMQTT_USBRESET_HPP
#define SMARTMETERTOMQTT_USBRESET_HPP

#include <QObject>
#include <QThread>

class UsbReset : public QThread{
    Q_OBJECT
public:
    explicit UsbReset(void);
    explicit UsbReset(const QString device);

public slots:
    void doReset() { doReset(this->device_); };
    void doReset(const QString device);

protected:
    QString resolveDevice(const QString device) const;

signals:
    void resetSuccess(QString device);
    void resetFailed(QString device);
    void resetDone(bool success, QString device);

private:
    QString device_;
};


#endif //SMARTMETERTOMQTT_USBRESET_HPP

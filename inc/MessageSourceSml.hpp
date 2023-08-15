/*  Copyright 2021 - 2021, Andreas Oetken and the smartmetertomqtt contributors.

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
#ifndef MESSAGESOURCESML_H
#define MESSAGESOURCESML_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include "IMessageSource.hpp"

class MessageSourceSml : public IMessageSource
{
    Q_OBJECT
public:
    MessageSourceSml(QString topicBase, QString device, uint32_t baudrate);
    bool setup();
    void test();
private:
    bool connectUart();
    void disconnectUart();

private slots:
    void handleReadReady();
    void handleWatchdog();

private:
    QByteArray readData_;
    QSerialPort serialPort_;
    QString topicBase_;
    QString device_;
    uint32_t baudrate_;
    bool connected_;
    const char startPattern_[8] = {0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01};
    const char endPattern_[5] = {0x1b, 0x1b, 0x1b, 0x1b, 0x1a};
    const int dataWatchdogTimeMs_ = 10 * 1000;
    QTimer dataWatchdog_;
};

#endif // MESSAGESOURCESML_H

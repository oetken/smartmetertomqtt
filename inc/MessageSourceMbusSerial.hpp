/*
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
#ifndef QSMARTMETERTOMQTT_MESSAGESOURCEMBUSSERIAL_HPP
#define QSMARTMETERTOMQTT_MESSAGESOURCEMBUSSERIAL_HPP

#include <QTimer>
#include "IMessageSource.hpp"
#include "mbus/mbus.h"

class MessageSourceMbusSerial : public IMessageSource{
public:
    explicit MessageSourceMbusSerial(QString topic, QString device, QStringList addresses, uint32_t baudrate);
    ~MessageSourceMbusSerial() override;
    int32_t setup(bool debug = false);
    int32_t poll();
    int32_t init_slaves(mbus_handle * handle);
private slots:
    void readData();
private:
    void handleXmlData(char * data);
    QTimer m_timer;
    QString m_device;
    QStringList m_addresses;
    uint32_t m_baudrate;
    bool m_debug;
    mbus_handle *m_handle = nullptr;
    QString m_topic;
};


#endif //QSMARTMETERTOMQTT_MESSAGESOURCEMBUSSERIAL_HPP

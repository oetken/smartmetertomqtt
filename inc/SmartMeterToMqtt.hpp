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
#ifndef QSMARTMETERTOMQTT_QSMARTMETERTOMQTT_HPP
#define QSMARTMETERTOMQTT_QSMARTMETERTOMQTT_HPP

#include <QtMqtt/QtMqtt>
#include <QtCore/QDateTime>
#include <QtMqtt/QMqttClient>
#include <QSettings>
#include <mbus/mbus.h>
#include "IMessageSource.hpp"
#include "SmartMeterSettings.hpp"
#include "IMessageFilter.hpp"

class SmartMeterToMqtt : public QObject{
    Q_OBJECT
public:
    explicit SmartMeterToMqtt();
    explicit SmartMeterToMqtt(const QString &settingsFileName);
    bool addMessageSource(IMessageSource * messageSource);
    bool setup();
    bool setupClient(QString hostname, uint16_t port, QString user, QString password, QString clientId = "", uint32_t keepAliveTime = 10);
    bool publishMqttMessage(QString topic, QVariant message);
    bool getMessageSources();
private:
    bool getFilters(QJsonArray &messageFilters, IMessageSource *filters);

    SmartMeterSettings m_settings;
    QString m_filename = "/home/z001131e/.config/SmartHomeTools/SmartMeterToMqtt.json";
    QMqttClient * m_client{};
    QTimer m_timer;
    QTimer m_keepAliveSendTimer;
    QTimer m_keepAliveTimer;
protected slots:
    void updateLogStateChange(QMqttClient::ClientState state);
    void brokerDisconnected();
    void timerTimedout();
    void messageReceived(QString topic, QVariant message);
};


#endif //QSMARTMETERTOMQTT_QSMARTMETERTOMQTT_HPP

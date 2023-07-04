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
#include "SmartMeterToMqtt.hpp"
#include "MessageSourceSml.hpp"
#include "MessageSourceMbusSerial.hpp"
#include <QDebug>
#include <IMessageFilter.hpp>
#include <MessageFilterMean.hpp>
#include <MessageFilterSkip.hpp>

SmartMeterToMqtt::SmartMeterToMqtt() : m_settings(this) {
}

SmartMeterToMqtt::SmartMeterToMqtt(const QString &settingsFileName) : m_settings(this), m_filename(settingsFileName) {
}

bool SmartMeterToMqtt::setup() {
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &SmartMeterToMqtt::timerTimedout);
    m_timer.start();
    if(!m_settings.open(m_filename))
    {
        qCritical() << "Failed to open settings.";
        return false;
    }
    auto hostname = m_settings["MQTT"]["hostname"];
    if(hostname.isNull())
    {
        qCritical() << "Hostname must be supplied!";
        return false;
    }
    auto port = m_settings["MQTT"]["port"];
    if(port.isNull())
    {
        qCritical() << "Port must be supplied!";
        return false;
    }
    auto user = m_settings["MQTT"]["user"];
    if(user.isNull())
    {
        qCritical() << "User must be supplied!";
        return false;
    }
    auto password = m_settings["MQTT"]["password"];
    if(password.isNull())
    {
        qCritical() << "Password must be supplied!";
        return false;
    }
    auto clientid = m_settings["MQTT"]["clientid"];
    if(clientid.isNull())
    {
        qCritical() << "ClientId must be supplied!";
        return false;
    }
    if(!setupClient(
            hostname.toString(),
            port.toInt(),
            user.toString(),
            password.toString(),
            clientid.toString()
    )){
        qCritical() << "Failed to setup MQTT client!";
        return false;
    }
    if(!getMessageSources())
    {
        qCritical() << "Failed to setup MessageSources!";
        return false;
    }
    return true;
}

bool SmartMeterToMqtt::getMessageSources()
{
    auto messageSources = m_settings["MessageSources"].toArray();
    foreach (const auto & messageSource, messageSources) {
        IMessageSource * iMessageSource = nullptr;
        auto type = messageSource["type"];
        auto topic = messageSource["topic"];
        auto device = messageSource["device"];
        auto baudrate = messageSource["baudrate"];
        if(type.isNull() || topic.isNull() || device.isNull() || baudrate.isNull())
        {
            qCritical() << "Settings: Message source is corrupt!";
            return false;
        }
        if(type.toString() == "MbusSerial")
        {
            auto addressesString = messageSource["addresses"];
            if(addressesString.isNull())
            {
                qCritical() << "Settings: Message source is corrupt!";
                return false;
            }
            auto pollIntervalSec = messageSource["pollIntervalSec"];
            if(pollIntervalSec.isNull())
            {
                qCritical() << "Settings: Message source is corrupt!";
                return false;
            }
            QStringList addresses;
            for (auto & address : addressesString.toString().trimmed().split(','))
            {
                addresses.append(address.trimmed());
            }
            auto ms = new MessageSourceMbusSerial(topic.toString(), device.toString(), addresses, baudrate.toInt(), pollIntervalSec.toInt());
            ms->setup();
            addMessageSource(ms);
            iMessageSource = ms;
        }
        else if(type == "Sml")
        {
            auto ms = new MessageSourceSml(topic.toString(), device.toString(), baudrate.toInt());
            ms->setup();
            addMessageSource(ms);
            iMessageSource = ms;
        }
        else
        {
            qCritical() << "Settings: unknown message source" << type;
            return false;
        }
        auto messageFilters = messageSource["MessageFilters"].toArray();
        if(!getFilters(messageFilters, iMessageSource))
        {
            return false;
        }
    }
    if(messageSources.isEmpty())
    {
        qCritical() << "Settings: No message sources defined!";
        return false;
    }
    return true;
}

bool SmartMeterToMqtt::getFilters(QJsonArray & messageFilters, IMessageSource *messageSource) {
    foreach (const auto &messageFilter, messageFilters) {
            auto type = messageFilter["type"];
        if (type.isNull()) {
            qCritical() << "Settings: Message Filter is corrupt!";
            return false;
        }
        auto datapoint = messageFilter["datapoint"];
        if (datapoint.isNull()) {
            qCritical() << "Settings: Message Filter is corrupt!";
            return false;
        }
        qDebug() << type << datapoint;
        if (type.toString().compare("Mean", Qt::CaseInsensitive) == 0) {
            auto windowSize = messageFilter["windowSize"];
            if (windowSize.isNull()) {
                qCritical() << "Settings: Message Filter is corrupt!";
                return false;
            }
            auto threshold = messageFilter["threshold"];
            double threasholdValue = std::nan("");
            if(!threshold.isNull() && !threshold.isUndefined())
                threasholdValue = threshold.toDouble();
            auto postThresholdIncreaseSampleCount = messageFilter["postThresholdIncreaseSampleCount"];
            uint32_t postThresholdIncreaseSampleCountValue = 0;
            if(!postThresholdIncreaseSampleCount.isNull() && !postThresholdIncreaseSampleCount.isUndefined())
                postThresholdIncreaseSampleCountValue = postThresholdIncreaseSampleCount.toInt();
            auto filter = new MessageFilterMean(windowSize.toInt(), threasholdValue,
                                                postThresholdIncreaseSampleCountValue);
            messageSource->addFilter(datapoint.toString(), filter);

        } else if (type.toString().compare("Skip", Qt::CaseInsensitive) == 0) {
            auto skipCount = messageFilter["skipCount"];
            qDebug() << skipCount;
            if (skipCount.isNull()) {
                qCritical() << "Settings: Message Filter is corrupt!";
                return false;
            }
            auto filter = new MessageFilterSkip(skipCount.toInt());
            messageSource->addFilter(datapoint.toString(), filter);
        }
        else
        {
            qCritical() << "Settings: Unknown Message Filter type:" << type.toString();
            return false;
        }

    }
    return true;
}

bool SmartMeterToMqtt::setupClient(QString hostname, uint16_t port, QString user, QString password, QString clientId, uint32_t keepAliveTime) {
    // Setup MQTT client
    m_client = new QMqttClient(this);
    m_client->setHostname(hostname);
    m_client->setPort(port);
    m_client->setUsername(user);
    m_client->setPassword(password);
    m_client->setKeepAlive(keepAliveTime);
    m_client->setClientId(clientId);
    // Setup keep alive
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    //m_client->setAutoKeepAlive(true);
#else
    m_keepAliveSendTimer.setInterval(keepAliveTime * 1000);
    connect(&m_keepAliveSendTimer, &QTimer::timeout, [this](){
        // Manual ping request if old version of QtMQTT
        if(m_client->state() == QMqttClient::Connected)
            m_client->requestPing();
    });
    m_keepAliveSendTimer.start();
#endif
    // Keep alive timeout
    m_keepAliveTimer.setInterval(keepAliveTime * 3 * 1000);
    connect(&m_keepAliveTimer, &QTimer::timeout, [this](){
        // Keep alive timer is only timedout if no ping reponse received from server
        // Try to reconnect.
        m_client->connectToHost();
    });
    connect(m_client, &QMqttClient::stateChanged, this, &SmartMeterToMqtt::updateLogStateChange);
    connect(m_client, &QMqttClient::disconnected, this, &SmartMeterToMqtt::brokerDisconnected);
    connect(m_client, &QMqttClient::pingResponseReceived, this, [this]() {
        const QString content = QDateTime::currentDateTime().toString()
                                + QLatin1String(" PingResponse")
                                + QLatin1Char('\n');
        qDebug() << content;
        // Restart keep alive timer
        m_keepAliveTimer.start();
    });
    // Not used but we can just print it in case anyone wants to test stuff
    // Might be used to setup refresh rate, etc.
    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        const QString content = QDateTime::currentDateTime().toString()
                                + QLatin1String(" Received Topic: ")
                                + topic.name()
                                + QLatin1String(" Message: ")
                                + message
                                + QLatin1Char('\n');
        qDebug() << content;
    });

    // connect to host
    m_client->connectToHost();
    m_keepAliveTimer.start();
    return true;
}


bool SmartMeterToMqtt::publishMqttMessage(QString topic, QVariant message) {
    QString messageString = QString();
    if (message.type() == QVariant::Double){
        messageString = QString("%1").arg(message.value<double>(), 0, 'g', 12);
    } else {
        messageString = message.toString();
    }
    qDebug() << "Sending" << topic << messageString;
    return (m_client->publish(topic, messageString.toUtf8()) == -1);
}

void SmartMeterToMqtt::updateLogStateChange(QMqttClient::ClientState state) {
    if(state == QMqttClient::ClientState::Disconnected)
        qDebug() << "MQTT Disconnected!";
    if(state == QMqttClient::ClientState::Connecting)
        qDebug() << "MQTT Connecting!";
    if(state == QMqttClient::ClientState::Connected)
        qDebug() << "MQTT Connected!";
}

void SmartMeterToMqtt::brokerDisconnected() {
    qDebug() << "MQTT Disconnected!";
    m_client->connectToHost();
}

void SmartMeterToMqtt::timerTimedout() {
    static uint32_t x = 0;
    publishMqttMessage("test/test", x++);
}

bool SmartMeterToMqtt::addMessageSource(IMessageSource *messageSource) {
    connect(messageSource, &IMessageSource::messageReceived, this, &SmartMeterToMqtt::messageReceived);
    return true;
}

void SmartMeterToMqtt::messageReceived(QString topic, QVariant message) {
    publishMqttMessage(topic, message);
}

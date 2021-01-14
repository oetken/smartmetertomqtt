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
#include "SmartMeterToMqtt.hpp"
#include "MessageSourceSml.hpp"
#include "MessageSourceMbusSerial.hpp"
#include <QDebug>

bool SmartMeterToMqtt::setup() {
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &SmartMeterToMqtt::timerTimedout);
    m_timer.start();
    if(!readSettings())
    {
        qCritical() << "Failed to read settings!";
        return false;
    }
    if(!setupClient(
            m_settings.value("hostname").toString(),
            m_settings.value("port").toString().toInt(),
            m_settings.value("user").toString(),
            m_settings.value("password").toString()
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
    auto size = m_settings.beginReadArray("MessageSources");
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        auto type = m_settings.value("type").toString();
        auto topic = m_settings.value("topic").toString();
        auto device = m_settings.value("device").toString();
        auto baudrate = m_settings.value("baudrate").toInt();
        if(type == "MbusSerial")
        {
            auto addressesString = m_settings.value("addresses").toString();
            QStringList addresses;
            for (auto & address : addressesString.trimmed().split(','))
            {
                addresses.append(address.trimmed());
            }
            auto ms = new MessageSourceMbusSerial(topic, device, addresses, baudrate);
            ms->setup();
            addMessageSource(ms);
        }
        else if(type == "Sml")
        {
            auto ms = new MessageSourceSml(topic, device, baudrate);
            addMessageSource(ms);
        }
    }
    m_settings.endArray();
    if(size == 0)
    {
        qCritical() << "No message sources defined!";
        return false;
    }
    return true;
}

bool SmartMeterToMqtt::setupClient(QString hostname, uint16_t port, QString user, QString password) {
    m_client = new QMqttClient(this);
    m_client->setHostname(hostname);
    m_client->setPort(port);
    m_client->setUsername(user);
    //m_client->setAutoKeepAlive(true);
    m_client->setKeepAlive(10);
    m_client->setPassword(password);
    m_keepAliveTimer.setInterval(m_client->keepAlive() * 5 * 1000);
    connect(&m_keepAliveTimer, &QTimer::timeout, [this](){
       m_client->connectToHost();
    });
    connect(m_client, &QMqttClient::stateChanged, this, &SmartMeterToMqtt::updateLogStateChange);
    connect(m_client, &QMqttClient::disconnected, this, &SmartMeterToMqtt::brokerDisconnected);
    connect(m_client, &QMqttClient::pingResponseReceived, this, [this]() {
        const QString content = QDateTime::currentDateTime().toString()
                                + QLatin1String(" PingResponse")
                                + QLatin1Char('\n');
        qDebug() << content;
    });
    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        const QString content = QDateTime::currentDateTime().toString()
                                + QLatin1String(" Received Topic: ")
                                + topic.name()
                                + QLatin1String(" Message: ")
                                + message
                                + QLatin1Char('\n');
        qDebug() << content;
        m_keepAliveTimer.start();
    });
    m_client->connectToHost();
    m_keepAliveTimer.start();
    return true;
}

bool SmartMeterToMqtt::readSettings() {
    m_settings.setIniCodec("UTF-8");
    qDebug() << m_settings.fileName();
    bool settingsOk = true;
    if(m_settings.contains("hostname") && m_settings.value("hostname") != "<HOSTNAME>")
        qDebug() << m_settings.value("hostname");
    else {
        m_settings.setValue("hostname", "<HOSTNAME>");
        settingsOk = false;
    }
    if(m_settings.contains("port") && m_settings.value("port") != "<PORT>")
        qDebug() << m_settings.value("port");
    else {
        m_settings.setValue("port", "<PORT>");
        settingsOk = false;
    }

    if(m_settings.contains("user") && m_settings.value("user") != "<USER>")
        qDebug() << m_settings.value("user");
    else {
        m_settings.setValue("user", "<USER>");
        settingsOk = false;
    }

    if(m_settings.contains("password") && m_settings.value("password") != "<PASSWORD>")
        qDebug() << m_settings.value("password");
    else {
        m_settings.setValue("password", "<PASSWORD>");
        settingsOk = false;
    }

    if(!settingsOk)
    {
        qCritical() << "Fatal: Settings file is not setupClient properly. Open" << m_settings.fileName() << "and put correct settings.";
    }
    return settingsOk;
}

SmartMeterToMqtt::SmartMeterToMqtt() : m_settings("SmartHomeTools", "SmartMeterToMqtt", this) {
}

SmartMeterToMqtt::SmartMeterToMqtt(const QString &settingsFileName)  : m_settings(settingsFileName, QSettings::Format::IniFormat, this) {
}

bool SmartMeterToMqtt::publishMqttMessage(QString topic, QVariant message) {
    QString messageString = message.toString();
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




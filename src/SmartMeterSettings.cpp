#include "SmartMeterSettings.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

SmartMeterSettings::SmartMeterSettings()
{
}

bool SmartMeterSettings::open(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    auto data = file.readAll();
    file.close();
    m_doc = QJsonDocument::fromJson(data);
    qDebug() << m_doc["MQTT"]["User"].toString();
    qDebug() << m_doc["MQTT"]["Password"].toString();
    qDebug() << (*this)["MQTT"]["Password"].toString();
}

const QJsonValue SmartMeterSettings::operator[](QString name)
{
    return m_doc[name];
}


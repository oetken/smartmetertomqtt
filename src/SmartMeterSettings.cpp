#include "SmartMeterSettings.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

SmartMeterSettings::SmartMeterSettings(QObject * parent) : QObject(parent)
{
}

bool SmartMeterSettings::open(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    auto data = file.readAll();
    file.close();
    QJsonParseError error{};
    m_doc = QJsonDocument::fromJson(data, &error);
    if(error.error)
    {
        qCritical() << "Failed to parse Json file:" << error.errorString();
        return false;
    }
    return true;
}

QJsonValue SmartMeterSettings::operator[](QString name)
{
    return m_doc[name];
}


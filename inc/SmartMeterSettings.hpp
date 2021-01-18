#ifndef SMARTMETERSETTINGS_HPP
#define SMARTMETERSETTINGS_HPP

#include <QJsonDocument>

class SmartMeterSettings : public QObject
{
    Q_OBJECT
public:
    SmartMeterSettings(QObject * parent);
    bool open(QString filename);
    QJsonValue operator[](QString name);
private:
    QJsonDocument m_doc;
};

#endif // SMARTMETERSETTINGS_HPP

#ifndef SMARTMETERSETTINGS_HPP
#define SMARTMETERSETTINGS_HPP

#include <QJsonDocument>

class SmartMeterSettings : public QObject
{
    Q_OBJECT
public:
    SmartMeterSettings();
    bool open(QString filename);
    const QJsonValue operator[](QString name);
private:
    QJsonDocument m_doc;
};

#endif // SMARTMETERSETTINGS_HPP

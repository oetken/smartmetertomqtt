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
#ifndef QSMARTMETERTOMQTT_IMESSAGESOURCE_HPP
#define QSMARTMETERTOMQTT_IMESSAGESOURCE_HPP

#include <QObject>
#include <QVariant>
#include "IMessageFilter.hpp"
#include <QMultiHash>
#include <QDebug>

class IMessageSource : public QObject{
    Q_OBJECT
public:
    virtual bool addFilter(QString datapoint, IMessageFilter * filter)
    {
        if (datapoint.endsWith("*") && !filter->wildchar() )
        {
            qCritical() << "Settings: Message Filter shall be installed for wildchar datapoint but not allowed by filter " + filter->type() + "!";
            return false;
        }

        m_filters.insert(datapoint, filter);
        return true;
    }
signals:
    void messageReceived(QString topic, QVariant value);

protected:
    QMultiHash<QString, IMessageFilter *> m_filters;
};


#endif //QSMARTMETERTOMQTT_IMESSAGESOURCE_HPP

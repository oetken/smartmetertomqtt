/*  Copyright 2023, Fabian Hassel and the smartmetertomqtt contributors.

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
#ifndef SMARTMETERTOMQTT_MESSAGEFILTERDELTA_HPP
#define SMARTMETERTOMQTT_MESSAGEFILTERDELTA_HPP

#include "IMessageFilter.hpp"

class MessageFilterDelta : public IMessageFilter{
public:
    explicit MessageFilterDelta();
    explicit MessageFilterDelta(QString m_name);
    QVariant filter(QVariant value) override;
    QString  rename(QString name) override;
    QString  type() override { return "Delta"; };

private:
    QVariant m_lastValue;
    QString m_name;
};


#endif //SMARTMETERTOMQTT_MESSAGEFILTERSKIP_HPP

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
#ifndef SMARTMETERTOMQTT_MESSAGEFILTERIGNORE_HPP
#define SMARTMETERTOMQTT_MESSAGEFILTERIGNORE_HPP

#include "IMessageFilter.hpp"

class MessageFilterIgnore : public IMessageFilter{
public:
    explicit MessageFilterIgnore();
    QVariant filter(QVariant value) override;
    QString  type() override { return "Ignore"; };
};


#endif //SMARTMETERTOMQTT_MESSAGEFILTERSKIP_HPP

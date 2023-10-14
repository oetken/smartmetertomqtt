/*  Copyright 2023 - 2023, Fabian Hassel and the smartmetertomqtt contributors.

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

#ifndef OBISCODE_H
#define OBISCODE_H

#include <QString>
#include <QHash>
#include "sml/sml_octet_string.h"

class ObisCode;

class ObisCode
{
    
    public:
        ObisCode(uint8_t medium, uint8_t channel, uint8_t value, uint8_t quantity, uint8_t group, uint8_t range);
        ObisCode(uint8_t medium, uint8_t channel, uint8_t value, uint8_t quantity, uint8_t group);
        ObisCode(uint8_t channel, uint8_t value, uint8_t quantiy, uint8_t group);
        ObisCode(uint8_t value, uint8_t quantiy, uint8_t group);
        ObisCode(octet_string* obis_code);

        bool isValid() const {return valid_;}
        bool hasName() const {return isValid() && names_.contains(*this);} 
        bool hasPostfix() const {return isValid() && postfixes_.contains(this->range_);} 
        const QString getName() const {return hasName() ? names_.value(*this) : "";}
        const QString getPostfix() const {return hasPostfix() ? postfixes_.value(this->range_) : "";}
        const QString toObisString() const;
        const QString toReadableString() const;
        const QString toString() const;

        friend bool operator==(const ObisCode &lhs, const ObisCode &rhs);

    private:
        uint8_t medium_;
        uint8_t channel_;
        uint8_t value_;
        uint8_t quantity_;
        uint8_t group_;
        uint8_t range_;
        bool    valid_;

    private:
        static const QHash<ObisCode,QString> names_;
        static const QHash<uint8_t,QString> postfixes_;
};

inline bool operator==(const ObisCode &lhs, const ObisCode &rhs)
{
    return lhs.medium_   == rhs.medium_  &&
           lhs.channel_  == rhs.channel_ &&
           lhs.value_    == rhs.value_   &&
           lhs.quantity_ == rhs.quantity_ &&
           lhs.group_    == rhs.group_ ;
}

inline uint qHash(const ObisCode &key, uint seed)
{
    return qHash(key.toObisString(), seed ^ 0xb036);
}
#endif // OBISCODE_H

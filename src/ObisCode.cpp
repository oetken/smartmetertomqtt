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

#include "ObisCode.hpp"
#include <QTextStream>

ObisCode::ObisCode(uint8_t medium, uint8_t channel, uint8_t value, uint8_t quantity, uint8_t group, uint8_t range) : 
medium_(medium), channel_(channel), value_(value), quantity_(quantity), group_(group), range_(range), valid_(true)
{
}

ObisCode::ObisCode(uint8_t medium, uint8_t channel, uint8_t value, uint8_t quantity, uint8_t group) : 
medium_(medium), channel_(channel), value_(value), quantity_(quantity), group_(group), range_(255), valid_(true)
{
}

ObisCode::ObisCode(uint8_t channel, uint8_t value, uint8_t quantity, uint8_t group) : 
medium_(1), channel_(channel), value_(value), quantity_(quantity), group_(group), range_(255), valid_(true)
{
}

ObisCode::ObisCode(uint8_t value, uint8_t quantity, uint8_t group) : 
medium_(1), channel_(0), value_(value), quantity_(quantity), group_(group), range_(255), valid_(true)
{
}

ObisCode::ObisCode(octet_string* obis_code) : 
medium_(0), channel_(0), value_(0), quantity_(0), group_(0), range_(0), valid_(true)
{
    if (obis_code->len == 6)
    {
        medium_   = obis_code->str[0];
        channel_  = obis_code->str[1];
        value_    = obis_code->str[2];
        quantity_ = obis_code->str[3];
        group_    = obis_code->str[4];
        range_    = obis_code->str[5];
        valid_    = true;
    }
}

QString ObisCode::toObisString() const 
{
    QString string;

    if (isValid()){
        QTextStream s(&string);
        s << medium_ << "-" << channel_ << ":" << value_ << "." << quantity_ << "." << group_ << "*" << range_;
    }else{
        string = QString("Invalid OBIS Code!");
    }

    return string;
}

QString ObisCode::toReadableString() const 
{
    QString string;

    if (isValid())
    {
        string = getName();
    }else{
        string = QString("Invalid OBIS Code!");
    }

    return string;
}

QString ObisCode::toString() const 
{
    QString string;

    if (hasText())
    {
        string = toReadableString();
    }else{
        string = toObisString();
    }

    return string;
}
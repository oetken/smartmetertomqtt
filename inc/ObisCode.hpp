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
#include "sml/sml_octet_string.h"

class ObisCode
{
    
    public:
        ObisCode(uint8_t medium, uint8_t channel, uint8_t value, uint8_t quantity, uint8_t group, uint8_t range);
        ObisCode(uint8_t medium, uint8_t channel, uint8_t value, uint8_t quantity, uint8_t group);
        ObisCode(uint8_t channel, uint8_t value, uint8_t quantiy, uint8_t group);
        ObisCode(uint8_t value, uint8_t quantiy, uint8_t group);
        ObisCode(octet_string* obis_code);

        bool isValid() const {return valid_;}
        bool hasText() const {return isValid() && false;} //TODO: Has to be implmented!
        QString getName() const {return QString();} //TODO: Has to be implmented!
        QString toObisString() const;
        QString toReadableString() const;
        QString toString() const;

    private:
        uint8_t medium_;
        uint8_t channel_;
        uint8_t value_;
        uint8_t quantity_;
        uint8_t group_;
        uint8_t range_;
        bool    valid_;
};

#endif // OBISCODE_H

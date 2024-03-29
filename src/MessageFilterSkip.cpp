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
#include "MessageFilterSkip.hpp"

MessageFilterSkip::MessageFilterSkip(uint32_t skipCount, QString name) : m_skipCount(skipCount), m_name(name){
}

QString MessageFilterSkip::rename(QString name) {
  if (m_name.isEmpty())
    return name;
  return m_name;
}

QVariant MessageFilterSkip::filter(QVariant value) {
    if(m_skipped++ <= m_skipCount)
        return QVariant();
    m_skipped = 0;
    return value;
}

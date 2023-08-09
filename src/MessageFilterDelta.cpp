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
#include "MessageFilterDelta.hpp"

MessageFilterDelta::MessageFilterDelta() : m_lastValue(QVariant()), m_name(QString()){
}

MessageFilterDelta::MessageFilterDelta(QString name) : m_lastValue(QVariant()), m_name(name){
}

QString MessageFilterDelta::rename(QString name) {
    if (m_name.isEmpty())
      return name;
    return m_name;
}

QVariant MessageFilterDelta::filter(QVariant value) {
    QVariant delta = QVariant();

    if (value.canConvert<double>())
    {
      double new_val = value.value<double>();

      if (!m_lastValue.isNull() && m_lastValue.canConvert<double>())
      {
        double old_val = m_lastValue.value<double>();
        delta.setValue<double>(new_val - old_val);
      }

      m_lastValue.setValue<double>(new_val);
    }
    
    return delta;
}

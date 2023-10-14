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
#ifndef SMARTMETERTOMQTT_MESSAGEFILTERMEAN_HPP
#define SMARTMETERTOMQTT_MESSAGEFILTERMEAN_HPP

#include "IMessageFilter.hpp"

class MessageFilterMean : public IMessageFilter {
public:
    QVariant filter(QVariant value) override;
    explicit MessageFilterMean(uint32_t sampleCount, double threshold = std::nan(""), uint32_t postThresholdIncreaseSampleCount = 0);
    QString  type() override { return "Mean"; };
private:
    QVariant getMeanValue(bool force = false);

    uint32_t m_sampleCount;
    uint32_t m_currentSampleCount;
    double m_threshold;
    uint32_t m_postThresholdIncreaseSampleCount;
    uint32_t m_postThresholdSampleCount{};
    QList<QVariant> m_samples;
    QVariant m_lastValue{};
};


#endif //SMARTMETERTOMQTT_MESSAGEFILTERMEAN_HPP

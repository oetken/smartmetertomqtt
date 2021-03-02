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
#include <cmath>
#include "MessageFilterMean.hpp"

QVariant MessageFilterMean::filter(QVariant value) {
    QVariant retVal;
    if(!std::isnan(m_threshold) and abs(value.toDouble() - m_lastValue.toDouble()) >= m_threshold )
    {
        if(m_samples.empty())
        {
            retVal = value;
        }
        else {
            QVariantList variantList;
            variantList.append(getMeanValue(true));
            variantList.append(value);
            retVal = variantList;
        }
        // set post threshold sample values
        m_postThresholdSampleCount = 0;
        if(m_postThresholdIncreaseSampleCount)
            m_currentSampleCount = 0;
    }
    else
    {
        m_samples.push_back(value);
        retVal = getMeanValue();
    }
    m_lastValue = value;

    return retVal;
}

MessageFilterMean::MessageFilterMean(uint32_t sampleCount, double threshold, uint32_t postThresholdIncreaseSampleCount) :
    m_sampleCount(sampleCount), m_currentSampleCount(sampleCount), m_threshold(threshold),
    m_postThresholdIncreaseSampleCount(postThresholdIncreaseSampleCount) {
}

QVariant MessageFilterMean::getMeanValue(bool force) {
    if(m_samples.size() <= m_currentSampleCount && !force)
        return QVariant();

    double mean = 0;
    for(auto & sample : m_samples)
    {
        mean = mean + sample.toDouble();
    }
    mean /= m_samples.size();
    m_samples.clear();

    if(m_postThresholdIncreaseSampleCount) {
        m_postThresholdSampleCount = (m_postThresholdSampleCount + 1) % m_postThresholdIncreaseSampleCount;
        if (m_postThresholdSampleCount == 0
            && m_currentSampleCount < m_sampleCount) {
            m_currentSampleCount++;
        }
    }

    return mean;
}

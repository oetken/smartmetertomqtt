//
// Created by z001131e on 18.01.21.
//

#include <cmath>
#include "MessageFilterMean.hpp"

QVariant MessageFilterMean::filter(QVariant value) {

    if(!std::isnan(m_threshold) and abs(value.toDouble() - m_lastValue.toDouble()) >= m_threshold )
    {
        m_lastValue = value;
        if(m_samples.empty())
        {
            return value;
        }
        QVariantList retVal;
        retVal.append(getMeanValue());
        retVal.append(value);
        return retVal;
    }

    m_samples.push_back(value);
    m_lastValue = value;
    if(m_samples.size() != m_sampleCount)
        return QVariant();

    auto mean = getMeanValue();
    return QVariant(mean);
}

MessageFilterMean::MessageFilterMean(uint32_t sampleCount, double threshold) : m_sampleCount(sampleCount), m_threshold(threshold) {
}

double MessageFilterMean::getMeanValue() {
    double mean = 0;
    for(auto & sample : m_samples)
    {
        mean = mean + sample.toDouble();
    }
    mean /= m_sampleCount;
    m_samples.clear();
    return mean;
}

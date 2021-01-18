//
// Created by z001131e on 18.01.21.
//

#include "MessageFilterMean.hpp"

QVariant MessageFilterMean::filter(QVariant value) {
    m_samples.push_back(value);
    if(m_samples.size() != m_sampleCount)
        return QVariant();

    double mean;
    for(auto & sample : m_samples)
    {
        mean = mean + sample.toDouble();
    }
    mean /= m_sampleCount;
    m_samples.clear();
    return QVariant(mean);
}

MessageFilterMean::MessageFilterMean(uint32_t sampleCount) : m_sampleCount(sampleCount) {
}

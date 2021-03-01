//
// Created by z001131e on 18.01.21.
//

#ifndef SMARTMETERTOMQTT_MESSAGEFILTERMEAN_HPP
#define SMARTMETERTOMQTT_MESSAGEFILTERMEAN_HPP

#include "IMessageFilter.hpp"

class MessageFilterMean : public IMessageFilter {
public:
    QVariant filter(QVariant value) override;
    explicit MessageFilterMean(uint32_t sampleCount, double threshold = std::nan(""));

private:
    uint32_t m_sampleCount;
    QList<QVariant> m_samples;
    double m_threshold;
    QVariant m_lastValue;

    double getMeanValue();
};


#endif //SMARTMETERTOMQTT_MESSAGEFILTERMEAN_HPP

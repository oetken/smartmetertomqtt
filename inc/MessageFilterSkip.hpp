//
// Created by z001131e on 18.01.21.
//

#ifndef SMARTMETERTOMQTT_MESSAGEFILTERSKIP_HPP
#define SMARTMETERTOMQTT_MESSAGEFILTERSKIP_HPP

#include "IMessageFilter.hpp"

class MessageFilterSkip : public IMessageFilter{
public:
    explicit MessageFilterSkip(uint32_t skipCount);
    QVariant filter(QVariant value) override;

private:
    uint32_t m_skipCount;
    uint32_t m_skipped{};
};


#endif //SMARTMETERTOMQTT_MESSAGEFILTERSKIP_HPP

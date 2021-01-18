//
// Created by z001131e on 18.01.21.
//

#include "MessageFilterSkip.hpp"

MessageFilterSkip::MessageFilterSkip(uint32_t skipCount) : m_skipCount(skipCount){
}

QVariant MessageFilterSkip::filter(QVariant value) {
    if(m_skipped++ <= m_skipCount)
        return QVariant();
    m_skipped = 0;
    return value;
}

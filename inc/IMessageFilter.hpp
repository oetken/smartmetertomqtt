//
// Created by z001131e on 18.01.21.
//

#ifndef SMARTMETERTOMQTT_IMESSAGEFILTER_HPP
#define SMARTMETERTOMQTT_IMESSAGEFILTER_HPP
#include <QVariant>

class IMessageFilter {
public:
    virtual QVariant filter(QVariant value) = 0;
};

#endif //SMARTMETERTOMQTT_IMESSAGEFILTER_HPP

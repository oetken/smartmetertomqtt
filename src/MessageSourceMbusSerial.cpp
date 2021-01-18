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

/* setup and poll are taken from https://github.com/rscada/libmbus/blob/master/bin/mbus-serial-request-data.c
 *
 * //------------------------------------------------------------------------------
 * // Copyright (C) 2011, Robert Johansson, Raditex AB
 * // All rights reserved.
 * //
 * // rSCADA
 * // http://www.rSCADA.se
 * // info@rscada.se
 * //
 * //------------------------------------------------------------------------------
 * BSD 3-Clause License

Copyright (c) 2010-2012, Raditex Control AB
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include <cstddef>
#include "MessageSourceMbusSerial.hpp"
#include <QDebug>
#include <QtXml>
#include "mbus/mbus.h"
#include <QtXmlPatterns/QXmlQuery>

MessageSourceMbusSerial::MessageSourceMbusSerial(QString topic, QString device, QStringList addresses, uint32_t baudrate) : m_topic(topic), m_device(device), m_addresses(addresses), m_baudrate(baudrate){
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &MessageSourceMbusSerial::readData);
    m_timer.start();
}

int32_t MessageSourceMbusSerial::setup(bool debug) {
    const char *device;
    long baudrate = m_baudrate;


    auto deviceStdString = new std::string(m_device.toStdString());
    device = deviceStdString->c_str();
    m_debug = debug;

    if ((m_handle = mbus_context_serial(device)) == NULL) {
        qCritical() << "Could not initialize M-Bus context: " << mbus_error_str();
        return 1;
    }

    if (m_debug) {
        mbus_register_send_event(m_handle, &mbus_dump_send_event);
        mbus_register_recv_event(m_handle, &mbus_dump_recv_event);
    }

    if (mbus_connect(m_handle) == -1) {
        qCritical() << "Failed to setup connection to M-bus gateway";
        mbus_context_free(m_handle);
        m_handle = nullptr;
        return 1;
    }

    if (mbus_serial_set_baudrate(m_handle, baudrate) == -1) {
        qCritical() << "Failed to set baud rate.";
        mbus_disconnect(m_handle);
        mbus_context_free(m_handle);
        return 1;
    }

    if (init_slaves(m_handle) == 0) {
        mbus_disconnect(m_handle);
        mbus_context_free(m_handle);
        return 1;
    }
    return 0;
};

int32_t MessageSourceMbusSerial::poll() {
    int address;
    char *xml_result;
    mbus_frame reply;
    mbus_frame_data reply_data;
    memset((void *) &reply, 0, sizeof(mbus_frame));
    memset((void *) &reply_data, 0, sizeof(mbus_frame_data));

    for(auto & addressQString : m_addresses)
    {
        auto addr_sstr = addressQString.toStdString();
        auto addr_str = addr_sstr.c_str();
        if (mbus_is_secondary_address(addr_str))
        {
            // secondary addressing
            int ret;

            ret = mbus_select_secondary_address(m_handle, addr_str);

            if (ret == MBUS_PROBE_COLLISION)
            {
                qCritical() << "Error: The address mask [%s] matches more than one device" << addr_str;
                continue;
            }
            else if (ret == MBUS_PROBE_NOTHING)
            {
                qCritical() << "%Error: The selected secondary address does not match any device" << addr_str;
                continue;
            }
            else if (ret == MBUS_PROBE_ERROR)
            {
                qCritical() << "Error: Failed to select secondary address" << addr_str;
                continue;
            }
            // else MBUS_PROBE_SINGLE

            address = MBUS_ADDRESS_NETWORK_LAYER;
        }
        else
        {
            // primary addressing
            address = atoi(addr_str);
        }

        if (mbus_send_request_frame(m_handle, address) == -1)
        {
            qCritical() << "Failed to send M-Bus request frame.";
            mbus_disconnect(m_handle);
            mbus_context_free(m_handle);
            continue;
        }

        if (mbus_recv_frame(m_handle, &reply) != MBUS_RECV_RESULT_OK)
        {
            qCritical() << "Failed to receive M-Bus response frame.";
            continue;
        }

        //
        // dump hex data if m_debug is true
        //
        if (m_debug)
        {
            mbus_frame_print(&reply);
        }

        //
        // parse data
        //
        if (mbus_frame_data_parse(&reply, &reply_data) == -1)
        {
            qCritical() << "M-bus data parse error: " << mbus_error_str();
            continue;
        }

        //
        // generate XML and print to standard output
        //
        if ((xml_result = mbus_frame_data_xml(&reply_data)) == NULL)
        {
            qCritical() << "Failed to generate XML representation of MBUS frame:" << mbus_error_str();
            continue;
        }
        handleXmlData(xml_result);
        free(xml_result);

        // manual free
        if (reply_data.data_var.record)
        {
            mbus_data_record_free(reply_data.data_var.record); // free's up the whole list
        }
    }
    return 0;
}

void MessageSourceMbusSerial::handleXmlData(char * data)
{
    QByteArray dataArray(data);
    QStringList values;
    QBuffer buffer(&dataArray);
    QStringList names;
    buffer.open(QIODevice::ReadOnly);
    QXmlQuery query;
    query.bindVariable("myDocument", &buffer);
    query.setQuery("doc($myDocument)//MBusData/DataRecord/Function/string()");
    query.evaluateTo(&names);
    foreach (const QString &name, names) {
        query.setQuery("doc($myDocument)//MBusData/DataRecord[./Function='"+name+"']/Value/string()");
        query.evaluateTo(&values);
        auto value = values[0];
        if(m_filters.contains(name))
        {
            QVariant variant = m_filters[name]->filter(value);
            if(!variant.isNull())
            {
                emit messageReceived(m_topic + "/" + name, variant);
                qDebug() << m_topic + "/" + name << variant;
            }
        }
        else
        {
            emit messageReceived(m_topic + "/" + name, value);
            qDebug() << m_topic + "/" + name << value;
        }
    }
}

int32_t MessageSourceMbusSerial::init_slaves(mbus_handle *handle) {
    if (m_debug)
        qDebug() << "m_debug: sending init frame #1";

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1) == -1)
    {
        return 0;
    }

    //
    // resend SND_NKE, maybe the first get lost
    //

    if (m_debug)
        qDebug() << "m_debug: sending init frame #2";

    if (mbus_send_ping_frame(handle, MBUS_ADDRESS_NETWORK_LAYER, 1) == -1)
    {
        return 0;
    }

    return 1;
}

void MessageSourceMbusSerial::readData() {
    if(m_handle)
        poll();
}

MessageSourceMbusSerial::~MessageSourceMbusSerial() {
    if(m_handle) {
        mbus_disconnect(m_handle);
        mbus_context_free(m_handle);
    }
}



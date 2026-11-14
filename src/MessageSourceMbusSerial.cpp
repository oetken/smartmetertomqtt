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
#include <QMutex>
#include <QMutexLocker>
#include <QHash>
#include <QThread>
#include <QDateTime>
#include "mbus/mbus.h"

// One M-Bus serial bus per device path: several message sources (water, gas) may
// share the same adapter. Sharing one libmbus context avoids two file descriptors
// fighting over the same tty, which produced "Failed to receive M-Bus response
// frame" and, worse, a use-after-free in the old error path (SIGSEGV).
static QMutex g_mbusBusMutex;
static QHash<QString, mbus_handle *> g_mbusBuses;
// M-Bus meters need a quiet gap between two transactions; without it, a request
// to the second meter right after the first one is silently ignored.
static qint64 g_lastBusUseMs = 0;
static constexpr qint64 kBusGapMs = 700;

MessageSourceMbusSerial::MessageSourceMbusSerial(QString topic, QString device, QStringList addresses, uint32_t baudrate, uint32_t pollIntervalSec,
                                                 QString deviceName, QString deviceId)
    : m_topic(topic), m_device(device), m_addresses(addresses), m_baudrate(baudrate), m_pollIntervalSec(pollIntervalSec)
{
    m_sourceType = "MbusSerial";
    m_deviceName = deviceName.isEmpty() ? topic : deviceName;
    m_deviceId   = deviceId.isEmpty()   ? topic : deviceId;
    m_timer.setInterval(m_pollIntervalSec * 1000);
    connect(&m_timer, &QTimer::timeout, this, &MessageSourceMbusSerial::readData);
    m_timer.start();
}

int32_t MessageSourceMbusSerial::setup(bool debug) {
    m_debug = debug;

    QMutexLocker lock(&g_mbusBusMutex);
    if (mbus_handle *shared = g_mbusBuses.value(m_device, nullptr)) {
        m_handle = shared;
        return 0;
    }

    auto deviceStdString = new std::string(m_device.toStdString());
    const char *device = deviceStdString->c_str();

    mbus_handle *handle = mbus_context_serial(device);
    if (handle == nullptr) {
        qCritical() << "Could not initialize M-Bus context:" << mbus_error_str();
        return 1;
    }

    if (m_debug) {
        mbus_register_send_event(handle, &mbus_dump_send_event);
        mbus_register_recv_event(handle, &mbus_dump_recv_event);
    }

    if (mbus_connect(handle) == -1) {
        qCritical() << "Failed to setup connection to M-bus gateway";
        mbus_context_free(handle);
        return 1;
    }

    if (mbus_serial_set_baudrate(handle, m_baudrate) == -1) {
        qCritical() << "Failed to set baud rate.";
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    if (init_slaves(handle) == 0) {
        mbus_disconnect(handle);
        mbus_context_free(handle);
        return 1;
    }

    g_mbusBuses.insert(m_device, handle);
    m_handle = handle;
    return 0;
}

int32_t MessageSourceMbusSerial::poll() {
    if (!m_handle)
        return 1;
    QMutexLocker lock(&g_mbusBusMutex);
    int address;
    char *xml_result;
    mbus_frame reply;
    mbus_frame_data reply_data;
    memset((void *) &reply, 0, sizeof(mbus_frame));
    memset((void *) &reply_data, 0, sizeof(mbus_frame_data));

    for(auto & addressQString : m_addresses)
    {
        // enforce the inter-transaction gap on the shared bus
        if (g_lastBusUseMs) {
            qint64 since = QDateTime::currentMSecsSinceEpoch() - g_lastBusUseMs;
            if (since < kBusGapMs)
                QThread::msleep(kBusGapMs - since);
        }

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

        // A weak/slow meter answers only a fraction of requests, so retry a few
        // times before giving up. Errors are only logged once it is persistent.
        bool got_response = false;
        for (int attempt = 1; attempt <= 4 && !got_response; attempt++) {
            if (attempt > 1)
                QThread::msleep(400);
            if (g_lastBusUseMs) {
                qint64 since = QDateTime::currentMSecsSinceEpoch() - g_lastBusUseMs;
                if (since < kBusGapMs)
                    QThread::msleep(kBusGapMs - since);
            }
            if (mbus_send_request_frame(m_handle, address) == -1) {
                g_lastBusUseMs = QDateTime::currentMSecsSinceEpoch();
                continue;
            }
            int recv = mbus_recv_frame(m_handle, &reply);
            g_lastBusUseMs = QDateTime::currentMSecsSinceEpoch();
            if (recv == MBUS_RECV_RESULT_OK)
                got_response = true;
        }

        if (!got_response) {
            m_consecutiveFailures++;
            if (m_consecutiveFailures == 3 || m_consecutiveFailures % 20 == 0)
                qWarning() << "M-Bus" << m_topic << "address" << address
                           << "- no response (" << m_consecutiveFailures << " polls in a row)";
            continue;
        }
        if (m_consecutiveFailures > 0) {
            qInfo() << "M-Bus" << m_topic << "address" << address
                    << "- responding again after" << m_consecutiveFailures << "failed polls";
            m_consecutiveFailures = 0;
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
        g_lastBusUseMs = QDateTime::currentMSecsSinceEpoch();

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

    QDomDocument document;
    if(!document.setContent(&buffer))
    {
        qDebug() << "Failed to load the file for reading.";
        return;
    }
    auto root = document.firstChildElement();

    auto SlaveInformation = root.toElement().elementsByTagName("SlaveInformation");
    for(int i = 0; i < SlaveInformation.count(); i++)
    {
        auto SlaveInfo = SlaveInformation.at(i).toElement();
        auto children = SlaveInfo.childNodes();
        for(int i = 0; i < children.count(); i++)
        {
            auto value = children.at(i).toElement().text();
            auto name = children.at(i).nodeName();

            processFilters("slaveinfo", name , value);
        }
    }

    auto DataRecords = root.toElement().elementsByTagName("DataRecord");
    for(int i = 0; i < DataRecords.count(); i++)
    {
        auto DataRecord = DataRecords.at(i).toElement();
        auto id = DataRecord.attribute("id");
        auto children = DataRecord.childNodes();
        for(int i = 0; i < children.count(); i++)
        {
            auto value = children.at(i).toElement().text();
            auto name = children.at(i).nodeName();

            processFilters(id, name, value);
        }
    }
}

void MessageSourceMbusSerial::processFilters(const QString id, const QString name, const QString value){
    QString reference = id + "/" + name;
    QString search = reference;
    bool found = false;

    qDebug() << "Processing MBUS value" << reference;

    // match longest string first
    if(m_filters.contains(search))
    {
        found = true;
    }
    else
    {
        search = id + "/*";
    }

    // otherwise try wildchar
    if(found || m_filters.contains(search))
    {
        for(auto filter : m_filters.values(search))
        {
            qDebug() << "Running filter" << filter->type();
            QVariant variant = filter->filter(value);
            QString string = filter->rename(reference);
            if(!variant.isNull())
            {
                if(variant.canConvert<QVariantList>())
                {
                    for(QVariant element : variant.toList())
                    {
                        QString fullTopic = m_topic + "/" + string;
                        if(!m_discoveredEntities.contains(fullTopic)) {
                            m_discoveredEntities.insert(fullTopic);
                            emit entityDiscovered(fullTopic, string, m_deviceId, m_deviceName, m_sourceType);
                        }
                        emit messageReceived(fullTopic, element);
                        qDebug() << "filtered" << string << element;
                    }
                } else {
                    QString fullTopic = m_topic + "/" + string;
                    if(!m_discoveredEntities.contains(fullTopic)) {
                        m_discoveredEntities.insert(fullTopic);
                        emit entityDiscovered(fullTopic, string, m_deviceId, m_deviceName, m_sourceType);
                    }
                    emit messageReceived(fullTopic, variant);
                    qDebug() << m_topic + "/" + string + "/" + variant.toString();
                }
            }
        }
    } else {
        QString fullTopic = m_topic + "/" + reference;
        if(!m_discoveredEntities.contains(fullTopic)) {
            m_discoveredEntities.insert(fullTopic);
            emit entityDiscovered(fullTopic, reference, m_deviceId, m_deviceName, m_sourceType);
        }
        qDebug() << fullTopic + "/" + value;
        emit messageReceived(fullTopic, value);
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
    // The libmbus context is shared between all sources on the same serial device
    // and stays open for the lifetime of the process.
}



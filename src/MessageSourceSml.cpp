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
#include "MessageSourceSml.hpp"
#include <QByteArrayMatcher>
#include "sml/sml_file.h"
#include <QTextStream>
#include <QDebug>

MessageSourceSml::MessageSourceSml(QString topicBase, QString device, uint32_t baudrate) : topicBase_(topicBase), device_(device), baudrate_(baudrate)
{
}

bool MessageSourceSml::setup() {
    serialPort_.setBaudRate(QSerialPort::BaudRate(baudrate_));
    serialPort_.setPortName(device_);
    if(!serialPort_.open(QIODevice::ReadOnly)) {
        qCritical() << "Faield to open serial port" << serialPort_.errorString();
        return false;
    }

    connect(&serialPort_, &QSerialPort::readyRead, this, &MessageSourceSml::handleReadReady);
    return true;
}


void MessageSourceSml::handleReadReady()
{
    qDebug() << "ReadReady";
    readData_.append(serialPort_.readAll());
    QByteArrayMatcher matcher(startPattern_, sizeof(startPattern_));
    auto index = matcher.indexIn(readData_);
    if(index >= 0)
    {
	qDebug() << "Found start pattern" << index;
        readData_.remove(0, index);
        QByteArrayMatcher matcher(endPattern_, sizeof(endPattern_));
        index = matcher.indexIn(readData_, sizeof(startPattern_));
        if(index >= 0)
        {
    	    qDebug() << "Found end pattern";
            sml_file *file = sml_file_parse((unsigned char*)(readData_.constData()) + sizeof(startPattern_), index - sizeof(startPattern_));
            int i = 0;
            double value;
            for (i = 0; i < file->messages_len; i++) {
                sml_message *message = file->messages[i];
                if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE) {
                   sml_list *entry;
                   sml_get_list_response *body;
                   body = (sml_get_list_response *) message->message_body->data;
                   for (entry = body->val_list; entry != NULL; entry = entry->next) {
                      switch (entry->value->type) {
                         case 0x51: value= *entry->value->data.int8; break;
                         case 0x52: value= *entry->value->data.int16; break;
                         case 0x54: value= *entry->value->data.int32; break;
                         case 0x58: value= *entry->value->data.int64; break;
                         case 0x61: value= *entry->value->data.uint8; break;
                         case 0x62: value= *entry->value->data.uint16; break;
                         case 0x64: value= *entry->value->data.uint32; break;
                         case 0x68: value= *entry->value->data.uint64; break;
                         default:
                            value = 0;
                      }
                      int scaler = (entry->scaler) ? *entry->scaler : 1;
                      if (scaler==-1)
                         value *= 0.0001;
                      QString string;
                      QTextStream s(&string);
                      s << entry->obj_name->str[0] << "-" << entry->obj_name->str[1]
                        << ":" << entry->obj_name->str[2] << "." << entry->obj_name->str[3]
                        << "." << entry->obj_name->str[4] << "*" << entry->obj_name->str[5];
                      if(m_filters.contains(string))
                      {
                          QVariant variant = m_filters[string]->filter(value);
                          if(!variant.isNull())
                          {
                              emit messageReceived(topicBase_ + "/" + string, variant);
                              qDebug() << "filtered" << string << variant;
                          }
                      }
                      else
                      {
                          emit messageReceived(topicBase_ + "/" + string, value);
                          qDebug() << string << value;
                      }
                   }
                }
            }
            readData_.remove(0, index + sizeof(endPattern_));
        }
    }
    else
    {
        readData_.remove(0, readData_.length() - sizeof(startPattern_));
    }
}


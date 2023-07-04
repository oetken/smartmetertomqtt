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
#include "ObisCode.hpp"

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
            QVariant value;
            for (i = 0; i < file->messages_len; i++) {
                sml_message *message = file->messages[i];
                if (*message->message_body->tag == SML_MESSAGE_GET_LIST_RESPONSE) {
                   sml_list *entry;
                   sml_get_list_response *body;
                   body = (sml_get_list_response *) message->message_body->data;
                   for (entry = body->val_list; entry != NULL; entry = entry->next) {
                      ObisCode obis = ObisCode(entry->obj_name);
                      QString str = QString();
                      QTextStream vs(&str);
                      switch (entry->value->type) {
                         // Octet String (exact decoding depends on datapoint...)
                         case 0x00: 
                         {
                            bool doASCII = true;
                            int j;
                            
                            // check if all values are valid printable characters
                            for (j = 0; j < entry->value->data.bytes->len; j++) {
                                if (!(entry->value->data.bytes->str[j] >= char('0') && entry->value->data.bytes->str[0] <= char('z'))){
                                    doASCII = false;
                                }
                            }

                            // convert array to string
                            for (j = 0; j < entry->value->data.bytes->len; j++) {
                                if (doASCII){
                                    str.append(QChar(entry->value->data.bytes->str[j]));
                                }else{
                                    str.append(QString::number(entry->value->data.bytes->str[j], 16).rightJustified(2, QLatin1Char('0')));
                                }
                            }
                            value.setValue(str);
                            break;
                         }

                         // Boolean
                         case 0x40: value.setValue<bool>(*entry->value->data.boolean); break;

                         // Signed Integers
                         case 0x51: value.setValue<double>(*entry->value->data.int8);  break;
                         case 0x52: value.setValue<double>(*entry->value->data.int16); break;
                         case 0x54: value.setValue<double>(*entry->value->data.int32); break;
                         case 0x58: value.setValue<double>(*entry->value->data.int64); break;

                         // Unsigned Integers
                         case 0x61: value.setValue<double>(*entry->value->data.uint8);  break;
                         case 0x62: value.setValue<double>(*entry->value->data.uint16); break;
                         case 0x64: value.setValue<double>(*entry->value->data.uint32); break;
                         case 0x68: value.setValue<double>(*entry->value->data.uint64); break;

                         // Should never be reached
                         default:
                            vs << "Unkown type: " << Qt::hex << Qt::showbase << entry->value->type;
                            value.setValue(str);
                      }
                      if (value.type() == QVariant::Type::Double){ 
                        double val = value.value<double>();
                        double scaler = (entry->scaler) ? *entry->scaler : 1;
                        scaler = (scaler==-1) ? 0.0001 : scaler;
                        value.setValue<double>(val * scaler);
                      }
                      QString name = obis.toString();
                      QString code = obis.toObisString();
                      if(m_filters.contains(name) || m_filters.contains(code))
                      {
                        QVariant variant = m_filters.contains(code) ? m_filters[code]->filter(value) : m_filters[name]->filter(value);
                        if(!variant.isNull())
                        {
                            if(variant.canConvert<QVariantList>())
                            {
                                for(QVariant element : variant.toList())
                                {
                                    emit messageReceived(topicBase_ + "/" + name, element);
                                    qDebug() << "filtered" << name << element;
                                }
                            }
                            else {
                                emit messageReceived(topicBase_ + "/" + name, variant);
                                qDebug() << "filtered" << name << variant;
                            }
                        }
                      }
                      else
                      {
                          emit messageReceived(topicBase_ + "/" + name, value);
                          qDebug() << name << value;
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


/*
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
#include <QtCore>
#include "SmartMeterToMqtt.hpp"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("SmartMeterToMqtt");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption targetDirectoryOption(QStringList() << "f" << "settings-path",
                                             QCoreApplication::translate("main", "Path to the settings.ini file."),
                                             QCoreApplication::translate("main", "path"));
    parser.addOption(targetDirectoryOption);
    parser.process(app);
    bool settingsPathExists = parser.isSet(targetDirectoryOption);
    bool success {};
    if(settingsPathExists)
    {
        QString settingsPath = parser.value(targetDirectoryOption);
        SmartMeterToMqtt qSmartMeterToMqtt(settingsPath);
        success = qSmartMeterToMqtt.setup();
        if(!success) {
            qCritical() << "Failed to setup!";
            return -1;
        }
        app.exec();
    }
    else
    {
        SmartMeterToMqtt qSmartMeterToMqtt;
        success = qSmartMeterToMqtt.setup();
        if(!success) {
            qCritical() << "Failed to setup!";
            return -1;
        }
        app.exec();
    }

    return 0;
}


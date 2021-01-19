#Copyright 2021 - 2021, Andreas Oetken and the smartmetertomqtt contributors.
#
#This file is part of SmartMeterToMqtt.
#
#SmartMeterToMqtt is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.
#
#SmartMeterToMqtt is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with SmartMeterToMqtt.  If not, see <http://www.gnu.org/licenses/>.

cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0011 NEW)
CMAKE_MINIMUM_REQUIRED (VERSION 3.13)
PROJECT(SmartMeterToMqtt)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Mqtt Sql Xml XmlPatterns SerialPort Network REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(MSVC)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
ENDIF()

file(GLOB SmartMeterToMqtt_sources "${PROJECT_SOURCE_DIR}/src/**.cpp")
file(GLOB SmartMeterToMqtt_headers "${PROJECT_SOURCE_DIR}/inc/**.h*")
file(GLOB SmartMeterToMqtt_resources "${PROJECT_SOURCE_DIR}/res/**.*rc*")
set(CMAKE_AUTOUIC_SEARCH_PATHS "./res/")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

ADD_EXECUTABLE(SmartMeterToMqtt ${SmartMeterToMqtt_resources} ${SmartMeterToMqtt_sources} ${SmartMeterToMqtt_headers})

set_target_properties(SmartMeterToMqtt PROPERTIES FOLDER "apps")
target_include_directories(SmartMeterToMqtt PUBLIC "${PROJECT_SOURCE_DIR}/inc")
TARGET_LINK_LIBRARIES(SmartMeterToMqtt ${EXTRA_LIBS} Qt5::XmlPatterns Qt5::SerialPort Qt5::Mqtt Qt5::Sql Qt5::Network libmbus.so libsml.so Qt5::Xml uuid)

install(TARGETS SmartMeterToMqtt 
        RUNTIME DESTINATION bin
)

install(FILES ${PROJECT_SOURCE_DIR}/etc/smartmeter.service.default
        DESTINATION /etc/systemd/system
)

install(FILES ${PROJECT_SOURCE_DIR}/etc/smartmeter.json.default
        DESTINATION /etc
)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Enno") #required
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/package/postinst;${CMAKE_CURRENT_SOURCE_DIR}/package/preinst")
INCLUDE(CPack)

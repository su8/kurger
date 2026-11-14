#   11/19/2022
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#   MA 02110-1301, USA.

cmake_minimum_required(VERSION 3.16)

project(kurger VERSION 0.2.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        resources.qrc
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(kurger
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET kurger APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(kurger SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(kurger
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(kurger PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(MSVC)
    target_compile_options(kurger PRIVATE /W4)
else()
    target_compile_options(kurger PRIVATE -Wall -Wextra -pedantic)
endif()

set_target_properties(kurger PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.github.su8.kurger
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Command line version, it does not need Qt at all.
add_executable(kurger_cli kurger_cli.cpp)

if(MSVC)
    target_compile_options(kurger_cli PRIVATE /W4)
else()
    target_compile_options(kurger_cli PRIVATE -Wall -Wextra -pedantic)
endif()

install(TARGETS kurger kurger_cli
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(kurger)
endif()

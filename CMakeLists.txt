project (ThinkControl)
cmake_minimum_required (VERSION 2.6)

find_package (Qt4 REQUIRED)
find_package (X11 REQUIRED)
find_library (Lib_Xi NAMES Xi)

if ("${Lib_Xi}" STREQUAL "Lib_Xi-NOTFOUND")
	message (FATAL_ERROR "libXi wasn't found!")
endif ()

set (ThinkControl_SOURCES src/main.cpp src/mainwindow.cpp src/devices.cpp src/dialogs.cpp
	src/governors.cpp src/settings.cpp)
set (ThinkControl_HEADERS h/mainwindow.h h/devices.h h/dialogs.h h/governors.h
	h/settings.h)
set (ThinkControl_FORMS ui/mainwindow.ui ui/fanpreset.ui ui/profileline.ui ui/settings.ui
	ui/touchpad.ui ui/trackpoint.ui)
set (ThinkControl_RESOURCES icons.qrc)

set (CMAKE_AUTOMOC true)
set (QT_USE_QTDBUS true)

QT4_WRAP_CPP (ThinkControl_HEADERS_MOC ${ThinkControl_HEADERS})
QT4_WRAP_UI (ThinkControl_FORMS_HEADERS ${ThinkControl_FORMS})
QT4_ADD_RESOURCES (ThinkControl_RESOURCES_RCC ${ThinkControl_RESOURCES})

include (${QT_USE_FILE})

add_executable (thinkctl ${ThinkControl_SOURCES}
	${ThinkControl_HEADERS_MOC}
	${ThinkControl_FORMS_HEADERS}
	${ThinkControl_RESOURCES_RCC})
target_link_libraries (thinkctl ${QT_LIBRARIES})
target_link_libraries (thinkctl ${Lib_Xi} ${X11_LIBRARIES})
include_directories (${CMAKE_CURRENT_BINARY_DIR})
include_directories (${CMAKE_SOURCE_DIR})

install (TARGETS thinkctl DESTINATION bin)
install (FILES ${CMAKE_SOURCE_DIR}/icons/trayicon.svg
	DESTINATION share/thinkctl/icons)

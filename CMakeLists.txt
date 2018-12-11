cmake_minimum_required(VERSION 2.8)

project(substringFinder)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)

add_executable(substringFinder main.cpp
        utils/parameters.h
        mainwindow.h mainwindow.cpp
        utils/filereader.h utils/filereader.cpp
        utils/readerbuffer.h utils/readerbuffer.cpp
        utils/directoryscanner.h utils/directoryscanner.cpp)

qt5_use_modules(substringFinder Widgets)

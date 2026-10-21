cmake_minimum_required(VERSION 3.16)
project(lgl-scheduler-manager VERSION 1.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_executable(lgl-scheduler-manager
    main.cpp
    mainwindow.cpp
    mainwindow.h
    resources.qrc
)

target_link_libraries(lgl-scheduler-manager PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)

# Enable warnings. Add -Werror here once all warnings are clean.
target_compile_options(lgl-scheduler-manager PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

install(TARGETS lgl-scheduler-manager DESTINATION bin)
install(FILES lgl-scheduler-manager.desktop DESTINATION share/applications)
install(DIRECTORY packaging/hicolor
        DESTINATION share/icons
        FILES_MATCHING PATTERN "*.png")

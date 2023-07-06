cmake_minimum_required(VERSION 3.14)

project(QtXMLRpc LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Xml REQUIRED)

set(source_files QXMLRpc.cpp)
set(header_files QXMLRpc.h)

add_library(QtXMLRpc SHARED ${source_files} ${header_files})
target_link_libraries(QtXMLRpc PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Xml)

target_compile_definitions(QtXMLRpc PRIVATE QTXMLRPC_LIBRARY)

configure_file(QtXMLRpc.pc.in QtXMLRpc.pc @ONLY)

install(FILES ${header_files} DESTINATION include)
install(TARGETS QtXMLRpc LIBRARY DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/QtXMLRpc.pc DESTINATION lib/pkgconfig)

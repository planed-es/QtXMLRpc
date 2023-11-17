cmake_minimum_required(VERSION 3.14)

project(QtXMLRpc LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Xml REQUIRED)

message("Building ${PROJECT_NAME} using Qt${QT_VERSION_MAJOR}")

set(source_files QXMLRpc.cpp QXMLRpcFault.cpp)
set(header_files QXMLRpc.h)
set(TARGET_NAME "Qt${QT_VERSION_MAJOR}XMLRpc")

add_library(${TARGET_NAME} SHARED ${source_files} ${header_files})
target_link_libraries(${TARGET_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Xml)

target_compile_definitions(${TARGET_NAME} PRIVATE QTXMLRPC_LIBRARY)

configure_file(QtXMLRpc.pc.in ${TARGET_NAME}.pc @ONLY)

install(FILES ${header_files} DESTINATION include)
install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION lib)
install(FILES ${CMAKE_BINARY_DIR}/${TARGET_NAME}.pc DESTINATION lib/pkgconfig)

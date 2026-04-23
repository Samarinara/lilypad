cmake_minimum_required(VERSION 3.21)
project(Lilypad LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cef_binary")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)

add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

add_executable(lilypad
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
)

target_include_directories(lilypad PRIVATE "${CEF_INCLUDE_PATH}")

target_link_libraries(lilypad PRIVATE
    Qt6::Widgets
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
)

SET_CEF_TARGET_OUT_DIR()
set_target_properties(lilypad PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(lilypad "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(lilypad "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
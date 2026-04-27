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
    src/tab_entry.h
    src/tab_entry.cpp
    src/tab_manager.h
    src/tab_manager.cpp
    src/tab_panel.h
    src/tab_panel.cpp
    src/tab_entry_widget.h
    src/tab_entry_widget.cpp
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

# Unit tests for CefOSRWidget input/render flags (Requirements 9.1, 9.2, 9.3)
find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_cef_osr_widget
    tests/test_cef_osr_widget.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/tab_manager.h
    src/tab_manager.cpp
    src/tab_entry.h
    src/tab_entry.cpp
)
target_include_directories(test_cef_osr_widget PRIVATE "${CEF_INCLUDE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(test_cef_osr_widget PRIVATE
    Qt6::Widgets
    Qt6::Test
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
)
set_target_properties(test_cef_osr_widget PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(test_cef_osr_widget "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(test_cef_osr_widget "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# ---------------------------------------------------------------------------
# Exploratory tests for input-passthrough bug (Requirements 3.5)
# Confirms null-handler guard is preserved for key and leave events.
# Sub-tasks: 6.1, 6.2, 6.3, 6.4
# ---------------------------------------------------------------------------
add_executable(test_input_passthrough_exploratory
    tests/test_input_passthrough_exploratory.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/tab_manager.h
    src/tab_manager.cpp
    src/tab_entry.h
    src/tab_entry.cpp
)
target_include_directories(test_input_passthrough_exploratory PRIVATE "${CEF_INCLUDE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(test_input_passthrough_exploratory PRIVATE
    Qt6::Widgets
    Qt6::Test
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
)
set_target_properties(test_input_passthrough_exploratory PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(test_input_passthrough_exploratory "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(test_input_passthrough_exploratory "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# ---------------------------------------------------------------------------
# Fix-checking unit tests for input-passthrough bug (Properties 1 and 2)
# Verifies that key and leave events are forwarded to CEF on fixed code.
# Sub-tasks: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6
# ---------------------------------------------------------------------------
add_executable(test_input_passthrough_fix_checking
    tests/test_input_passthrough_fix_checking.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/tab_manager.h
    src/tab_manager.cpp
    src/tab_entry.h
    src/tab_entry.cpp
)
target_include_directories(test_input_passthrough_fix_checking PRIVATE "${CEF_INCLUDE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(test_input_passthrough_fix_checking PRIVATE
    Qt6::Widgets
    Qt6::Test
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
)
set_target_properties(test_input_passthrough_fix_checking PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(test_input_passthrough_fix_checking "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(test_input_passthrough_fix_checking "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# ---------------------------------------------------------------------------
# Preservation tests for input-passthrough bug fix (Property 3)
# Verifies that existing mouse event forwarding paths are unchanged by the fix.
# Sub-tasks: 8.1, 8.2, 8.3, 8.4, 8.5
# ---------------------------------------------------------------------------
add_executable(test_input_passthrough_preservation
    tests/test_input_passthrough_preservation.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/tab_manager.h
    src/tab_manager.cpp
    src/tab_entry.h
    src/tab_entry.cpp
)
target_include_directories(test_input_passthrough_preservation PRIVATE "${CEF_INCLUDE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(test_input_passthrough_preservation PRIVATE
    Qt6::Widgets
    Qt6::Test
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
)
set_target_properties(test_input_passthrough_preservation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(test_input_passthrough_preservation "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(test_input_passthrough_preservation "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# ---------------------------------------------------------------------------
# rapidcheck — property-based testing library
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG        master
)
set(RC_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(RC_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidcheck)

# ---------------------------------------------------------------------------
# Property-based tests for TabManager (Properties 1–6, 10–12)
# Requirements: 3.2, 3.3, 3.4, 3.5, 3.6, 4.1, 4.2, 4.5, 5.1, 5.3, 5.4, 5.5, 5.6, 9.1–9.4
# ---------------------------------------------------------------------------
add_executable(test_tab_manager_props
    tests/test_tab_manager_props.cpp
    src/tab_manager.h
    src/tab_manager.cpp
    src/tab_entry.h
    src/tab_entry.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
)
target_include_directories(test_tab_manager_props PRIVATE
    "${CEF_INCLUDE_PATH}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(test_tab_manager_props PRIVATE
    Qt6::Widgets
    Qt6::Test
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
    rapidcheck
)
set_target_properties(test_tab_manager_props PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(test_tab_manager_props "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(test_tab_manager_props "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# ---------------------------------------------------------------------------
# Unit and property-based tests for TabEntryWidget (Properties 13, 14)
# Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 7.3, 7.4
# ---------------------------------------------------------------------------
add_executable(test_tab_entry_widget
    tests/test_tab_entry_widget.cpp
    src/tab_entry_widget.h
    src/tab_entry_widget.cpp
)
target_include_directories(test_tab_entry_widget PRIVATE
    "${CEF_INCLUDE_PATH}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(test_tab_entry_widget PRIVATE
    Qt6::Widgets
    Qt6::Test
    rapidcheck
)
set_target_properties(test_tab_entry_widget PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

# ---------------------------------------------------------------------------
# Property-based tests for TabPanel (Properties 8, 12 panel side)
# Requirements: 1.1, 1.2, 1.5, 2.5, 2.7, 3.1, 5.1
# ---------------------------------------------------------------------------
add_executable(test_tab_panel_props
    tests/test_tab_panel_props.cpp
    src/tab_panel.h
    src/tab_panel.cpp
    src/tab_entry_widget.h
    src/tab_entry_widget.cpp
    src/tab_manager.h
    src/tab_manager.cpp
    src/tab_entry.h
    src/tab_entry.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
)
target_include_directories(test_tab_panel_props PRIVATE
    "${CEF_INCLUDE_PATH}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(test_tab_panel_props PRIVATE
    Qt6::Widgets
    Qt6::Test
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
    rapidcheck
)
set_target_properties(test_tab_panel_props PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(test_tab_panel_props "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(test_tab_panel_props "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# ---------------------------------------------------------------------------
# Property-based tests for MainWindow (Properties 7, 9, 15)
# Requirements: 1.1, 1.4, 4.3, 6.1, 6.3, 6.4, 10.1, 10.2, 10.3
# ---------------------------------------------------------------------------
add_executable(test_mainwindow_props
    tests/test_mainwindow_props.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/tab_manager.h
    src/tab_manager.cpp
    src/tab_entry.h
    src/tab_entry.cpp
    src/tab_panel.h
    src/tab_panel.cpp
    src/tab_entry_widget.h
    src/tab_entry_widget.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
)
target_include_directories(test_mainwindow_props PRIVATE
    "${CEF_INCLUDE_PATH}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(test_mainwindow_props PRIVATE
    Qt6::Widgets
    Qt6::Test
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
    rapidcheck
)
set_target_properties(test_mainwindow_props PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(test_mainwindow_props "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(test_mainwindow_props "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

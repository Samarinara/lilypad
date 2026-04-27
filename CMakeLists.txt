cmake_minimum_required(VERSION 3.21)
project(Lilypad LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Test)

# Main application
add_executable(lilypad
    src/main.cpp
    src/mainwindow.cpp
    src/tab_entry.cpp
    src/tab_manager.cpp
    src/tab_panel.cpp
    src/tab_entry_widget.cpp
)

target_link_libraries(lilypad PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
)

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
# Unit tests for TabEntryWidget
# ---------------------------------------------------------------------------
add_executable(test_tab_entry_widget
    tests/test_tab_entry_widget.cpp
    src/tab_entry_widget.cpp
    src/tab_entry_widget.h
)

target_include_directories(test_tab_entry_widget PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/rapidcheck-src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(test_tab_entry_widget PRIVATE
    Qt6::Widgets
    Qt6::Test
    rapidcheck
)

# ---------------------------------------------------------------------------
# Property-based tests for TabManager
# ---------------------------------------------------------------------------
add_executable(test_tab_manager_props
    tests/test_tab_manager_props.cpp
    src/tab_manager.cpp
    src/tab_manager.h
    src/tab_entry.cpp
    src/tab_entry.h
)

target_include_directories(test_tab_manager_props PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/rapidcheck-src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(test_tab_manager_props PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Test
    rapidcheck
)

# ---------------------------------------------------------------------------
# Property-based tests for TabPanel
# ---------------------------------------------------------------------------
add_executable(test_tab_panel_props
    tests/test_tab_panel_props.cpp
    src/tab_panel.cpp
    src/tab_panel.h
    src/tab_entry_widget.cpp
    src/tab_entry_widget.h
    src/tab_manager.cpp
    src/tab_manager.h
    src/tab_entry.cpp
    src/tab_entry.h
)

target_include_directories(test_tab_panel_props PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/rapidcheck-src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(test_tab_panel_props PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Test
    rapidcheck
)

# ---------------------------------------------------------------------------
# Property-based tests for MainWindow
# ---------------------------------------------------------------------------
add_executable(test_mainwindow_props
    tests/test_mainwindow_props.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/tab_manager.cpp
    src/tab_manager.h
    src/tab_entry.cpp
    src/tab_entry.h
    src/tab_panel.cpp
    src/tab_panel.h
    src/tab_entry_widget.cpp
    src/tab_entry_widget.h
)

target_include_directories(test_mainwindow_props PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/rapidcheck-src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(test_mainwindow_props PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Test
    rapidcheck
)

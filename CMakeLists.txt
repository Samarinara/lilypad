cmake_minimum_required(VERSION 3.21)
project(Lilypad LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick WebEngineQuick Sql QuickControls2)

# QML source files - these will be set as dependencies for the QRC
set(QML_FILES
    qml/Main.qml
    qml/TabPanel.qml
    qml/TabEntry.qml
    qml/UrlBar.qml
    qml/SearchButton.qml
    qml/UrlBarContainer.qml
    qml/HamburgerMenu.qml
    qml/LilypadTheme/Theme.qml
    qml/LilypadTheme/qmldir
    qml/qmldir
    qml/test.qml
)

# Main application
add_executable(lilypad
    src/main.cpp
    src/urlprocessor.cpp
    src/tab_entry.cpp
    src/tab_manager.cpp
    resources.qrc
)

# Set QRC dependencies so CMake re-runs rcc when QML files change
set_property(TARGET lilypad APPEND
    PROPERTY AUTORCC_DEPENDS ${QML_FILES}
)

# Copy bangs.db to build directory for runtime access
file(COPY ${CMAKE_SOURCE_DIR}/bangs.db
     DESTINATION ${CMAKE_BINARY_DIR})

target_link_libraries(lilypad PRIVATE
    Qt6::Quick
    Qt6::WebEngineQuick
    Qt6::Sql
    Qt6::QuickControls2
)

# ============================================================================
# Tests — browser-performance bugfix spec (Tasks 9-12)
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS Test)

enable_testing()

add_executable(test_browser_performance
    tests/test_browser_performance.cpp
    src/tab_entry.cpp
    src/tab_manager.cpp
    src/urlprocessor.cpp
)

target_include_directories(test_browser_performance PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_browser_performance PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Sql
    Qt6::Gui
)

# Copy bangs.db next to the test binary so UrlProcessor can find it
add_custom_command(TARGET test_browser_performance POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/bangs.db
        $<TARGET_FILE_DIR:test_browser_performance>/bangs.db
)

add_test(NAME test_browser_performance
    COMMAND test_browser_performance
    WORKING_DIRECTORY $<TARGET_FILE_DIR:test_browser_performance>
)

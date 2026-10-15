cmake_minimum_required(VERSION 3.21)

project(SignalForge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIGNALFORGE_BUILD_GUI "Build the Qt 6 desktop application" ON)

add_library(signalforge_core STATIC src/core/Frequency.cpp src/core/WebMercator.cpp
    src/core/RtlDiscovery.cpp)
target_link_libraries(signalforge_core PRIVATE ${CMAKE_DL_LIBS})
target_include_directories(signalforge_core PUBLIC src)
target_compile_features(signalforge_core PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(signalforge_core PRIVATE /W4)
else()
    target_compile_options(signalforge_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(signalforge_core_tests tests/FrequencyTests.cpp)
    target_link_libraries(signalforge_core_tests PRIVATE signalforge_core)
    add_test(NAME signalforge_core_tests COMMAND signalforge_core_tests)
    add_executable(signalforge_map_projection_tests tests/MapProjectionTests.cpp)
    target_link_libraries(signalforge_map_projection_tests PRIVATE signalforge_core)
    add_test(NAME signalforge_map_projection_tests COMMAND signalforge_map_projection_tests)
    add_library(signalforge_mock_rtlsdr SHARED tests/MockRtlSdr.cpp)
    add_executable(signalforge_rtl_discovery_tests tests/RtlDiscoveryTests.cpp)
    target_link_libraries(signalforge_rtl_discovery_tests PRIVATE signalforge_core)
    add_test(NAME signalforge_rtl_discovery_tests
             COMMAND signalforge_rtl_discovery_tests
             $<TARGET_FILE:signalforge_mock_rtlsdr>)
endif()

if(SIGNALFORGE_BUILD_GUI)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Sql)
    add_executable(SignalForge WIN32
        src/app/main.cpp
        src/ui/MainWindow.h
        src/ui/MainWindow.cpp
        src/ui/SpectrumWidget.h
        src/ui/SpectrumWidget.cpp
        src/ui/DemoMapWidget.h
        src/ui/DemoMapWidget.cpp
        src/ui/OfflineMapWidget.h
        src/ui/OfflineMapWidget.cpp
        src/ui/ReceiverWidget.h
        src/ui/ReceiverWidget.cpp
    )
    target_link_libraries(SignalForge PRIVATE signalforge_core Qt6::Widgets Qt6::Sql)
    target_include_directories(SignalForge PRIVATE src)
    set_target_properties(SignalForge PROPERTIES MACOSX_BUNDLE TRUE)
endif()

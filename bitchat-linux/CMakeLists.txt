cmake_minimum_required(VERSION 3.16)
project(bitchat-linux VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 configuration
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Bluetooth)

# Enable Qt6 MOC, RCC, and UIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(DBUS REQUIRED dbus-1)

# Find zlib for compression
find_package(ZLIB REQUIRED)

# Services
set(SERVICES_SOURCES
    Services/BluetoothMeshService.cpp
    Services/MessageRetryService.cpp
    Services/DeliveryTracker.cpp
    Services/MessageRetentionService.cpp
    Services/KeychainManager.cpp
    Services/NotificationService.cpp
    Services/EncryptionService.cpp
)

set(SERVICES_HEADERS
    Services/BluetoothMeshService.h
    Services/MessageRetryService.h
    Services/DeliveryTracker.h
    Services/MessageRetentionService.h
    Services/KeychainManager.h
    Services/NotificationService.h
    Services/EncryptionService.h
)

# Utils
set(UTILS_SOURCES
    Utils/CompressionUtil.cpp
    Utils/MessagePadding.cpp
    Utils/BatteryOptimizer.cpp
    Utils/OptimizedBloomFilter.cpp
)

set(UTILS_HEADERS
    Utils/CompressionUtil.h
    Utils/MessagePadding.h
    Utils/OptimizedBloomFilter.h
    Utils/BatteryOptimizer.h
)

# Protocols
set(PROTOCOLS_SOURCES
    Protocols/BinaryProtocol.cpp
)

set(PROTOCOLS_HEADERS
    Protocols/BinaryProtocol.h
)

# ViewModels
set(VIEWMODELS_SOURCES
    ViewModels/ConfigManager.cpp
    ViewModels/MessageHandler.cpp
)

set(VIEWMODELS_HEADERS
    ViewModels/ConfigManager.h
    ViewModels/MessageHandler.h
)

# Views (UI)
file(GLOB_RECURSE VIEWS_SOURCES "Views/ui/*.cpp")
file(GLOB_RECURSE VIEWS_HEADERS "Views/ui/*.h")

# Main application
set(MAIN_SOURCES
    main.cpp
    BitchatApplication.cpp
)

set(MAIN_HEADERS
    BitchatApplication.h
)

# Combine all sources
set(ALL_SOURCES
    ${SERVICES_SOURCES}
    ${UTILS_SOURCES}
    ${PROTOCOLS_SOURCES}
    ${VIEWMODELS_SOURCES}
    ${VIEWS_SOURCES}
    ${MAIN_SOURCES}
)

set(ALL_HEADERS
    ${SERVICES_HEADERS}
    ${UTILS_HEADERS}
    ${PROTOCOLS_HEADERS}
    ${VIEWMODELS_HEADERS}
    ${VIEWS_HEADERS}
    ${MAIN_HEADERS}
)

# Create the executable
add_executable(bitchat-linux ${ALL_SOURCES} ${ALL_HEADERS})

# Link libraries
target_link_libraries(bitchat-linux
    Qt6::Core
    Qt6::Widgets
    Qt6::Bluetooth
    ${GLIB_LIBRARIES}
    ${DBUS_LIBRARIES}
    cap
    sodium
    ZLIB::ZLIB
)

# Include directories
target_include_directories(bitchat-linux PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Services
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/Protocols
    ${CMAKE_CURRENT_SOURCE_DIR}/ViewModels
    ${CMAKE_CURRENT_SOURCE_DIR}/Views
    ${CMAKE_CURRENT_SOURCE_DIR}/Views/ui
    ${GLIB_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
)

# Compiler flags
target_compile_options(bitchat-linux PRIVATE
    ${GLIB_CFLAGS_OTHER}
    ${DBUS_CFLAGS_OTHER}
)

# Install target
install(TARGETS bitchat-linux DESTINATION bin)

# Install desktop file and icon if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/bitchat.desktop")
    install(FILES Assets/bitchat.desktop DESTINATION share/applications)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/bitchat.png")
    install(FILES Assets/bitchat.png DESTINATION share/icons/hicolor/256x256/apps)
endif()

# Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(bitchat-linux PRIVATE DEBUG)
endif()

# Optimization for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(bitchat-linux PRIVATE -O3)
endif()

# Set executable name to match project
set_target_properties(bitchat-linux PROPERTIES OUTPUT_NAME "bitchat-linux") 
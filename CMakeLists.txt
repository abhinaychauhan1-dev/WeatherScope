# -----------------------------------------------------------------------------
# @file       : CMakeLists.txt
# @brief      : Configures the WeatherScope Qt application and its dependencies.
# @target     : weather-globe-qt
# @cmake      : 3.21 or newer
# @cpp        : C++20, required without compiler-specific language extensions
# @details    : Builds the native weather-globe-qt executable and attaches the
#               WeatherGlobe QML module, embedded visual resources, generated
#               translation catalogs, and platform installation rules to it.
# @author     : Abhinay Chauhan (email: chauhan089306@gmail.com)
# @version    : 1.0.0
#
# Copyright (c) 2024
# Abhinay Chauhan. All rights reserved.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Project Metadata & C++ Language Standard
# -----------------------------------------------------------------------------
# Keep this baseline aligned with the oldest supported CMake deployment.
cmake_minimum_required(VERSION 3.21)

project(weather-globe-qt VERSION 0.1.0 LANGUAGES CXX)

# Require portable ISO C++20 behavior and reject fallback to an older standard
# or compiler-specific GNU language extensions.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Qt Module Dependency Discovery
# -----------------------------------------------------------------------------
# Qt 6.8 is the application baseline. REQUIRED fails configuration early when
# a runtime-critical networking, QML, Quick 3D, SQL, or translation module is absent.
find_package(Qt6 6.8 REQUIRED COMPONENTS
    Concurrent
    Core
    Gui
    LinguistTools
    Network
    Quick
    Quick3D
    Quick3DParticles
    Sql
)

# Apply Qt's standard AUTOMOC, AUTORCC, QML, and installation defaults for 6.8.
qt_standard_project_setup(REQUIRES 6.8)

# -----------------------------------------------------------------------------
# Source and Localization Manifests
# -----------------------------------------------------------------------------
# Keep translation catalogs explicit so every supported locale is compiled and embedded.
set(TS_FILES
    translations/weather-globe-qt_en.ts
    translations/weather-globe-qt_de.ts
    translations/weather-globe-qt_fr.ts
    translations/weather-globe-qt_zh_CN.ts
    translations/weather-globe-qt_nl.ts
    translations/weather-globe-qt_nb.ts
    translations/weather-globe-qt_sv.ts
    translations/weather-globe-qt_ja.ts
    translations/weather-globe-qt_ko.ts
    translations/weather-globe-qt_es.ts
    translations/weather-globe-qt_it.ts
)

# Model headers are listed with implementations so IDEs, AUTOMOC, and generated
# build metadata consistently discover QObject-based service implementations.
set(MODEL_SOURCES
    include/model/geocoding_client.h
    include/model/i_geocoding_service.h
    include/model/i_localization_service.h
    include/model/i_network_reachability_service.h
    include/model/i_weather_client.h
    include/model/i_weather_repository.h
    include/model/localization_manager.h
    include/model/network_reachability_service.h
    include/model/network_weather_client.h
    include/model/sqlite_weather_store.h
    include/model/weather_repository.h
    include/model/weather_telemetry.h
    include/model/weather_telemetry_parser.h
    src/model/geocoding_client.cpp
    src/model/localization_manager.cpp
    src/model/network_reachability_service.cpp
    src/model/network_weather_client.cpp
    src/model/sqlite_weather_store.cpp
    src/model/weather_repository.cpp
    src/model/weather_telemetry_parser.cpp
)

# ViewModel sources are registered with the QML module below so their Qt type
# metadata is available to generated QML registration code.
set(VIEWMODEL_SOURCES
    include/viewmodel/application_view_model.h
    include/viewmodel/daily_forecast_view_model.h
    include/viewmodel/hourly_forecast_view_model.h
    include/viewmodel/weather_main_view_model.h
    include/viewmodel/weather_presentation_formatter.h
    include/viewmodel/weather_shader_params.h
    src/viewmodel/application_view_model.cpp
    src/viewmodel/daily_forecast_view_model.cpp
    src/viewmodel/hourly_forecast_view_model.cpp
    src/viewmodel/weather_main_view_model.cpp
    src/viewmodel/weather_shader_params.cpp
)

# -----------------------------------------------------------------------------
# Core Executable Target Definition
# -----------------------------------------------------------------------------
# The executable owns the application entry point and native Model layer; QML
# and ViewModel sources are attached by qt_add_qml_module in the next section.
qt_add_executable(weather-globe-qt
    src/main.cpp
    ${MODEL_SOURCES}
)

# -----------------------------------------------------------------------------
# QML Module Registration & Resource Aliases
# -----------------------------------------------------------------------------
# Strip repository-only directory prefixes from embedded URLs. These stable
# aliases match the module-relative paths referenced by QML at runtime.
set_source_files_properties(presentation/views/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(presentation/views/WeatherHud.qml PROPERTIES QT_RESOURCE_ALIAS WeatherHud.qml)
set_source_files_properties(presentation/scenes/GlobeView3D.qml PROPERTIES QT_RESOURCE_ALIAS GlobeView3D.qml)
set_source_files_properties(
    presentation/shaders/EarthWeatherMaterial.qml
    PROPERTIES QT_RESOURCE_ALIAS EarthWeatherMaterial.qml
)
set_source_files_properties(
    presentation/views/HourlyCardDelegate.qml
    PROPERTIES QT_RESOURCE_ALIAS HourlyCardDelegate.qml
)
set_source_files_properties(
    presentation/views/DailyCardDelegate.qml
    PROPERTIES QT_RESOURCE_ALIAS DailyCardDelegate.qml
)
set_source_files_properties(
    presentation/views/WeatherConditionIcon.qml
    PROPERTIES QT_RESOURCE_ALIAS WeatherConditionIcon.qml
)
set_source_files_properties(
    presentation/assets/earth/earth-day.jpg
    PROPERTIES QT_RESOURCE_ALIAS assets/earth/earth-day.jpg
)
set_source_files_properties(
    presentation/assets/earth/earth-night.png
    PROPERTIES QT_RESOURCE_ALIAS assets/earth/earth-night.png
)
set_source_files_properties(
    presentation/assets/earth/earth-roughness.jpg
    PROPERTIES QT_RESOURCE_ALIAS assets/earth/earth-roughness.jpg
)
set_source_files_properties(
    presentation/assets/earth/earth-clouds.png
    PROPERTIES QT_RESOURCE_ALIAS assets/earth/earth-clouds.png
)
set_source_files_properties(
    presentation/assets/markers/location-pin.svg
    PROPERTIES QT_RESOURCE_ALIAS assets/markers/location-pin.svg
)
set(WEATHER_ICON_RESOURCES
    presentation/assets/weather/clear.svg
    presentation/assets/weather/partly-cloudy.svg
    presentation/assets/weather/cloudy.svg
    presentation/assets/weather/fog.svg
    presentation/assets/weather/rain.svg
    presentation/assets/weather/showers.svg
    presentation/assets/weather/snow.svg
    presentation/assets/weather/storm.svg
)
foreach(weather_icon IN LISTS WEATHER_ICON_RESOURCES)
    get_filename_component(weather_icon_name "${weather_icon}" NAME)
    set_source_files_properties(
        "${weather_icon}"
        PROPERTIES QT_RESOURCE_ALIAS "assets/weather/${weather_icon_name}"
    )
endforeach()
set_source_files_properties(
    presentation/shaders/earth_weather.vert
    PROPERTIES QT_RESOURCE_ALIAS shaders/earth_weather.vert
)
set_source_files_properties(
    presentation/shaders/earth_weather.frag
    PROPERTIES QT_RESOURCE_ALIAS shaders/earth_weather.frag
)

# Register the executable-backed WeatherGlobe 1.0 module and embed all QML,
# texture, marker, icon, and shader resources into the application target.
qt_add_qml_module(weather-globe-qt
    URI WeatherGlobe
    VERSION 1.0
    DEPENDENCIES
        QtQuick
    SOURCES
        ${VIEWMODEL_SOURCES}
    QML_FILES
        presentation/views/Main.qml
        presentation/views/WeatherHud.qml
        presentation/scenes/GlobeView3D.qml
        presentation/views/DailyCardDelegate.qml
        presentation/views/HourlyCardDelegate.qml
        presentation/views/WeatherConditionIcon.qml
        presentation/shaders/EarthWeatherMaterial.qml
    RESOURCES
        presentation/assets/earth/earth-day.jpg
        presentation/assets/earth/earth-night.png
        presentation/assets/earth/earth-roughness.jpg
        presentation/assets/earth/earth-clouds.png
        presentation/assets/markers/location-pin.svg
        ${WEATHER_ICON_RESOURCES}
        presentation/shaders/earth_weather.vert
        presentation/shaders/earth_weather.frag
)

# -----------------------------------------------------------------------------
# Target Includes, Compile Definitions & Link Libraries
# -----------------------------------------------------------------------------
# PRIVATE visibility keeps project headers internal to the executable. The root
# supports qualified model/... and viewmodel/... includes; the ViewModel path
# also supports Qt-generated integration sources that include those headers directly.
target_include_directories(weather-globe-qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/viewmodel
)

# Link only the Qt modules used by native code or QML resources. PRIVATE avoids
# exporting Qt linkage requirements because this target is an executable.
target_link_libraries(weather-globe-qt PRIVATE
    Qt6::Concurrent
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Quick
    Qt6::Quick3D
    Qt6::Quick3DParticles
    Qt6::Sql
)

# No target_compile_definitions are required; runtime behavior is configured
# through typed APIs and embedded resources rather than preprocessor switches.

# -----------------------------------------------------------------------------
# Project Metadata & Compiler Warning Flags
# -----------------------------------------------------------------------------
# Apply equivalent strict warning policies per compiler family. Generator
# expressions prevent one toolchain from receiving another toolchain's switches.
target_compile_options(weather-globe-qt PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        # Broad diagnostics plus conversion, shadowing, polymorphic-destruction,
        # and legacy-cast checks support the project's defensive C++ policy.
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Werror
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        # /permissive- tightens standards conformance; /WX matches -Werror.
        /W4
        /WX
        /permissive-
    >
)

# Build a GUI subsystem executable on Windows and an application bundle on
# macOS. Indirect QML cache calls preserve compatibility with dynamic bindings.
set_target_properties(weather-globe-qt PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    QT_QMLCACHEGEN_DIRECT_CALLS OFF
)

# -----------------------------------------------------------------------------
# Localization & Translation Setup
# -----------------------------------------------------------------------------
# Compile TS catalogs to QM resources under :/i18n, the path consumed directly
# by LocalizationManager when switching the application language at runtime.
qt_add_translations(weather-globe-qt
    TS_FILES ${TS_FILES}
    RESOURCE_PREFIX "/i18n"
)

# -----------------------------------------------------------------------------
# Installation & Deployment Rules
# -----------------------------------------------------------------------------
# GNUInstallDirs supplies platform-appropriate runtime and library destinations;
# bundles remain self-contained at the selected installation prefix.
include(GNUInstallDirs)
install(TARGETS weather-globe-qt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET weather-globe-qt
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
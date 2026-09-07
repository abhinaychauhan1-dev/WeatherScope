/**
 * @file     : main.cpp
 * @brief    : Composes application dependencies and starts the Qt GUI.
 * @details  : Defines the standalone process entry point; no associated header
 * is required. It constructs Model, Service, and ViewModel dependencies,
 * injects the root ViewModel into QML, validates startup, and owns the GUI event loop.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

#include "model/geocoding_client.h"
#include "model/localization_manager.h"
#include "model/network_reachability_service.h"
#include "model/network_weather_client.h"
#include "model/weather_repository.h"
#include "viewmodel/weather_main_view_model.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QList>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQmlError>
#include <QStandardPaths>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <memory>

/**
 * @brief Initializes WeatherScope and runs the Qt GUI event loop.
 * @param[in] argc Number of process command-line arguments.
 * @param[in] argv Array of mutable argument strings supplied by the runtime.
 * @return EXIT_SUCCESS after a clean event-loop exit; EXIT_FAILURE when storage,
 * dependency construction, QML loading, runtime warnings, or exceptions fail.
 * @pre argv addresses at least argc entries according to the C++ runtime contract.
 * @post Stack-owned Qt objects and injected services are destroyed in reverse
 * construction order after the event loop exits or initialization fails.
 */
int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("WeatherGlobe"));
    QCoreApplication::setApplicationName(QStringLiteral("weather-globe-qt"));

    try {
        const QString applicationDataPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir applicationDataDirectory{};
        // Persistence is mandatory for offline fallback; fail before constructing
        // services if Qt cannot provide or create an application-writable directory.
        if (applicationDataPath.isEmpty()
            || !applicationDataDirectory.mkpath(applicationDataPath)) {
            qCritical() << "Unable to create the application data directory.";
            return EXIT_FAILURE;
        }

        const QString databasePath =
            QDir{applicationDataPath}.filePath(QStringLiteral("weather-cache.sqlite"));
        const auto geocodingService = std::make_shared<GeocodingClient>();
        const auto reachabilityService = std::make_shared<NetworkReachabilityService>();
        const auto weatherClient = std::make_shared<NetworkWeatherClient>();
        // Shared ownership keeps injected services alive through all callbacks;
        // stack declaration order ensures the ViewModel is destroyed before them.
        const auto weatherRepository = std::make_shared<WeatherRepository>(
            *weatherClient,
            *reachabilityService,
            databasePath);

        LocalizationManager localizationManager{};
        WeatherMainViewModel weatherMainViewModel{
            weatherRepository,
            reachabilityService,
            geocodingService,
            localizationManager};
        // The engine is constructed last so every QML root is destroyed before
        // its borrowed ViewModel pointer and localization service.
        QQmlApplicationEngine engine;
        QObject::connect(
            &localizationManager,
            &ILocalizationService::languageChanged,
            &engine,
            &QQmlEngine::retranslate);
        // Translation failure is recoverable because source strings remain usable.
        if (!localizationManager.switchLanguage(AppLanguage::English)) {
            qWarning() << "Unable to load the default application translation catalog.";
        }
        bool qmlWarningDetected{false};
        bool objectCreationFailed{false};

        const QMetaObject::Connection startupWarningConnection = QObject::connect(
            &engine,
            &QQmlEngine::warnings,
            &application,
            [&application, &qmlWarningDetected](const QList<QQmlError> &warnings) {
                qmlWarningDetected = true;
                for (const QQmlError &warning : warnings) {
                    qCritical().noquote() << warning.toString();
                }
                // Defer exit until the current signal delivery unwinds; terminating
                // the event loop synchronously from an engine callback is unsafe.
                QTimer::singleShot(
                    0,
                    &application,
                    []() { QCoreApplication::exit(EXIT_FAILURE); });
            });
        QObject::connect(
            &engine,
            &QQmlApplicationEngine::objectCreationFailed,
            &application,
            [&objectCreationFailed](const QUrl &) { objectCreationFailed = true; });

        engine.setInitialProperties(QVariantMap{
            {QStringLiteral("viewModel"), QVariant::fromValue(&weatherMainViewModel)}});
        // Main.qml requires the injected ViewModel and cannot construct one itself.
        engine.loadFromModule("WeatherGlobe", "Main");

        const QList<QObject *> rootObjects = engine.rootObjects();
        const bool hasNullRoot = std::any_of(
            rootObjects.cbegin(),
            rootObjects.cend(),
            [](const QObject *rootObject) { return rootObject == nullptr; });
        // Treat warnings, explicit creation failure, and malformed root lists as
        // independent fatal startup gates before entering the long-lived event loop.
        if (qmlWarningDetected || objectCreationFailed || rootObjects.isEmpty() || hasNullRoot) {
            return EXIT_FAILURE;
        }

        // Runtime warnings are recoverable in production; only startup warnings
        // invalidate the object graph assembled above.
        QObject::disconnect(startupWarningConnection);
        QObject::connect(
            &engine,
            &QQmlEngine::warnings,
            &application,
            [](const QList<QQmlError> &warnings) {
                for (const QQmlError &warning : warnings) {
                    qWarning().noquote() << warning.toString();
                }
            });

        const std::int32_t eventLoopResult = static_cast<std::int32_t>(application.exec());
        // Normalize Qt's platform-specific non-zero exit codes to the process contract.
        return (eventLoopResult == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const std::exception &exception) {
        // Initialization exceptions are fatal but retain their diagnostic message.
        qCritical() << "Application initialization failed:" << exception.what();
        return EXIT_FAILURE;
    } catch (...) {
        // Preserve the no-exception process boundary even for non-standard failures.
        qCritical() << "Application initialization failed with an unknown error.";
        return EXIT_FAILURE;
    }
}
/**
 * @file     : weather_repository.cpp
 * @brief    : Coordinates remote, cached, and network-aware weather access.
 * @details  : Provides the definitions declared in weather_repository.h,
 * dispatching parse/cache work to QtConcurrent workers and returning completed
 * forecast snapshots to the repository's QObject affinity thread.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "model/weather_repository.h"

#include "model/weather_telemetry_parser.h"

#include <QMetaObject>
#include <QtConcurrentRun>

#include <stdexcept>
#include <utility>

namespace
{
/**
 * @brief Transfers either a forecast or an error from a worker to the repository.
 * @note Immutable members make each result a self-contained cross-thread value.
 */
struct RepositoryWorkerResult final
{
    const std::optional<WeatherForecastData> forecast{};
    const QString error{};
};
} // namespace

/**
 * @brief Wires borrowed services to repository processing and reachability policy.
 * @param[in] weatherClient Borrowed transport client.
 * @param[in] reachabilityService Borrowed connectivity service.
 * @param[in] databasePath Path used by the shared cache store.
 * @param[in] parent Optional QObject lifecycle owner.
 * @pre Collaborators outlive this repository; construction occurs on its target thread.
 * @post Payload/error handlers and a reachability subscription are installed.
 */
WeatherRepository::WeatherRepository(
    IWeatherClient &weatherClient,
    INetworkReachabilityService &reachabilityService,
    QString databasePath,
    QObject *parent)
    : QObject(parent)
    , weatherClient_(weatherClient)
    , reachabilityService_(reachabilityService)
    , store_(std::make_shared<SqliteWeatherStore>(std::move(databasePath)))
{
    try {
        weatherClient_.setWeatherDataHandler(
            [this](const QByteArray &payload) { processNetworkPayload(payload); });
        weatherClient_.setErrorHandler([this](const QString &message) {
            reportError(message);
            if (pollingRequested_) {
                loadCachedPayload();
            }
        });
        const auto subscription = reachabilityService_.addReachabilityChangedHandler(
            [this](const bool isReachable) {
                QMetaObject::invokeMethod(
                    this,
                    [this, isReachable]() { handleReachabilityChanged(isReachable); },
                    Qt::QueuedConnection);
            });
        if (subscription == 0U) {
            throw std::runtime_error{"Unable to subscribe to network reachability."};
        }
        reachabilitySubscription_ = subscription;
    } catch (...) {
        weatherClient_.setWeatherDataHandler({});
        weatherClient_.setErrorHandler({});
        throw;
    }
}

/**
 * @brief Detaches callbacks and reachability observation before state destruction.
 * @pre Called on the repository affinity thread while borrowed services are alive.
 * @post Borrowed services no longer intentionally invoke this repository.
 */
WeatherRepository::~WeatherRepository()
{
    weatherClient_.setWeatherDataHandler({});
    weatherClient_.setErrorHandler({});
    if (reachabilitySubscription_.has_value()) {
        reachabilityService_.removeReachabilityChangedHandler(
            reachabilitySubscription_.value());
    }
}

/**
 * @brief Records polling intent and selects online retrieval or offline fallback.
 * @param[in] latitude Latitude forwarded to the transport client.
 * @param[in] longitude Longitude forwarded to the transport client.
 * @param[in] intervalMilliseconds Polling cadence in milliseconds.
 * @pre Called on the affinity thread; the weather client validates input ranges.
 * @post Polling intent is retained across later reachability transitions.
 */
void WeatherRepository::startPolling(
    const double latitude,
    const double longitude,
    const std::int32_t intervalMilliseconds)
{
    latitude_ = latitude;
    longitude_ = longitude;
    intervalMilliseconds_ = intervalMilliseconds;
    pollingRequested_ = true;

    // Reachability selects exactly one source path: live polling or latest cache.
    if (reachabilityService_.isReachable()) {
        weatherClient_.startPolling(latitude_, longitude_, intervalMilliseconds_);
    } else {
        weatherClient_.stopPolling();
        loadCachedPayload();
    }
}

/** @brief Clears polling intent and stops live retrieval. @pre Affinity-thread access. @post Reconnection will not restart polling until requested again. @note Non-throwing. */
void WeatherRepository::stopPolling() noexcept
{
    pollingRequested_ = false;
    weatherClient_.stopPolling();
}

/** @brief Forwards a locale to future network requests. @param[in] languageCode ISO language code. @pre Affinity-thread access. @post The transport owns the normalized locale policy. */
void WeatherRepository::setLanguageCode(const QString &languageCode)
{
    weatherClient_.setLanguageCode(languageCode);
}

/** @brief Installs the forecast consumer. @param[in] handler Replacement callback or empty callable. @pre Affinity-thread access. @post The previous callback is released. */
void WeatherRepository::setForecastHandler(ForecastHandler handler) noexcept
{
    forecastHandler_ = std::move(handler);
}

/** @brief Installs the error consumer. @param[in] handler Replacement callback or empty callable. @pre Affinity-thread access. @post The previous callback is released. */
void WeatherRepository::setErrorHandler(ErrorHandler handler) noexcept
{
    errorHandler_ = std::move(handler);
}

/**
 * @brief Applies connectivity changes to a previously requested polling lifecycle.
 * @param[in] isReachable Current network state.
 * @pre Executed on the affinity thread through the constructor's queued hop.
 * @post Active source matches reachability when polling remains requested.
 */
void WeatherRepository::handleReachabilityChanged(const bool isReachable)
{
    // Connectivity alone must not start work after the consumer explicitly stopped polling.
    if (!pollingRequested_) {
        return;
    }

    if (isReachable) {
        weatherClient_.startPolling(latitude_, longitude_, intervalMilliseconds_);
    } else {
        weatherClient_.stopPolling();
        loadCachedPayload();
    }
}

/**
 * @brief Parses and caches a fresh network payload on a worker thread.
 * @param[in] payload Serialized response copied into the worker closure.
 * @pre Called on the affinity thread with a complete payload.
 * @post A valid forecast is delivered as non-cached; parse/cache errors are reported.
 */
void WeatherRepository::processNetworkPayload(const QByteArray &payload)
{
    // Shared ownership keeps the store alive if repository destruction races worker completion.
    const std::shared_ptr<SqliteWeatherStore> store = store_;
    static_cast<void>(QtConcurrent::run([store, payload]() {
        // Parsing and disk I/O remain off the UI thread; invalid payloads are never cached.
        const WeatherTelemetryParseResult parsed = WeatherTelemetryParser::parse(payload);
        if (!parsed.forecast.has_value()) {
            return RepositoryWorkerResult{{}, parsed.error};
        }

        const QString cacheError = store->save(payload);
        return RepositoryWorkerResult{parsed.forecast, cacheError};
    }).then(this, [this](const RepositoryWorkerResult &result) {
        // QObject context schedules continuation delivery on the repository thread
        // and suppresses invocation after the context object is destroyed.
        if (result.forecast.has_value() && forecastHandler_) {
            forecastHandler_(result.forecast.value(), false);
        }
        if (!result.error.isEmpty()) {
            reportError(result.error);
        }
    }));
}

/**
 * @brief Loads and parses the latest cache snapshot on a worker thread.
 * @pre Called on the affinity thread when live data should not be used.
 * @post A valid forecast is delivered with the cached flag; absence/failure is reported.
 */
void WeatherRepository::loadCachedPayload()
{
    const std::shared_ptr<SqliteWeatherStore> store = store_;
    static_cast<void>(QtConcurrent::run([store]() {
        // Database access and parsing share the worker so no raw payload crosses back to UI.
        const CachedWeatherPayloadResult cached = store->loadLatest();
        if (!cached.error.isEmpty()) {
            return RepositoryWorkerResult{{}, cached.error};
        }
        if (!cached.payload.has_value()) {
            // Empty cache is operationally distinct from a database error but still
            // leaves the consumer without a usable forecast.
            return RepositoryWorkerResult{{}, QStringLiteral("No cached weather payload is available.")};
        }

        const WeatherTelemetryParseResult parsed = WeatherTelemetryParser::parse(cached.payload.value());
        return RepositoryWorkerResult{parsed.forecast, parsed.error};
    }).then(this, [this](const RepositoryWorkerResult &result) {
        // Context-bound continuation restores QObject thread affinity and lifetime safety.
        if (result.forecast.has_value() && forecastHandler_) {
            forecastHandler_(result.forecast.value(), true);
        }
        if (!result.error.isEmpty()) {
            reportError(result.error);
        }
    }));
}

/** @brief Forwards an error when a consumer is installed. @param[in] message Human-readable failure. @pre Affinity-thread access. @post The callback has run at most once. */
void WeatherRepository::reportError(const QString &message) const
{
    if (errorHandler_) {
        errorHandler_(message);
    }
}
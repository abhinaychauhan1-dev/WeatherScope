/**
 * @file     : weather_repository.h
 * @brief    : Declares the network-aware weather repository implementation.
 * @details  : Coordinates weather polling, reachability changes, background
 * parsing, SQLite caching, and offline fallback. Architecture role: Repository
 * implementation in the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include "model/i_network_reachability_service.h"
#include "model/i_weather_client.h"
#include "model/i_weather_repository.h"
#include "model/sqlite_weather_store.h"

#include <QObject>
#include <QString>

#include <cstdint>
#include <memory>
#include <optional>

/**
 * @brief Supplies parsed forecasts from network data or the persistent cache.
 * @details The QObject-facing API and callbacks are affinity-thread confined;
 * CPU parsing and SQLite access are delegated to QtConcurrent workers before
 * results return to the owning thread.
 * @note Public methods are not thread-safe and must run on the QObject affinity
 * thread. QObject disables copy and move. Client and reachability references
 * are borrowed and must outlive this object; the cache store is shared-owned.
 */
class WeatherRepository final : public QObject, public IWeatherRepository
{
    /** @brief Enables Qt context-bound continuations and queued invocation. */
    Q_OBJECT

public:
    /**
     * @brief Constructs a repository and installs collaborator callbacks.
     * @param[in] weatherClient Borrowed weather transport that must outlive this object.
     * @param[in] reachabilityService Borrowed reachability service that must outlive this object.
     * @param[in] databasePath Path used to construct the owned cache store.
     * @param[in] parent Optional QObject owner.
     * @note Construct on the intended affinity thread; exceptions may propagate.
     */
    WeatherRepository(
        IWeatherClient &weatherClient,
        INetworkReachabilityService &reachabilityService,
        QString databasePath,
        QObject *parent = nullptr);
    /** @brief Detaches callbacks and the reachability subscription. @note Call on the affinity thread; implicitly noexcept. */
    ~WeatherRepository() override;

    /** @brief Records a polling request and selects network or cache delivery. @param[in] latitude Latitude in decimal degrees. @param[in] longitude Longitude in decimal degrees. @param[in] intervalMilliseconds Positive polling interval in milliseconds. @return Nothing. @note Affinity-thread only; collaborator exceptions may propagate. */
    void startPolling(
        double latitude,
        double longitude,
        std::int32_t intervalMilliseconds) override;
    /** @brief Stops transport polling and clears requested-polling state. @return Nothing. @note Affinity-thread only and non-throwing. */
    void stopPolling() noexcept override;
    /** @brief Forwards the request locale to the weather client. @param[in] languageCode ISO 639 code. @return Nothing. @note Affinity-thread only; exceptions may propagate. */
    void setLanguageCode(const QString &languageCode) override;
    /** @brief Replaces the forecast consumer callback. @param[in] handler Callback to retain, or empty to detach. @return Nothing. @note Affinity-thread only. */
    void setForecastHandler(ForecastHandler handler) noexcept override;
    /** @brief Replaces the repository error callback. @param[in] handler Callback to retain, or empty to detach. @return Nothing. @note Affinity-thread only. */
    void setErrorHandler(ErrorHandler handler) noexcept override;

private:
    /** @brief Switches between network polling and offline cache fallback. @param[in] isReachable Current reachability state. @return Nothing. @note Marshalled to the affinity thread before mutating repository state. */
    void handleReachabilityChanged(bool isReachable);
    /** @brief Parses and caches a fresh serialized payload asynchronously. @param[in] payload Network response bytes copied for worker processing. @return Nothing. @note Starts QtConcurrent work; completion returns to the affinity thread. */
    void processNetworkPayload(const QByteArray &payload);
    /** @brief Loads and parses the latest cache entry asynchronously. @return Nothing. @note Starts QtConcurrent work; completion returns to the affinity thread. */
    void loadCachedPayload();
    /** @brief Publishes an error when a consumer is registered. @param[in] message Human-readable failure description. @return Nothing. @note Affinity-thread only; consumer exceptions may propagate. */
    void reportError(const QString &message) const;

    // These collaborators are non-owning and must outlive this repository.
    IWeatherClient &weatherClient_;
    INetworkReachabilityService &reachabilityService_;
    std::shared_ptr<SqliteWeatherStore> store_;
    std::optional<INetworkReachabilityService::SubscriptionId> reachabilitySubscription_{};
    ForecastHandler forecastHandler_;
    ErrorHandler errorHandler_;
    double latitude_{0.0};
    double longitude_{0.0};
    std::int32_t intervalMilliseconds_{0};
    bool pollingRequested_{false};
};
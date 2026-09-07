/**
 * @file     : i_weather_repository.h
 * @brief    : Declares the weather repository abstraction.
 * @details  : Defines the Model-layer boundary that supplies parsed forecasts
 * while hiding network selection, cache persistence, and fallback policy.
 * Architecture role: Repository interface in the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include "model/weather_telemetry.h"

#include <QString>

#include <cstdint>
#include <functional>

/**
 * @brief Coordinates forecast delivery independently of its data source.
 * @details Consumers request polling and receive parsed forecast snapshots with
 * an explicit cache-origin flag through a replaceable callback.
 * @note Thread safety and callback execution context are implementation-defined.
 * Copy and move semantics are intentionally not constrained by this interface.
 */
class IWeatherRepository
{
public:
    /** @brief Callback receiving a forecast and true when it originated from cache. */
    using ForecastHandler = std::function<void(const WeatherForecastData &, bool)>;
    /** @brief Callback receiving a human-readable repository error. */
    using ErrorHandler = std::function<void(const QString &)>;

    /** @brief Enables polymorphic destruction. @note Implicitly noexcept. */
    virtual ~IWeatherRepository() = default;

    /**
     * @brief Starts or updates periodic forecast retrieval for a location.
     * @param[in] latitude Latitude in decimal degrees.
     * @param[in] longitude Longitude in decimal degrees.
     * @param[in] intervalMilliseconds Polling interval in milliseconds.
     * @return Nothing.
     * @note Validation, fallback policy, execution thread, and exceptions are implementation-defined.
     */
    virtual void startPolling(
        double latitude,
        double longitude,
        std::int32_t intervalMilliseconds) = 0;
    /** @brief Stops requested forecast polling. @return Nothing. @note Non-throwing; thread requirements are implementation-defined. */
    virtual void stopPolling() noexcept = 0;
    /**
     * @brief Selects the language used by subsequent forecast requests.
     * @param[in] languageCode ISO 639 language code.
     * @return Nothing.
     * @note Unsupported-code behavior and exceptions are implementation-defined.
     */
    virtual void setLanguageCode(const QString &languageCode) = 0;
    /**
     * @brief Replaces the parsed-forecast callback.
     * @param[in] handler Callable to retain, or an empty callable to detach.
     * @return Nothing.
     * @note Callback ownership and synchronization are implementation-defined.
     */
    virtual void setForecastHandler(ForecastHandler handler) noexcept = 0;
    /**
     * @brief Replaces the repository-error callback.
     * @param[in] handler Callable to retain, or an empty callable to detach.
     * @return Nothing.
     * @note Callback ownership and synchronization are implementation-defined.
     */
    virtual void setErrorHandler(ErrorHandler handler) noexcept = 0;
};
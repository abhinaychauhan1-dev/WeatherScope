/**
 * @file     : i_weather_client.h
 * @brief    : Declares the asynchronous weather client abstraction.
 * @details  : Defines the Service-layer boundary for polling serialized weather
 * payloads and reporting transport failures to Model-layer consumers.
 * Architecture role: Service interface in the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include <QByteArray>
#include <QString>

#include <cstdint>
#include <functional>

/**
 * @brief Defines asynchronous weather polling independent of transport details.
 * @details Implementations publish raw payloads and errors through replaceable
 * callbacks so parsing and persistence remain outside the network client.
 * @note Thread safety, callback context, and request cancellation are
 * implementation-defined. Copy and move semantics are not constrained here.
 */
class IWeatherClient
{
public:
    /** @brief Callback receiving an immutable serialized weather payload. */
    using WeatherDataHandler = std::function<void(const QByteArray &)>;
    /** @brief Callback receiving a human-readable transport error. */
    using ErrorHandler = std::function<void(const QString &)>;

    /** @brief Enables polymorphic destruction. @note Implicitly noexcept. */
    virtual ~IWeatherClient() = default;

    /**
     * @brief Starts immediate and periodic weather retrieval for a location.
     * @param[in] latitude Latitude in decimal degrees.
     * @param[in] longitude Longitude in decimal degrees.
     * @param[in] intervalMilliseconds Polling interval in milliseconds.
     * @return Nothing.
     * @note Validation, execution thread, and exceptions are implementation-defined.
     */
    virtual void startPolling(
        double latitude,
        double longitude,
        std::int32_t intervalMilliseconds) = 0;
    /** @brief Stops periodic retrieval. @return Nothing. @note Non-throwing; thread requirements are implementation-defined. */
    virtual void stopPolling() noexcept = 0;
    /** @brief Reports whether polling is active. @return true while polling is active. @note Non-throwing; thread safety is implementation-defined. */
    [[nodiscard]] virtual bool isPolling() const noexcept = 0;
    /**
     * @brief Selects the language used by subsequent network requests.
     * @param[in] languageCode ISO 639 language code.
     * @return Nothing.
     * @note Unsupported-code behavior and exceptions are implementation-defined.
     */
    virtual void setLanguageCode(const QString &languageCode) = 0;
    /**
     * @brief Replaces the successful-payload callback.
     * @param[in] handler Callable to retain, or an empty callable to detach.
     * @return Nothing.
     * @note Callback ownership and synchronization are implementation-defined.
     */
    virtual void setWeatherDataHandler(WeatherDataHandler handler) noexcept = 0;
    /**
     * @brief Replaces the transport-error callback.
     * @param[in] handler Callable to retain, or an empty callable to detach.
     * @return Nothing.
     * @note Callback ownership and synchronization are implementation-defined.
     */
    virtual void setErrorHandler(ErrorHandler handler) noexcept = 0;
};
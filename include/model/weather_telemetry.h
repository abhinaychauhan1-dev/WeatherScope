/**
 * @file     : weather_telemetry.h
 * @brief    : Defines immutable weather telemetry domain values.
 * @details  : Provides transport-independent current, hourly, and daily
 * forecast records shared by Model and ViewModel code. Architecture role:
 * domain-value definitions in the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include <QString>

#include <cstdint>
#include <vector>

/**
 * @brief Immutable snapshot of current weather conditions.
 * @note Copy/move construction is supported, assignment is disabled by const
 * members, and distinct instances are safe to transfer across threads.
 */
struct WeatherTelemetry final
{
    /** @brief World Meteorological Organization interpretation code. */
    const std::int32_t weatherCode{0};
    /** @brief Air temperature in degrees Celsius. */
    const float temperatureC{0.0F};
    /** @brief Wind speed in kilometres per hour. */
    const float windSpeedKph{0.0F};
    /** @brief Precipitation probability as a percentage in [0, 100]. */
    const std::int32_t precipitationProbability{0};
    /** @brief Relative humidity as a percentage in [0, 100]. */
    const std::int32_t relativeHumidity{0};

    [[nodiscard]] bool operator==(const WeatherTelemetry &) const = default;
};

/**
 * @brief Mutable value record for one hourly forecast sample.
 * @note Supports ordinary value copy and move. Concurrent mutation of the same
 * instance requires external synchronization.
 */
struct HourlyForecastData final
{
    /** @brief Provider timestamp encoded as an ISO 8601 string. */
    QString timestampStr{};
    /** @brief Forecast temperature in degrees Celsius. */
    float temperatureC{0.0F};
    /** @brief Forecast wind speed in kilometres per hour. */
    float windSpeedKph{0.0F};
    /** @brief World Meteorological Organization interpretation code. */
    std::int32_t weatherCode{0};
    /** @brief Precipitation probability as a percentage in [0, 100]. */
    std::int32_t precipitationProbability{0};

    [[nodiscard]] bool operator==(const HourlyForecastData &) const = default;
};

/**
 * @brief Mutable value record for one daily forecast summary.
 * @note Supports ordinary value copy and move. Concurrent mutation of the same
 * instance requires external synchronization.
 */
struct DailyForecastData final
{
    /** @brief Forecast date encoded as an ISO 8601 date string. */
    QString dateStr{};
    /** @brief Minimum forecast temperature in degrees Celsius. */
    float tempMin{0.0F};
    /** @brief Maximum forecast temperature in degrees Celsius. */
    float tempMax{0.0F};
    /** @brief World Meteorological Organization interpretation code. */
    std::int32_t weatherCode{0};
    /** @brief Precipitation probability as a percentage in [0, 100]. */
    std::int32_t precipitationProbability{0};

    [[nodiscard]] bool operator==(const DailyForecastData &) const = default;
};

/**
 * @brief Aggregates current conditions with hourly and daily forecast series.
 * @note Copy/move construction is supported. Assignment is constrained by the
 * immutable current snapshot. Distinct instances may be transferred across threads.
 */
struct WeatherForecastData final
{
    /** @brief Current-condition snapshot. */
    WeatherTelemetry current{};
    /** @brief Ordered hourly forecast samples, typically up to 24 entries. */
    std::vector<HourlyForecastData> hourly{};
    /** @brief Ordered daily forecast summaries, typically up to seven entries. */
    std::vector<DailyForecastData> daily{};

    [[nodiscard]] bool operator==(const WeatherForecastData &) const = default;
};
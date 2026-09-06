/**
 * @file     : weather_telemetry_parser.h
 * @brief    : Declares weather telemetry payload parsing operations.
 * @details  : Converts serialized provider JSON into validated domain values
 * without retaining state. Architecture role: parser Service in the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include "model/weather_telemetry.h"

#include <QByteArray>
#include <QString>

#include <optional>

/**
 * @brief Carries either a parsed forecast or a validation error.
 * @details An empty error denotes successful parsing. Const members make the
 * result immutable and non-assignable after construction.
 * @note Separate result instances may be transferred safely between threads.
 */
struct WeatherTelemetryParseResult final
{
    /** @brief Parsed forecast; disengaged when parsing or validation fails. */
    const std::optional<WeatherForecastData> forecast{};
    /** @brief Human-readable parse failure; empty on success. */
    const QString error{};
};

/**
 * @brief Stateless utility for strict weather payload parsing and validation.
 * @details Numeric conversions and forecast ranges are checked before domain
 * values are produced; failures are returned rather than stored globally.
 * @note Thread-safe and callable from any thread because no shared state exists.
 * Construction is disabled; copy and move operations are consequently unavailable.
 */
class WeatherTelemetryParser final
{
public:
    /** @brief Prevents instantiation of this static-only utility type. */
    WeatherTelemetryParser() = delete;

    /** @brief Parses and validates a serialized weather response. @param[in] payload Provider JSON bytes. @return Immutable forecast/error result. @note Thread-safe; parse failures are reported in the result rather than thrown. */
    [[nodiscard]] static WeatherTelemetryParseResult parse(const QByteArray &payload);
};
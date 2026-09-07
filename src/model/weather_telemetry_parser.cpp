/**
 * @file     : weather_telemetry_parser.cpp
 * @brief    : Parses weather API payloads into telemetry models.
 * @details  : Provides the definitions declared in weather_telemetry_parser.h,
 * validating the Open-Meteo JSON schema and numeric conversions before
 * constructing fixed-size current, hourly, and daily domain snapshots.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

#include "model/weather_telemetry_parser.h"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <cmath>
#include <array>
#include <algorithm>
#include <limits>

namespace
{
constexpr int hourlyForecastCount{24};
constexpr int dailyForecastCount{7};

/** @brief Tests whether a JSON number can be narrowed safely to float. @param[in] value Candidate value. @return true for finite values within float range. @note Non-throwing and stateless. */
[[nodiscard]] bool canConvertToFloat(const double value) noexcept
{
    const double maximum = static_cast<double>(std::numeric_limits<float>::max());
    return std::isfinite(value) && (value >= -maximum) && (value <= maximum);
}

/** @brief Tests whether a JSON number is an exact representable 32-bit integer. @param[in] value Candidate value. @return true when finite, integral, and in range. @note The truncation check rejects fractional values before narrowing. */
[[nodiscard]] bool canConvertToInt32(const double value) noexcept
{
    const double minimum = static_cast<double>(std::numeric_limits<std::int32_t>::min());
    const double maximum = static_cast<double>(std::numeric_limits<std::int32_t>::max());
    return std::isfinite(value) && (value >= minimum) && (value <= maximum)
        && (std::trunc(value) == value);
}

/** @brief Validates an API percentage. @param[in] value Candidate percentage. @return true for integral values in [0, 100]. @note Out-of-contract values are rejected rather than clamped. */
[[nodiscard]] bool isValidProbability(const double value) noexcept
{
    return canConvertToInt32(value) && (value >= 0.0) && (value <= 100.0);
}

/** @brief Accepts only weather interpretation codes defined by the Open-Meteo WMO table. */
[[nodiscard]] bool isValidWeatherCode(const double value) noexcept
{
    constexpr std::array<std::int32_t, 28> validCodes{
        0, 1, 2, 3, 45, 48, 51, 53, 55, 56, 57, 61, 63, 65,
        66, 67, 71, 73, 75, 77, 80, 81, 82, 85, 86, 95, 96, 99};
    if (!canConvertToInt32(value)) {
        return false;
    }

    const auto weatherCode = static_cast<std::int32_t>(value);
    return std::find(validCodes.cbegin(), validCodes.cend(), weatherCode)
        != validCodes.cend();
}

/**
 * @brief Verifies that five parallel provider arrays all contain an index.
 * @param[in] index Required zero-based index.
 * @param[in] first First schema array.
 * @param[in] second Second schema array.
 * @param[in] third Third schema array.
 * @param[in] fourth Fourth schema array.
 * @param[in] fifth Fifth schema array.
 * @return true when index is non-negative and valid for every array.
 * @note Non-throwing; prevents misaligned forecast fields from being combined.
 */
[[nodiscard]] bool arraysContainIndex(
    const int index,
    const QJsonArray &first,
    const QJsonArray &second,
    const QJsonArray &third,
    const QJsonArray &fourth,
    const QJsonArray &fifth) noexcept
{
    return (index >= 0) && (index < first.size()) && (index < second.size())
        && (index < third.size()) && (index < fourth.size()) && (index < fifth.size());
}
} // namespace

/**
 * @brief Validates and converts an Open-Meteo payload into domain forecast data.
 * @param[in] payload Serialized UTF-8 JSON response.
 * @return Parsed forecast on success, otherwise an explanatory error.
 * @pre payload is a complete response using the requested UTC forecast schema.
 * @post Success contains exactly 24 hourly and seven daily entries; failure
 * contains no forecast and does not retain partial state.
 */
WeatherTelemetryParseResult WeatherTelemetryParser::parse(const QByteArray &payload)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);

    // Fail fast at each schema boundary so no partially validated telemetry escapes.
    if (parseError.error != QJsonParseError::NoError) {
        return {{}, QStringLiteral("Invalid weather JSON: %1").arg(parseError.errorString())};
    }

    if (!document.isObject()) {
        return {{}, QStringLiteral("Weather payload root must be a JSON object.")};
    }

    const QJsonObject root = document.object();
    const QJsonValue currentValue = root.value(QStringLiteral("current"));
    if (!currentValue.isObject()) {
        return {{}, QStringLiteral("Weather payload does not contain a current object.")};
    }

    const QJsonObject current = currentValue.toObject();
    const QJsonValue weatherCodeValue = current.value(QStringLiteral("weather_code"));
    const QJsonValue temperatureValue = current.value(QStringLiteral("temperature_2m"));
    const QJsonValue windSpeedValue = current.value(QStringLiteral("wind_speed_10m"));
    const QJsonValue humidityValue = current.value(QStringLiteral("relative_humidity_2m"));

    if (!weatherCodeValue.isDouble() || !temperatureValue.isDouble()
        || !windSpeedValue.isDouble() || !humidityValue.isDouble()) {
        return {{}, QStringLiteral("Weather telemetry fields are missing or not numeric.")};
    }

    const double weatherCode = weatherCodeValue.toDouble();
    const double temperature = temperatureValue.toDouble();
    const double windSpeed = windSpeedValue.toDouble();
    const double relativeHumidity = humidityValue.toDouble();
    // Validate before every narrowing conversion to avoid undefined or lossy values.
    if (!isValidWeatherCode(weatherCode) || !canConvertToFloat(temperature)
        || !canConvertToFloat(windSpeed) || (windSpeed < 0.0)
        || !isValidProbability(relativeHumidity)) {
        return {{}, QStringLiteral("Weather telemetry contains an out-of-range value.")};
    }

    const QJsonValue hourlyValue = root.value(QStringLiteral("hourly"));
    const QJsonValue dailyValue = root.value(QStringLiteral("daily"));
    if (!hourlyValue.isObject() || !dailyValue.isObject()) {
        return {{}, QStringLiteral("Weather payload does not contain forecast objects.")};
    }

    const QJsonObject hourlyObject = hourlyValue.toObject();
    const QJsonArray hourlyTimes = hourlyObject.value(QStringLiteral("time")).toArray();
    const QJsonArray hourlyTemperatures =
        hourlyObject.value(QStringLiteral("temperature_2m")).toArray();
    const QJsonArray hourlyWeatherCodes =
        hourlyObject.value(QStringLiteral("weather_code")).toArray();
    const QJsonArray hourlyPrecipitation =
        hourlyObject.value(QStringLiteral("precipitation_probability")).toArray();
    const QJsonArray hourlyWindSpeeds =
        hourlyObject.value(QStringLiteral("wind_speed_10m")).toArray();

    const QDateTime currentTimestamp = QDateTime::currentDateTimeUtc();
    int hourlyStartIndex{-1};
    // The API was requested in UTC without a suffix; append Z while parsing and
    // select the containing hour, or the first future hour if no interval contains now.
    for (int index = 0; index < hourlyTimes.size(); ++index) {
        const QString timestampText = hourlyTimes.at(index).toString();
        const QDateTime timestamp =
            QDateTime::fromString(timestampText + QStringLiteral("Z"), Qt::ISODate);
        if (!timestamp.isValid()) {
            continue;
        }

        const QDateTime hourEnd = timestamp.addSecs(3'600);
        const bool containsCurrentTime = (timestamp <= currentTimestamp)
            && (currentTimestamp < hourEnd);
        const bool isNextAvailableHour = timestamp > currentTimestamp;
        if (containsCurrentTime || isNextAvailableHour) {
            hourlyStartIndex = index;
            break;
        }
    }

    // All parallel hourly fields must cover the complete 24-entry presentation window.
    if ((hourlyStartIndex < 0)
        || !arraysContainIndex(
            hourlyStartIndex + hourlyForecastCount - 1,
            hourlyTimes,
            hourlyTemperatures,
            hourlyWeatherCodes,
            hourlyPrecipitation,
            hourlyWindSpeeds)) {
        return {{}, QStringLiteral("Weather payload does not contain 24 upcoming hourly entries.")};
    }

    std::vector<HourlyForecastData> hourlyForecast;
    hourlyForecast.reserve(static_cast<std::size_t>(hourlyForecastCount));
    for (int offset = 0; offset < hourlyForecastCount; ++offset) {
        const int index = hourlyStartIndex + offset;
        const QJsonValue temperatureEntry = hourlyTemperatures.at(index);
        const QJsonValue weatherCodeEntry = hourlyWeatherCodes.at(index);
        const QJsonValue precipitationEntry = hourlyPrecipitation.at(index);
        const QJsonValue windSpeedEntry = hourlyWindSpeeds.at(index);
        if (!hourlyTimes.at(index).isString() || !temperatureEntry.isDouble()
            || !weatherCodeEntry.isDouble() || !precipitationEntry.isDouble()
            || !windSpeedEntry.isDouble()) {
            return {{}, QStringLiteral("An hourly forecast entry has an invalid type.")};
        }

        const double hourlyTemperature = temperatureEntry.toDouble();
        const double hourlyWeatherCode = weatherCodeEntry.toDouble();
        const double precipitationProbability = precipitationEntry.toDouble();
        const double hourlyWindSpeed = windSpeedEntry.toDouble();
        if (!canConvertToFloat(hourlyTemperature) || !isValidWeatherCode(hourlyWeatherCode)
            || !isValidProbability(precipitationProbability)
            || !canConvertToFloat(hourlyWindSpeed) || (hourlyWindSpeed < 0.0)) {
            return {{}, QStringLiteral("An hourly forecast entry is out of range.")};
        }

        const QString timestampText = hourlyTimes.at(index).toString();
        const QDateTime timestamp =
            QDateTime::fromString(timestampText + QStringLiteral("Z"), Qt::ISODate);
        if (!timestamp.isValid()) {
            return {{}, QStringLiteral("An hourly forecast timestamp is invalid.")};
        }

        hourlyForecast.push_back(HourlyForecastData{
            .timestampStr = timestampText,
            .temperatureC = static_cast<float>(hourlyTemperature),
            .windSpeedKph = static_cast<float>(hourlyWindSpeed),
            .weatherCode = static_cast<std::int32_t>(hourlyWeatherCode),
            .precipitationProbability =
                static_cast<std::int32_t>(precipitationProbability)});
    }

    const QJsonObject dailyObject = dailyValue.toObject();
    const QJsonArray dailyDates = dailyObject.value(QStringLiteral("time")).toArray();
    const QJsonArray dailyWeatherCodes =
        dailyObject.value(QStringLiteral("weather_code")).toArray();
    const QJsonArray dailyMaximumTemperatures =
        dailyObject.value(QStringLiteral("temperature_2m_max")).toArray();
    const QJsonArray dailyMinimumTemperatures =
        dailyObject.value(QStringLiteral("temperature_2m_min")).toArray();
    const QJsonArray dailyPrecipitation =
        dailyObject.value(QStringLiteral("precipitation_probability_max")).toArray();

    const QDate currentDate = currentTimestamp.date();
    int dailyStartIndex{-1};
    // Skip stale dates while preserving today as the first valid daily summary.
    for (int index = 0; index < dailyDates.size(); ++index) {
        const QDate date = QDate::fromString(dailyDates.at(index).toString(), Qt::ISODate);
        if (date.isValid() && (date >= currentDate)) {
            dailyStartIndex = index;
            break;
        }
    }

    // Reject truncated or misaligned arrays rather than synthesizing incomplete days.
    if ((dailyStartIndex < 0)
        || !arraysContainIndex(
            dailyStartIndex + dailyForecastCount - 1,
            dailyDates,
            dailyWeatherCodes,
            dailyMinimumTemperatures,
            dailyMaximumTemperatures,
            dailyPrecipitation)) {
        return {{}, QStringLiteral("Weather payload does not contain 7 upcoming daily entries.")};
    }

    std::vector<DailyForecastData> dailyForecast;
    dailyForecast.reserve(static_cast<std::size_t>(dailyForecastCount));
    for (int offset = 0; offset < dailyForecastCount; ++offset) {
        const int index = dailyStartIndex + offset;
        const QJsonValue weatherCodeEntry = dailyWeatherCodes.at(index);
        const QJsonValue minimumTemperatureEntry = dailyMinimumTemperatures.at(index);
        const QJsonValue maximumTemperatureEntry = dailyMaximumTemperatures.at(index);
        const QJsonValue precipitationEntry = dailyPrecipitation.at(index);
        if (!dailyDates.at(index).isString() || !weatherCodeEntry.isDouble()
            || !minimumTemperatureEntry.isDouble() || !maximumTemperatureEntry.isDouble()
            || !precipitationEntry.isDouble()) {
            return {{}, QStringLiteral("A daily forecast entry has an invalid type.")};
        }

        const double dailyWeatherCode = weatherCodeEntry.toDouble();
        const double minimumTemperature = minimumTemperatureEntry.toDouble();
        const double maximumTemperature = maximumTemperatureEntry.toDouble();
        const double precipitationProbability = precipitationEntry.toDouble();
        if (!isValidWeatherCode(dailyWeatherCode) || !canConvertToFloat(minimumTemperature)
            || !canConvertToFloat(maximumTemperature)
            || (minimumTemperature > maximumTemperature)
            || !isValidProbability(precipitationProbability)) {
            return {{}, QStringLiteral("A daily forecast entry is out of range.")};
        }

        const QString dateText = dailyDates.at(index).toString();
        if (!QDate::fromString(dateText, Qt::ISODate).isValid()) {
            return {{}, QStringLiteral("A daily forecast date is invalid.")};
        }

        dailyForecast.push_back(DailyForecastData{
            .dateStr = dateText,
            .tempMin = static_cast<float>(minimumTemperature),
            .tempMax = static_cast<float>(maximumTemperature),
            .weatherCode = static_cast<std::int32_t>(dailyWeatherCode),
            .precipitationProbability =
                static_cast<std::int32_t>(precipitationProbability)});
    }

    // Construction occurs only after every entry is validated, making success all-or-nothing.
    return {
        WeatherForecastData{
            .current = WeatherTelemetry{
                .weatherCode = static_cast<std::int32_t>(weatherCode),
                .temperatureC = static_cast<float>(temperature),
                .windSpeedKph = static_cast<float>(windSpeed),
                // Current precipitation probability is aligned with the selected
                // current/next hourly sample because the current object lacks that field.
                .precipitationProbability =
                    hourlyForecast.front().precipitationProbability,
                .relativeHumidity = static_cast<std::int32_t>(relativeHumidity)},
            .hourly = std::move(hourlyForecast),
            .daily = std::move(dailyForecast)},
        {}};
}
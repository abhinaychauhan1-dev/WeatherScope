/**
 * @file     : weather_presentation_formatter.h
 * @brief    : Provides localized formatting shared by weather ViewModels.
 */

#pragma once

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QLocale>
#include <QString>

#include <cstdint>

namespace WeatherPresentationFormatter
{
[[nodiscard]] inline QString conditionDescription(const std::int32_t weatherCode)
{
    switch (weatherCode) {
    case 0:
        return QCoreApplication::translate("WeatherMainViewModel", "Clear sky");
    case 1:
        return QCoreApplication::translate("WeatherMainViewModel", "Mainly clear");
    case 2:
        return QCoreApplication::translate("WeatherMainViewModel", "Partly cloudy");
    case 3:
        return QCoreApplication::translate("WeatherMainViewModel", "Overcast");
    case 45:
    case 48:
        return QCoreApplication::translate("WeatherMainViewModel", "Fog");
    case 51:
    case 53:
    case 55:
        return QCoreApplication::translate("WeatherMainViewModel", "Drizzle");
    case 56:
    case 57:
        return QCoreApplication::translate("WeatherMainViewModel", "Freezing drizzle");
    case 61:
    case 63:
    case 65:
        return QCoreApplication::translate("WeatherMainViewModel", "Rain");
    case 66:
    case 67:
        return QCoreApplication::translate("WeatherMainViewModel", "Freezing rain");
    case 71:
    case 73:
    case 75:
    case 77:
        return QCoreApplication::translate("WeatherMainViewModel", "Snow");
    case 80:
    case 81:
    case 82:
        return QCoreApplication::translate("WeatherMainViewModel", "Rain showers");
    case 85:
    case 86:
        return QCoreApplication::translate("WeatherMainViewModel", "Snow showers");
    case 95:
    case 96:
    case 99:
        return QCoreApplication::translate("WeatherMainViewModel", "Thunderstorm");
    default:
        return QCoreApplication::translate("WeatherMainViewModel", "Unknown");
    }
}

[[nodiscard]] inline QString conditionSummary(const std::int32_t weatherCode)
{
    switch (weatherCode) {
    case 0:
        return QCoreApplication::translate("DailyCardDelegate", "Clear", "Weather condition");
    case 1:
    case 2:
    case 3:
        return QCoreApplication::translate("DailyCardDelegate", "Cloudy", "Weather condition");
    case 45:
    case 48:
        return QCoreApplication::translate("DailyCardDelegate", "Fog", "Weather condition");
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
    case 61:
    case 63:
    case 65:
    case 66:
    case 67:
        return QCoreApplication::translate("DailyCardDelegate", "Rain", "Weather condition");
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
        return QCoreApplication::translate("DailyCardDelegate", "Snow", "Weather condition");
    case 80:
    case 81:
    case 82:
        return QCoreApplication::translate("DailyCardDelegate", "Showers", "Weather condition");
    case 95:
    case 96:
    case 99:
        return QCoreApplication::translate("DailyCardDelegate", "Storm", "Weather condition");
    default:
        return QCoreApplication::translate("DailyCardDelegate", "Unknown", "Weather condition");
    }
}

[[nodiscard]] inline QString dailyDateLabel(
    const QString &isoDate,
    const bool isFirstRow)
{
    const QDate date = QDate::fromString(isoDate, Qt::ISODate);
    if (!date.isValid()) {
        return isoDate;
    }

    if (isFirstRow && (date == QDateTime::currentDateTimeUtc().date())) {
        return QCoreApplication::translate("DailyCardDelegate", "Today", "Relative date label");
    }

    return QLocale{}.toString(date, QStringLiteral("ddd"));
}

[[nodiscard]] inline QString temperatureText(
    const float temperatureCelsius,
    const int decimalPlaces = 0)
{
    return QLocale{}.toString(
               static_cast<double>(temperatureCelsius),
               'f',
               decimalPlaces)
        + QStringLiteral("\u00B0C");
}

[[nodiscard]] inline QString windSpeedText(const float windSpeedKph)
{
    return QLocale{}.toString(static_cast<double>(windSpeedKph), 'f', 1)
        + QStringLiteral(" km/h");
}

[[nodiscard]] inline QString temperatureRangeText(
    const float minimumCelsius,
    const float maximumCelsius)
{
    return QLocale{}.toString(static_cast<double>(minimumCelsius), 'f', 0)
        + QStringLiteral("\u00B0 / ")
        + QLocale{}.toString(static_cast<double>(maximumCelsius), 'f', 0)
        + QStringLiteral("\u00B0");
}

[[nodiscard]] inline QString percentageText(const std::int32_t percentage)
{
    return QLocale{}.toString(percentage) + QStringLiteral("%");
}
} // namespace WeatherPresentationFormatter

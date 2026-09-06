/**
 * @file     : hourly_forecast_view_model.cpp
 * @brief    : Implements the hourly forecast list model exposed to QML.
 * @details  : Provides the definitions declared in
 * hourly_forecast_view_model.h, filtering UTC forecast records to the next 24
 * entries and publishing replacements through Qt model-reset signals.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "viewmodel/hourly_forecast_view_model.h"
#include "viewmodel/weather_presentation_formatter.h"

#include <QDateTime>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

/**
 * @brief Constructs an empty hourly forecast list model.
 * @param[in] parent Optional QObject lifecycle owner.
 * @pre Construction occurs on the intended model-affinity thread.
 * @post rowCount() is zero until forecast data is supplied.
 */
HourlyForecastViewModel::HourlyForecastViewModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

/**
 * @brief Reports the number of rows in this flat model.
 * @param[in] parent Parent index supplied by the model consumer.
 * @return Zero for child queries, otherwise the safely bounded row count.
 * @pre Called on the model-affinity thread.
 * @post Model state is unchanged.
 */
int HourlyForecastViewModel::rowCount(const QModelIndex &parent) const
{
    // A flat list has no children beneath a valid model index.
    if (parent.isValid()) {
        return 0;
    }

    const std::size_t maximumRowCount =
        static_cast<std::size_t>(std::numeric_limits<int>::max());
    // Bound the unsigned container size before narrowing to Qt's signed row type.
    const std::size_t boundedRowCount = std::min(forecastData_.size(), maximumRowCount);
    return static_cast<int>(boundedRowCount);
}

/**
 * @brief Resolves an hourly row and role to a QVariant for QML.
 * @param[in] index Requested model row.
 * @param[in] role Integer value corresponding to ForecastRoles.
 * @return Requested value, or an invalid QVariant for invalid input.
 * @pre Called on the model-affinity thread.
 * @post Model state is unchanged.
 */
QVariant HourlyForecastViewModel::data(const QModelIndex &index, const int role) const
{
    // Reject malformed QModelIndex input before converting its signed row value.
    if (!index.isValid() || (index.row() < 0)) {
        return QVariant{};
    }

    const std::size_t row = static_cast<std::size_t>(index.row());
    if (row >= forecastData_.size()) {
        return QVariant{};
    }

    const HourlyForecastData &forecast = forecastData_.at(row);
    switch (static_cast<ForecastRoles>(role)) {
    case ForecastRoles::HourLabel: {
        // API timestamps omit the UTC suffix because UTC was requested explicitly;
        // append Z for unambiguous parsing and reject malformed values defensively.
        const QDateTime timestamp = QDateTime::fromString(
            forecast.timestampStr + QStringLiteral("Z"), Qt::ISODate);
        return timestamp.isValid()
            ? QVariant::fromValue(timestamp.toString(QStringLiteral("HH:mm")))
            : QVariant{};
    }
    case ForecastRoles::Temperature:
        return QVariant::fromValue(forecast.temperatureC);
    case ForecastRoles::ConditionCode:
        return QVariant::fromValue(forecast.weatherCode);
    case ForecastRoles::RainChance:
        return QVariant::fromValue(forecast.precipitationProbability);
    case ForecastRoles::ConditionDescription:
        return QVariant::fromValue(
            WeatherPresentationFormatter::conditionSummary(forecast.weatherCode));
    case ForecastRoles::TemperatureText:
        return QVariant::fromValue(
            WeatherPresentationFormatter::temperatureText(forecast.temperatureC));
    case ForecastRoles::RainChanceText:
        return QVariant::fromValue(
            WeatherPresentationFormatter::percentageText(forecast.precipitationProbability));
    default:
        // Unknown roles map to QVariant invalid/undefined as required by Qt models.
        return QVariant{};
    }
}

/** @brief Publishes stable QML names for hourly forecast roles. @return Role-to-name mapping. @pre Called on the model-affinity thread. @post Model state is unchanged. */
QHash<int, QByteArray> HourlyForecastViewModel::roleNames() const
{
    return {
        {static_cast<int>(ForecastRoles::HourLabel), QByteArrayLiteral("hourLabel")},
        {static_cast<int>(ForecastRoles::Temperature), QByteArrayLiteral("temperature")},
        {static_cast<int>(ForecastRoles::ConditionCode), QByteArrayLiteral("conditionCode")},
        {static_cast<int>(ForecastRoles::RainChance), QByteArrayLiteral("rainChance")},
        {static_cast<int>(ForecastRoles::ConditionDescription), QByteArrayLiteral("conditionDescription")},
        {static_cast<int>(ForecastRoles::TemperatureText), QByteArrayLiteral("temperatureText")},
        {static_cast<int>(ForecastRoles::RainChanceText), QByteArrayLiteral("rainChanceText")}};
}

/**
 * @brief Filters and replaces visible data with up to 24 future UTC forecasts.
 * @param[in] newData Forecast values inspected and copied into bounded storage.
 * @pre Called on the model-affinity thread outside another model reset.
 * @post QML observers see one complete reset containing only valid future rows.
 */
void HourlyForecastViewModel::updateForecastData(std::vector<HourlyForecastData> newData)
{
    constexpr std::size_t maximumHourlyForecasts{24U};
    const QDateTime currentTimestamp = QDateTime::currentDateTimeUtc();
    std::vector<HourlyForecastData> upcomingForecasts{};
    // Reserve one presentation day to avoid reallocations during filtering.
    upcomingForecasts.reserve(maximumHourlyForecasts);

    // Filtering at update time keeps delegates simple and excludes stale provider entries.
    for (std::size_t index = 0U;
         (index < newData.size()) && (upcomingForecasts.size() < maximumHourlyForecasts);
         ++index) {
        const HourlyForecastData &forecast = newData.at(index);
        const QDateTime timestamp = QDateTime::fromString(
            forecast.timestampStr + QStringLiteral("Z"), Qt::ISODate);
        if (timestamp.isValid() && (timestamp.addSecs(3'600) > currentTimestamp)) {
            upcomingForecasts.push_back(forecast);
        }
    }

    if (forecastData_ == upcomingForecasts) {
        return;
    }

    // Bracket the storage swap so QML discards stale indexes and rebuilds delegates atomically.
    beginResetModel();
    forecastData_.swap(upcomingForecasts);
    endResetModel();
}

void HourlyForecastViewModel::retranslate()
{
    if (forecastData_.empty()) {
        return;
    }

    emit dataChanged(
        index(0, 0),
        index(rowCount() - 1, 0),
        {static_cast<int>(ForecastRoles::ConditionDescription),
         static_cast<int>(ForecastRoles::TemperatureText),
         static_cast<int>(ForecastRoles::RainChanceText)});
}
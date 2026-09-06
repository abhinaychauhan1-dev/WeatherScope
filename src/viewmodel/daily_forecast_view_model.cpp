/**
 * @file     : daily_forecast_view_model.cpp
 * @brief    : Implements the daily forecast list model exposed to QML.
 * @details  : Provides the definitions declared in
 * daily_forecast_view_model.h, adapting up to seven domain records into a flat
 * role-based model and publishing replacements through Qt model-reset signals.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "viewmodel/daily_forecast_view_model.h"
#include "viewmodel/weather_presentation_formatter.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

/**
 * @brief Constructs an empty daily forecast list model.
 * @param[in] parent Optional QObject lifecycle owner.
 * @pre Construction occurs on the intended model-affinity thread.
 * @post rowCount() is zero until forecast data is supplied.
 */
DailyForecastViewModel::DailyForecastViewModel(QObject *parent)
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
int DailyForecastViewModel::rowCount(const QModelIndex &parent) const
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
 * @brief Resolves a daily row and role to a QVariant for QML.
 * @param[in] index Requested model row.
 * @param[in] role Integer value corresponding to ForecastRoles.
 * @return Requested value, or an invalid QVariant for invalid input.
 * @pre Called on the model-affinity thread.
 * @post Model state is unchanged.
 */
QVariant DailyForecastViewModel::data(const QModelIndex &index, const int role) const
{
    // Reject malformed QModelIndex input before converting its signed row value.
    if (!index.isValid() || (index.row() < 0)) {
        return QVariant{};
    }

    const std::size_t row = static_cast<std::size_t>(index.row());
    if (row >= forecastData_.size()) {
        return QVariant{};
    }

    const DailyForecastData &forecast = forecastData_.at(row);
    switch (static_cast<ForecastRoles>(role)) {
    case ForecastRoles::DateLabel:
        return QVariant::fromValue(WeatherPresentationFormatter::dailyDateLabel(
            forecast.dateStr,
            row == 0U));
    case ForecastRoles::TempMax:
        return QVariant::fromValue(forecast.tempMax);
    case ForecastRoles::TempMin:
        return QVariant::fromValue(forecast.tempMin);
    case ForecastRoles::ConditionCode:
        return QVariant::fromValue(forecast.weatherCode);
    case ForecastRoles::RainChance:
        return QVariant::fromValue(forecast.precipitationProbability);
    case ForecastRoles::ConditionDescription:
        return QVariant::fromValue(
            WeatherPresentationFormatter::conditionSummary(forecast.weatherCode));
    case ForecastRoles::TemperatureRangeText:
        return QVariant::fromValue(WeatherPresentationFormatter::temperatureRangeText(
            forecast.tempMin,
            forecast.tempMax));
    default:
        // Unknown roles map to QVariant invalid/undefined as required by Qt models.
        return QVariant{};
    }
}

/** @brief Publishes stable QML names for daily forecast roles. @return Role-to-name mapping. @pre Called on the model-affinity thread. @post Model state is unchanged. */
QHash<int, QByteArray> DailyForecastViewModel::roleNames() const
{
    return {
        {static_cast<int>(ForecastRoles::DateLabel), QByteArrayLiteral("dateLabel")},
        {static_cast<int>(ForecastRoles::TempMax), QByteArrayLiteral("tempMax")},
        {static_cast<int>(ForecastRoles::TempMin), QByteArrayLiteral("tempMin")},
        {static_cast<int>(ForecastRoles::ConditionCode), QByteArrayLiteral("conditionCode")},
        {static_cast<int>(ForecastRoles::RainChance), QByteArrayLiteral("rainChance")},
        {static_cast<int>(ForecastRoles::ConditionDescription), QByteArrayLiteral("conditionDescription")},
        {static_cast<int>(ForecastRoles::TemperatureRangeText), QByteArrayLiteral("temperatureRangeText")}};
}

/**
 * @brief Replaces visible data with at most seven daily forecasts.
 * @param[in] newData Forecast values transferred into model storage.
 * @pre Called on the model-affinity thread outside another model reset.
 * @post QML observers see one complete reset and no more than seven rows.
 */
void DailyForecastViewModel::updateForecastData(std::vector<DailyForecastData> newData)
{
    constexpr std::size_t maximumDailyForecasts{7U};
    // Keep the presentation contract bounded even if an upstream provider expands its response.
    if (newData.size() > maximumDailyForecasts) {
        newData.resize(maximumDailyForecasts);
    }

    if (forecastData_ == newData) {
        return;
    }

    // Bracket the storage swap so QML discards stale indexes and rebuilds delegates atomically.
    beginResetModel();
    forecastData_.swap(newData);
    endResetModel();
}

void DailyForecastViewModel::retranslate()
{
    if (forecastData_.empty()) {
        return;
    }

    emit dataChanged(
        index(0, 0),
        index(rowCount() - 1, 0),
        {static_cast<int>(ForecastRoles::DateLabel),
         static_cast<int>(ForecastRoles::ConditionDescription),
         static_cast<int>(ForecastRoles::TemperatureRangeText)});
}
/**
 * @file     : hourly_forecast_view_model.h
 * @brief    : Declares the hourly forecast list model exposed to QML.
 * @details  : Filters hourly domain records to upcoming entries, exposes named
 * QML roles, and bounds the presentation set to 24 hours. Architecture role:
 * ViewModel in the MVVM layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include "model/weather_telemetry.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QVariant>
#include <QtQmlIntegration/qqmlintegration.h>

#include <cstdint>
#include <vector>

/**
 * @brief Presents upcoming hourly forecast records as a flat list model.
 * @details Role values provide display times, temperatures, weather codes, and
 * precipitation probabilities to the horizontal QML timeline.
 * @note Not thread-safe; all model access and updates require the QObject
 * affinity thread. QObject disables copy and move. The optional parent owns
 * the model; forecast records are owned by value.
 */
class HourlyForecastViewModel : public QAbstractListModel
{
    /** @brief Enables Qt model meta-object behavior. */
    Q_OBJECT
    /** @brief Registers HourlyForecastViewModel in the WeatherGlobe QML module. */
    QML_ELEMENT

public:
    /** @brief Identifies the QML roles exported for each hourly forecast row. */
    enum class ForecastRoles : std::int32_t
    {
        /** @brief Local-time display label derived from the ISO timestamp. */
        HourLabel = static_cast<std::int32_t>(Qt::UserRole) + 1,
        /** @brief Forecast temperature in degrees Celsius. */
        Temperature,
        /** @brief World Meteorological Organization condition code. */
        ConditionCode,
        /** @brief Precipitation probability as a percentage. */
        RainChance,
        /** @brief Localized summary of the WMO weather condition. */
        ConditionDescription,
        /** @brief Locale-aware temperature including its unit. */
        TemperatureText,
        /** @brief Locale-aware precipitation percentage. */
        RainChanceText
    };

    /** @brief Constructs an empty hourly forecast model. @param[in] parent Optional QObject owner. @note Construct on the intended affinity thread; exceptions may propagate. */
    explicit HourlyForecastViewModel(QObject *parent = nullptr);
    /** @brief Destroys the model and owned forecast records. @note Implicitly noexcept. */
    ~HourlyForecastViewModel() override = default;

    /** @brief Returns the number of top-level hourly rows. @param[in] parent Parent index; valid parents have no children. @return Row count, or zero for a valid parent. @note Affinity-thread only; exceptions do not propagate. */
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex{}) const override;
    /** @brief Resolves one row and role to a QML-compatible value. @param[in] index Requested model index. @param[in] role ForecastRoles value. @return Role value, or an invalid QVariant for invalid input. @note Affinity-thread only; exceptions may propagate. */
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    /** @brief Returns stable QML names for all forecast roles. @return Mapping from integer role IDs to property names. @note Affinity-thread only; exceptions may propagate. */
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    /** @brief Replaces the model with up to 24 future hourly records. @param[in] newData Forecast records transferred by value. @return Nothing. @note Affinity-thread only; performs a full model reset and may throw. */
    void updateForecastData(std::vector<HourlyForecastData> newData);
    /** @brief Notifies QML that locale-dependent roles must be read again. */
    void retranslate();

private:
    std::vector<HourlyForecastData> forecastData_{};
};
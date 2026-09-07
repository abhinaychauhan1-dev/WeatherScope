/**
 * @file     : weather_shader_params.cpp
 * @brief    : Implements temperature-based shader parameter access.
 * @details  : Provides the definitions declared in
 * weather_shader_params.h. The implementation exposes immutable ViewModel
 * constants consumed by EarthWeatherMaterial.qml without retaining view state.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

#include "viewmodel/weather_shader_params.h"

/**
 * @brief Establishes an immutable shader-parameter object.
 * @param[in] parent Optional QObject owner responsible for lifecycle management.
 * @pre parent, when non-null, belongs to the same thread as the new object.
 * @post Temperature thresholds and weights retain their declared constant values
 * for the complete object lifetime.
 * @note Construction follows QObject thread-affinity and ownership rules.
 */
WeatherShaderParams::WeatherShaderParams(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief Returns the ordered temperature breakpoints used by the material.
 * @return A value copy containing Celsius thresholds from coldest to hottest.
 * @pre The object has completed construction.
 * @post Internal immutable state is unchanged.
 * @note Non-throwing; returning by value prevents QML consumers from aliasing state.
 */
QVector4D WeatherShaderParams::temperatureThresholds() const noexcept
{
    return temperatureThresholds_;
}

/**
 * @brief Returns the color-blending weights paired with the temperature bands.
 * @return A value copy of the dimensionless shader weights.
 * @pre The object has completed construction.
 * @post Internal immutable state is unchanged.
 * @note Non-throwing; vector component order matches the shader uniform contract.
 */
QVector4D WeatherShaderParams::temperatureWeights() const noexcept
{
    return temperatureWeights_;
}
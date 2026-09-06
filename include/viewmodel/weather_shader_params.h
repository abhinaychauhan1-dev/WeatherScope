/**
 * @file     : weather_shader_params.h
 * @brief    : Declares temperature-based rendering parameters for QML.
 * @details  : Exposes immutable threshold and blend-weight vectors consumed by
 * the Earth weather material. Architecture role: rendering ViewModel in the
 * MVVM presentation layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include <QObject>
#include <QVector4D>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Supplies immutable temperature-color mapping constants to QML shaders.
 * @details Values are fixed at construction and exposed as CONSTANT properties,
 * so consumers require no change notifications.
 * @note Read access is safe after construction because state is immutable;
 * QObject lifecycle operations remain affinity-thread bound. QObject disables
 * copy and move. The C++ creator controls lifetime through optional parenting.
 */
class WeatherShaderParams : public QObject
{
    /** @brief Enables Qt property metadata. */
    Q_OBJECT
    /** @brief Registers WeatherShaderParams in the WeatherGlobe QML module. */
    QML_ELEMENT
    /** @brief Prevents QML construction because WeatherMainViewModel owns the instance. */
    QML_UNCREATABLE("WeatherShaderParams is owned by WeatherMainViewModel")

    /** @brief Exposes four temperature breakpoints in degrees Celsius. @note Read-only and constant for object lifetime. */
    Q_PROPERTY(QVector4D temperatureThresholds READ temperatureThresholds NOTIFY temperatureThresholdsChanged)
    /** @brief Exposes four dimensionless color-blending weights. @note Read-only and constant for object lifetime. */
    Q_PROPERTY(QVector4D temperatureWeights READ temperatureWeights NOTIFY temperatureWeightsChanged)

public:
    /** @brief Constructs immutable shader parameters. @param[in] parent Optional QObject owner. @note Construct on the intended affinity thread; exceptions may propagate. */
    explicit WeatherShaderParams(QObject *parent = nullptr);
    /** @brief Destroys the parameter object. @note Implicitly noexcept. */
    ~WeatherShaderParams() override = default;

    /** @brief Returns ordered temperature breakpoints. @return Vector (-10, 5, 24, 36) in degrees Celsius. @note Non-throwing; returned by value for safe QML transfer. */
    [[nodiscard]] QVector4D temperatureThresholds() const noexcept;
    /** @brief Returns shader color-blending weights. @return Dimensionless vector (0.34, 0.12, 0.38, 0.28). @note Non-throwing; returned by value for safe QML transfer. */
    [[nodiscard]] QVector4D temperatureWeights() const noexcept;

signals:
    /** @brief Retained for the immutable threshold property contract. */
    void temperatureThresholdsChanged();
    /** @brief Retained for the immutable weight property contract. */
    void temperatureWeightsChanged();

private:
    const QVector4D temperatureThresholds_{-10.0F, 5.0F, 24.0F, 36.0F};
    const QVector4D temperatureWeights_{0.34F, 0.12F, 0.38F, 0.28F};
};
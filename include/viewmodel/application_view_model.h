/**
 * @file     : application_view_model.h
 * @brief    : Declares the application-level ViewModel foundation.
 * @details  : Provides a QML-registerable root for future application-scoped
 * presentation state. Architecture role: ViewModel in the MVVM layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

/**
 * @brief Represents application-wide presentation state exposed to QML.
 * @details The current type establishes the QML registration and QObject
 * lifecycle boundary without adding mutable application state.
 * @note Not thread-safe; use on its QObject affinity thread. Parent ownership
 * is supported. QObject disables copy and move to preserve stable identity.
 */
class ApplicationViewModel : public QObject
{
    /** @brief Enables Qt meta-object behavior. */
    Q_OBJECT
    /** @brief Registers ApplicationViewModel in the WeatherGlobe QML module. */
    QML_ELEMENT

public:
    /** @brief Constructs the application-level ViewModel. @param[in] parent Optional QObject owner. @note Construct on the intended affinity thread; exceptions may propagate. */
    explicit ApplicationViewModel(QObject *parent = nullptr);
};
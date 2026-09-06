/**
 * @file     : i_localization_service.h
 * @brief    : Declares the application localization service abstraction.
 * @details  : Keeps ViewModels independent of translator and QML-engine details
 * while exposing successful language changes through the Qt meta-object system.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#pragma once

#include <QObject>

#include <cstdint>

/** @brief Identifies every translation catalog packaged with the application. */
enum class AppLanguage : std::int32_t
{
    English = 0,
    German,
    French,
    Chinese,
    Dutch,
    Norwegian,
    Swedish,
    Japanese,
    Korean,
    Spanish,
    Italian
};

/**
 * @brief Abstracts runtime language activation for presentation consumers.
 * @note Implementations and consumers must use the GUI thread.
 */
class ILocalizationService : public QObject
{
    Q_OBJECT

public:
    explicit ILocalizationService(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~ILocalizationService() override = default;

    [[nodiscard]] virtual bool switchLanguage(AppLanguage language) = 0;

signals:
    /** @brief Signals that a translator and default locale were committed. */
    void languageChanged();
};

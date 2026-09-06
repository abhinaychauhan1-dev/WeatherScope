/**
 * @file     : localization_manager.h
 * @brief    : Declares runtime application translation management.
 * @details  : Owns the active Qt translator and refreshes the QML engine when
 * the user selects another catalog. Architecture role: localization Service in
 * the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include "model/i_localization_service.h"

#include <memory>

class QTranslator;

/**
 * @brief Manages the single active application translation catalog.
 * @details Replaces the installed translator and requests QML retranslation
 * only after a requested catalog loads successfully.
 * @note Not thread-safe; switchLanguage() must run on the GUI thread. The QML
 * engine is observed but not owned, while the active translator is exclusively
 * owned. Copying and moving are explicitly disabled to preserve that identity.
 */
class LocalizationManager final : public ILocalizationService
{
    /** @brief Enables the languageChanged() Qt signal. */
    Q_OBJECT

public:
    /** @brief Constructs an inactive localization service. @param[in] parent Optional QObject owner. @note Construct on the GUI thread; exceptions may propagate. */
    explicit LocalizationManager(QObject *parent = nullptr);
    /** @brief Removes the active translator and destroys the manager. @note Call on the GUI thread; implicitly noexcept. */
    ~LocalizationManager() override;

    /** @brief Copy construction is disabled to preserve translator ownership. */
    LocalizationManager(const LocalizationManager &) = delete;
    /** @brief Copy assignment is disabled to preserve QObject identity. */
    LocalizationManager &operator=(const LocalizationManager &) = delete;
    /** @brief Move construction is disabled because QObject instances have stable identity. */
    LocalizationManager(LocalizationManager &&) = delete;
    /** @brief Move assignment is disabled because QObject instances have stable identity. */
    LocalizationManager &operator=(LocalizationManager &&) = delete;

    /** @brief Loads and activates a packaged language catalog. @param[in] language Requested catalog identifier. @return true when activation succeeds. @note GUI-thread only; returns false for invalid languages, missing engine, wrong thread, or load/install failure. */
    [[nodiscard]] bool switchLanguage(AppLanguage language);

private:
    std::unique_ptr<QTranslator> activeTranslator_{};
};
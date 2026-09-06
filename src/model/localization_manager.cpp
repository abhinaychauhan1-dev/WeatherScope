/**
 * @file     : localization_manager.cpp
 * @brief    : Implements runtime application translation management.
 * @details  : Provides the definitions declared in localization_manager.h and
 * coordinates catalog lookup, translator replacement, locale selection, and
 * QML retranslation on the GUI thread.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "model/localization_manager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLocale>
#include <QString>
#include <QThread>
#include <QTranslator>

#include <utility>

namespace {
/** @brief Maps a supported language to its embedded QM catalog. @param[in] language Language identifier. @return Resource path, or an empty string for an invalid enum value. */
[[nodiscard]] QString catalogPath(const AppLanguage language)
{
    switch (language) {
    case AppLanguage::English:
        return QStringLiteral(":/i18n/weather-globe-qt_en.qm");
    case AppLanguage::German:
        return QStringLiteral(":/i18n/weather-globe-qt_de.qm");
    case AppLanguage::French:
        return QStringLiteral(":/i18n/weather-globe-qt_fr.qm");
    case AppLanguage::Chinese:
        return QStringLiteral(":/i18n/weather-globe-qt_zh_CN.qm");
    case AppLanguage::Dutch:
        return QStringLiteral(":/i18n/weather-globe-qt_nl.qm");
    case AppLanguage::Norwegian:
        return QStringLiteral(":/i18n/weather-globe-qt_nb.qm");
    case AppLanguage::Swedish:
        return QStringLiteral(":/i18n/weather-globe-qt_sv.qm");
    case AppLanguage::Japanese:
        return QStringLiteral(":/i18n/weather-globe-qt_ja.qm");
    case AppLanguage::Korean:
        return QStringLiteral(":/i18n/weather-globe-qt_ko.qm");
    case AppLanguage::Spanish:
        return QStringLiteral(":/i18n/weather-globe-qt_es.qm");
    case AppLanguage::Italian:
        return QStringLiteral(":/i18n/weather-globe-qt_it.qm");
    }

    // A corrupted or future enum value must not select an arbitrary catalog.
    return {};
}

/** @brief Maps a supported language to its QLocale identifier. @param[in] language Language identifier. @return Locale name, or an empty string for an invalid enum value. */
[[nodiscard]] QString localeName(const AppLanguage language)
{
    switch (language) {
    case AppLanguage::English:
        return QStringLiteral("en");
    case AppLanguage::German:
        return QStringLiteral("de");
    case AppLanguage::French:
        return QStringLiteral("fr");
    case AppLanguage::Chinese:
        return QStringLiteral("zh_CN");
    case AppLanguage::Dutch:
        return QStringLiteral("nl");
    case AppLanguage::Norwegian:
        return QStringLiteral("nb");
    case AppLanguage::Swedish:
        return QStringLiteral("sv");
    case AppLanguage::Japanese:
        return QStringLiteral("ja");
    case AppLanguage::Korean:
        return QStringLiteral("ko");
    case AppLanguage::Spanish:
        return QStringLiteral("es");
    case AppLanguage::Italian:
        return QStringLiteral("it");
    }

    return {};
}
} // namespace

/**
 * @brief Stores a guarded reference to the QML engine used for retranslation.
 * @param[in] engine Borrowed QML engine.
 * @param[in] parent Optional QObject lifecycle owner.
 * @pre engine outlives this manager or is destroyed as a tracked QObject.
 * @post No translator is installed by construction.
 */
LocalizationManager::LocalizationManager(QObject *parent)
    : ILocalizationService(parent)
{
}

/**
 * @brief Removes the owned translator when application infrastructure still exists.
 * @pre Destruction occurs on the QObject affinity thread.
 * @post The manager no longer contributes translations to QCoreApplication.
 */
LocalizationManager::~LocalizationManager()
{
    // QCoreApplication may already be gone during shutdown; avoid dereferencing
    // global application state after its lifetime has ended.
    if ((activeTranslator_ != nullptr) && (QCoreApplication::instance() != nullptr)) {
        static_cast<void>(QCoreApplication::removeTranslator(activeTranslator_.get()));
    }
}

/**
 * @brief Replaces the active catalog and refreshes all QML translations.
 * @param[in] language Supported application language.
 * @return true only when the requested translator is fully installed.
 * @pre Called on the GUI thread while the tracked QML engine is alive.
 * @post On success, locale, translator, QML text, and observers are synchronized;
 * on failure, the previous translator is retained or restoration is attempted.
 */
bool LocalizationManager::switchLanguage(const AppLanguage language)
{
    QCoreApplication *const application = QCoreApplication::instance();
    // Translator installation and QML retranslation are GUI-thread operations.
    if ((application == nullptr)
        || (QThread::currentThread() != application->thread())) {
        return false;
    }

    const QString resourcePath = catalogPath(language);
    const QString requestedLocaleName = localeName(language);
    // Both mappings must recognize the enum before any global state is changed.
    if (resourcePath.isEmpty() || requestedLocaleName.isEmpty()) {
        qDebug() << "LocalizationManager rejected unsupported language value:"
                 << static_cast<std::int32_t>(language);
        return false;
    }

    const QDir translationDirectory{QStringLiteral(":/i18n")};
    qDebug() << "LocalizationManager resource catalogs:"
             << translationDirectory.entryList(QDir::Files, QDir::Name);
    qDebug().noquote() << "LocalizationManager loading locale"
                       << requestedLocaleName << "from" << resourcePath;

    auto newTranslator = std::make_unique<QTranslator>();
    const bool loadSucceeded = newTranslator->load(resourcePath);
    qDebug() << "QTranslator::load() returned" << loadSucceeded;
    if (!loadSucceeded) {
        qDebug().noquote() << "Translation catalog load failed. Absolute resource path:"
                           << translationDirectory.absoluteFilePath(
                                  QStringLiteral("weather-globe-qt_%1.qm")
                                      .arg(requestedLocaleName))
                           << "locale:" << requestedLocaleName;
        return false;
    }

    // Load first, then replace: a missing/corrupt catalog leaves the active translator intact.
    if (activeTranslator_ != nullptr) {
        const bool removalSucceeded =
            QCoreApplication::removeTranslator(activeTranslator_.get());
        qDebug() << "QCoreApplication::removeTranslator() returned" << removalSucceeded;
        if (!removalSucceeded) {
            return false;
        }
    }

    const bool installationSucceeded =
        QCoreApplication::installTranslator(newTranslator.get());
    qDebug() << "QCoreApplication::installTranslator() returned" << installationSucceeded;
    if (!installationSucceeded) {
        // Best-effort rollback restores the previous user-visible language.
        if (activeTranslator_ != nullptr) {
            const bool restorationSucceeded =
                QCoreApplication::installTranslator(activeTranslator_.get());
            qDebug() << "Previous translator restoration returned" << restorationSucceeded;
        }
        return false;
    }

    activeTranslator_ = std::move(newTranslator);
    QLocale::setDefault(QLocale{requestedLocaleName});
    // Notify only after all translation consumers observe the committed locale.
    emit languageChanged();
    return true;
}
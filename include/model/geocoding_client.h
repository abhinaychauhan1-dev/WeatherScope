/**
 * @file     : geocoding_client.h
 * @brief    : Declares the Qt HTTP geocoding service adapter.
 * @details  : Provides debounced forward geocoding and localized reverse
 * geocoding through Qt Network. Architecture role: Service adapter in the
 * MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include "model/i_geocoding_service.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include <vector>

class QNetworkReply;

/**
 * @brief Implements asynchronous geocoding over Qt Network.
 * @details Searches are debounced and previous in-flight requests may be
 * replaced; parsed results are delivered through registered callbacks.
 * @note Not thread-safe. Construct and call on the QObject affinity thread.
 * QObject disables copying and moving. Qt parent ownership controls lifetime;
 * active replies are observed through non-owning QPointer instances.
 */
class GeocodingClient final : public QObject, public IGeocodingService
{
    /** @brief Enables Qt signals and meta-object behavior. */
    Q_OBJECT

public:
    /** @brief Constructs the service and configures search debouncing. @param[in] parent Optional QObject owner. @note Call on the intended event-loop thread; exceptions may propagate. */
    explicit GeocodingClient(QObject *parent = nullptr);
    /** @brief Destroys the service and its QObject-owned resources. @note Implicitly noexcept; pending QPointers become irrelevant. */
    ~GeocodingClient() override = default;

    /** @brief Schedules a debounced city search. @param[in] query Search text; fewer than three trimmed characters cancel dispatch. @return Nothing. @note Affinity-thread only; exceptions may propagate. */
    void search(const QString &query) override;
    /** @brief Starts localized reverse geocoding. @param[in] latitude Latitude in decimal degrees. @param[in] longitude Longitude in decimal degrees. @return Nothing. @note The caller must supply valid coordinates; affinity-thread only. */
    void resolveLocalizedLocation(double latitude, double longitude) override;
    /** @brief Sets the locale for subsequent requests. @param[in] languageCode ISO 639 code. @return Nothing. @note Affinity-thread only; unsupported values are normalized by the implementation. */
    void setLanguageCode(const QString &languageCode) override;
    /** @brief Replaces the forward-search callback. @param[in] handler Callback to retain, or empty to detach. @return Nothing. @note Affinity-thread only. */
    void setResultsHandler(ResultsHandler handler) noexcept override;
    /** @brief Replaces the reverse-geocoding callback. @param[in] handler Callback to retain, or empty to detach. @return Nothing. @note Affinity-thread only. */
    void setLocationResolvedHandler(LocationResolvedHandler handler) noexcept override;

signals:
    /** @brief Publishes parsed search results to internal Qt connections. @param[out] results Immutable result batch. @return Nothing. @note Emitted on this object's affinity thread. */
    void locationResultsReady(const std::vector<LocationResult> &results);

private:
    /** @brief Dispatches the pending query after the debounce interval. @return Nothing. @note Affinity-thread only; replaces any active search reply. */
    void requestLocations();
    /** @brief Validates and parses a completed forward-geocoding reply. @param[in,out] reply Completed network reply scheduled for deletion after processing. @param[in] requestLanguageCode Locale used by this request. @return Nothing. @note Affinity-thread only. */
    void handleReply(QNetworkReply &reply, const QString &requestLanguageCode);
    /** @brief Validates and parses a completed reverse-geocoding reply. @param[in,out] reply Completed network reply scheduled for deletion after processing. @param[in] requestLanguageCode Locale used by this request. @return Nothing. @note Affinity-thread only. */
    void handleLocalizedLocationReply(
        QNetworkReply &reply,
        const QString &requestLanguageCode);

    QNetworkAccessManager networkAccessManager_{};
    QTimer debounceTimer_{};
    QPointer<QNetworkReply> activeReply_{};
    QPointer<QNetworkReply> activeLocalizedLocationReply_{};
    QString pendingQuery_{};
    QString languageCode_{QStringLiteral("en")};
    ResultsHandler resultsHandler_{};
    LocationResolvedHandler locationResolvedHandler_{};
};
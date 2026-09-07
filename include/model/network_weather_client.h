/**
 * @file     : network_weather_client.h
 * @brief    : Declares the Qt HTTP weather client adapter.
 * @details  : Polls Open-Meteo through Qt Network and publishes serialized
 * payloads or transport errors. Architecture role: Service adapter in the
 * MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include "model/i_weather_client.h"

#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>

class QNetworkReply;

/**
 * @brief Implements timer-driven asynchronous weather retrieval.
 * @details Validates polling inputs, issues one active request at a time, and
 * forwards successful payloads without parsing repository-domain data.
 * @note Not thread-safe; all methods require the QObject affinity thread.
 * QObject disables copy and move. The client owns its manager and timer while
 * observing the manager-owned active reply through QPointer.
 */
class NetworkWeatherClient final : public QObject, public IWeatherClient
{
    /** @brief Enables Qt timer and network-reply signal connections. */
    Q_OBJECT

public:
    /** @brief Constructs an idle weather client. @param[in] parent Optional QObject owner. @note Construct on the intended event-loop thread; exceptions may propagate. */
    explicit NetworkWeatherClient(QObject *parent = nullptr);
    /** @brief Destroys the client and QObject-owned resources. @note Implicitly noexcept. */
    ~NetworkWeatherClient() override = default;

    /** @brief Starts immediate and periodic requests. @param[in] latitude Latitude in [-90, 90] degrees. @param[in] longitude Longitude in [-180, 180] degrees. @param[in] intervalMilliseconds Positive interval in milliseconds. @return Nothing. @note Affinity-thread only; invalid input is reported without starting polling. */
    void startPolling(
        double latitude,
        double longitude,
        std::int32_t intervalMilliseconds) override;
    /** @brief Stops the timer and aborts the active reply. @return Nothing. @note Affinity-thread only and non-throwing. */
    void stopPolling() noexcept override;
    /** @brief Reports timer activity. @return true while periodic polling is active. @note Affinity-thread only and non-throwing. */
    [[nodiscard]] bool isPolling() const noexcept override;
    /** @brief Sets the locale for subsequent requests. @param[in] languageCode ISO 639 code. @return Nothing. @note Affinity-thread only; empty input preserves the previous locale. */
    void setLanguageCode(const QString &languageCode) override;
    /** @brief Replaces the payload callback. @param[in] handler Callback to retain, or empty to detach. @return Nothing. @note Affinity-thread only. */
    void setWeatherDataHandler(WeatherDataHandler handler) noexcept override;
    /** @brief Replaces the error callback. @param[in] handler Callback to retain, or empty to detach. @return Nothing. @note Affinity-thread only. */
    void setErrorHandler(ErrorHandler handler) noexcept override;

private:
    /** @brief Issues a request for the stored polling coordinates. @return Nothing. @note Affinity-thread only; avoids overlapping active requests. */
    void requestWeather();
    /** @brief Validates and forwards a completed reply payload. @param[in,out] reply Completed reply scheduled for deletion after processing. @return Nothing. @note Affinity-thread only. */
    void handleReply(QNetworkReply &reply);
    /** @brief Publishes an error when a handler is installed. @param[in] message Human-readable failure description. @return Nothing. @note Affinity-thread only; exceptions from user callbacks may propagate. */
    void reportError(const QString &message) const;

    QNetworkAccessManager networkAccessManager_;
    QTimer pollingTimer_;
    QPointer<QNetworkReply> activeReply_;
    WeatherDataHandler weatherDataHandler_;
    ErrorHandler errorHandler_;
    QString languageCode_{QStringLiteral("en")};
    double latitude_{0.0};
    double longitude_{0.0};
};
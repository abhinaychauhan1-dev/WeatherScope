/**
 * @file     : network_weather_client.cpp
 * @brief    : Implements remote weather forecast retrieval over HTTP.
 * @details  : Provides the definitions declared in network_weather_client.h,
 * including polling lifecycle, Open-Meteo request construction, reply cleanup,
 * and callback-based payload/error delivery.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "model/network_weather_client.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include <chrono>
#include <utility>

namespace
{
constexpr double minimumLatitude{-90.0};
constexpr double maximumLatitude{90.0};
constexpr double minimumLongitude{-180.0};
constexpr double maximumLongitude{180.0};
constexpr std::int32_t minimumPollingIntervalMilliseconds{1};
constexpr std::chrono::seconds networkTransferTimeout{30};
} // namespace

/**
 * @brief Connects the polling timer to asynchronous request dispatch.
 * @param[in] parent Optional QObject lifecycle owner.
 * @pre Construction occurs on the intended Qt event-loop thread.
 * @post The client is idle and timer expirations call requestWeather().
 */
NetworkWeatherClient::NetworkWeatherClient(QObject *parent)
    : QObject(parent)
{
    connect(&pollingTimer_, &QTimer::timeout, this, &NetworkWeatherClient::requestWeather);
}

/**
 * @brief Validates polling inputs, stores them, and requests weather immediately.
 * @param[in] latitude Latitude in decimal degrees.
 * @param[in] longitude Longitude in decimal degrees.
 * @param[in] intervalMilliseconds Positive polling interval in milliseconds.
 * @pre Called on the QObject affinity thread.
 * @post Valid input starts the timer; invalid input preserves prior polling state.
 */
void NetworkWeatherClient::startPolling(
    const double latitude,
    const double longitude,
    const std::int32_t intervalMilliseconds)
{
    // Ordered bounds reject infinities and NaN while enforcing the geographic domain.
    const bool coordinatesAreValid =
        (latitude >= minimumLatitude) && (latitude <= maximumLatitude)
        && (longitude >= minimumLongitude) && (longitude <= maximumLongitude);

    if (!coordinatesAreValid) {
        reportError(QStringLiteral("Latitude or longitude is outside its valid range."));
        return;
    }

    // QTimer cannot represent a meaningful non-positive polling cadence.
    if (intervalMilliseconds < minimumPollingIntervalMilliseconds) {
        reportError(QStringLiteral("The polling interval must be positive."));
        return;
    }

    // A location change supersedes any reply for the previous coordinates.
    // Clear first so the obsolete completion cannot clear the replacement reply.
    if (activeReply_ != nullptr) {
        QNetworkReply *const obsoleteReply = activeReply_.data();
        activeReply_.clear();
        obsoleteReply->abort();
    }

    latitude_ = latitude;
    longitude_ = longitude;
    pollingTimer_.start(std::chrono::milliseconds{intervalMilliseconds});
    requestWeather();
}

/** @brief Stops periodic and in-flight retrieval. @pre Affinity-thread access. @post The timer is inactive and any active reply is aborting. @note Non-throwing. */
void NetworkWeatherClient::stopPolling() noexcept
{
    pollingTimer_.stop();

    if (activeReply_ != nullptr) {
        QNetworkReply *const obsoleteReply = activeReply_.data();
        activeReply_.clear();
        obsoleteReply->abort();
    }
}

/** @brief Reports timer-driven polling state. @return true while the timer is active. @pre Affinity-thread access. @post State is unchanged. @note Non-throwing. */
bool NetworkWeatherClient::isPolling() const noexcept
{
    return pollingTimer_.isActive();
}

/** @brief Normalizes the locale used by future requests. @param[in] languageCode ISO language code. @pre Affinity-thread access. @post Non-empty input replaces the current locale. */
void NetworkWeatherClient::setLanguageCode(const QString &languageCode)
{
    const QString normalizedCode = languageCode.trimmed().toLower();
    if (!normalizedCode.isEmpty()) {
        languageCode_ = normalizedCode;
    }
}

/** @brief Installs the successful-payload callback. @param[in] handler Replacement callback or empty callable. @pre Affinity-thread access. @post The prior callback is released. */
void NetworkWeatherClient::setWeatherDataHandler(WeatherDataHandler handler) noexcept
{
    weatherDataHandler_ = std::move(handler);
}

/** @brief Installs the transport-error callback. @param[in] handler Replacement callback or empty callable. @pre Affinity-thread access. @post The prior callback is released. */
void NetworkWeatherClient::setErrorHandler(ErrorHandler handler) noexcept
{
    errorHandler_ = std::move(handler);
}

/**
 * @brief Issues one Open-Meteo request for the stored location.
 * @pre Polling inputs were validated and execution is on the affinity thread.
 * @post At most one active reply is observed until its finished callback runs.
 */
void NetworkWeatherClient::requestWeather()
{
    // Serialize requests so timer ticks cannot overlap a slow network response.
    if (activeReply_ != nullptr) {
        return;
    }

    QUrl endpoint{QStringLiteral("https://api.open-meteo.com/v1/forecast")};
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), QString::number(latitude_));
    query.addQueryItem(QStringLiteral("longitude"), QString::number(longitude_));
    query.addQueryItem(
        QStringLiteral("current"),
        QStringLiteral(
            "temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,"
            "weather_code,cloud_cover,pressure_msl,wind_speed_10m,wind_direction_10m"));
    query.addQueryItem(
        QStringLiteral("hourly"),
        QStringLiteral(
            "temperature_2m,weather_code,precipitation_probability,wind_speed_10m"));
    query.addQueryItem(
        QStringLiteral("daily"),
        QStringLiteral(
            "weather_code,temperature_2m_max,temperature_2m_min,"
            "precipitation_probability_max"));
            query.addQueryItem(QStringLiteral("language"), languageCode_);
            // UTC gives parser and ViewModel filtering a single unambiguous time basis.
    query.addQueryItem(QStringLiteral("timezone"), QStringLiteral("UTC"));
    endpoint.setQuery(query);

    QNetworkRequest request{endpoint};
    request.setTransferTimeout(networkTransferTimeout);

    // QNetworkAccessManager owns the reply; this pointer only observes it until deleteLater().
    QNetworkReply *const reply = networkAccessManager_.get(request);
    activeReply_ = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // The context-bound connection prevents delivery after this client is destroyed.
        if (activeReply_ == reply) {
            handleReply(*reply);
            activeReply_.clear();
        }
        reply->deleteLater();
    });
}

/**
 * @brief Delivers a successful payload or reports an unexpected network failure.
 * @param[in,out] reply Completed reply whose body may be consumed.
 * @pre reply is valid and finished on the affinity thread.
 * @post Exactly one success/error path runs; expected cancellation remains silent.
 */
void NetworkWeatherClient::handleReply(QNetworkReply &reply)
{
    if (reply.error() == QNetworkReply::NoError) {
        if (weatherDataHandler_) {
            weatherDataHandler_(reply.readAll());
        }
        return;
    }

    // Aborts are an intentional consequence of stop/replacement, not user-facing failures.
    if (reply.error() != QNetworkReply::OperationCanceledError) {
        reportError(reply.errorString());
    }
}

/** @brief Forwards an error when a consumer is installed. @param[in] message Human-readable failure. @pre Affinity-thread access. @post The callback has run at most once. */
void NetworkWeatherClient::reportError(const QString &message) const
{
    if (errorHandler_) {
        errorHandler_(message);
    }
}
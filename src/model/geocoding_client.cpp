/**
 * @file     : geocoding_client.cpp
 * @brief    : Implements debounced location retrieval from Open-Meteo.
 * @details  : Provides the definitions declared in geocoding_client.h,
 * including forward search, localized reverse geocoding, and Qt-managed reply
 * lifecycle handling on the object's affinity thread.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "model/geocoding_client.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include <chrono>
#include <cmath>
#include <utility>

namespace
{
constexpr qsizetype minimumQueryLength{3};
constexpr std::int32_t debounceDelayMilliseconds{300};
constexpr std::int32_t maximumResultCount{5};
constexpr double minimumLatitude{-90.0};
constexpr double maximumLatitude{90.0};
constexpr double minimumLongitude{-180.0};
constexpr double maximumLongitude{180.0};
constexpr std::chrono::seconds networkTransferTimeout{30};

[[nodiscard]] bool coordinatesAreValid(
    const double latitude,
    const double longitude) noexcept
{
    return std::isfinite(latitude) && std::isfinite(longitude)
        && (latitude >= minimumLatitude) && (latitude <= maximumLatitude)
        && (longitude >= minimumLongitude) && (longitude <= maximumLongitude);
}

/**
 * @brief Converts a successful Open-Meteo geocoding reply into domain results.
 * @param[in,out] reply Completed reply whose payload is consumed by readAll().
 * @return Valid locations, or an empty collection for transport/schema failure.
 * @pre reply remains alive for the duration of this call.
 * @post The reply payload has been consumed; malformed entries are omitted.
 */
[[nodiscard]] std::vector<LocationResult> parseLocations(QNetworkReply &reply)
{
    // Transport failures are represented as no suggestions; this adapter has no error channel.
    if (reply.error() != QNetworkReply::NoError) {
        return {};
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(reply.readAll(), &parseError);
    if ((parseError.error != QJsonParseError::NoError) || !document.isObject()) {
        return {};
    }

    const QJsonValue resultsValue = document.object().value(QStringLiteral("results"));
    if (!resultsValue.isArray()) {
        return {};
    }

    const QJsonArray resultArray = resultsValue.toArray();
    std::vector<LocationResult> results{};
    results.reserve(static_cast<std::size_t>(resultArray.size()));

    for (const QJsonValue &resultValue : resultArray) {
        // A malformed provider entry must not invalidate other independently usable results.
        if (!resultValue.isObject()) {
            continue;
        }

        const QJsonObject resultObject = resultValue.toObject();
        const QJsonValue cityName = resultObject.value(QStringLiteral("name"));
        const QJsonValue country = resultObject.value(QStringLiteral("country"));
        const QJsonValue countryCode = resultObject.value(QStringLiteral("country_code"));
        const QJsonValue adminArea = resultObject.value(QStringLiteral("admin1"));
        const QJsonValue latitude = resultObject.value(QStringLiteral("latitude"));
        const QJsonValue longitude = resultObject.value(QStringLiteral("longitude"));

        if (!cityName.isString() || !country.isString() || !latitude.isDouble()
            || !longitude.isDouble()) {
            continue;
        }

        const double latitudeValue = latitude.toDouble();
        const double longitudeValue = longitude.toDouble();
        if (!coordinatesAreValid(latitudeValue, longitudeValue)) {
            continue;
        }

        // City, country, and coordinates identify a usable result; country code
        // and administrative area are optional enrichment fields.
        LocationResult result{};
        result.cityName = cityName.toString();
        result.country = country.toString();
        result.countryCode = countryCode.isString()
            ? countryCode.toString().trimmed().toUpper()
            : QString{};
        result.adminArea = adminArea.isString() ? adminArea.toString() : QString{};
        result.latitude = latitudeValue;
        result.longitude = longitudeValue;
        results.push_back(result);
    }

    return results;
}
} // namespace

/**
 * @brief Configures the single-shot timer used to debounce search input.
 * @param[in] parent Optional QObject lifecycle owner.
 * @pre Construction occurs on the intended Qt event-loop thread.
 * @post The client is idle and a timeout dispatches the latest pending query.
 */
GeocodingClient::GeocodingClient(QObject *parent)
    : QObject(parent)
{
    debounceTimer_.setSingleShot(true);
    debounceTimer_.setInterval(std::chrono::milliseconds{debounceDelayMilliseconds});
    connect(&debounceTimer_, &QTimer::timeout, this, &GeocodingClient::requestLocations);
}

/**
 * @brief Replaces the pending query and restarts the debounce lifecycle.
 * @param[in] query User-entered location text.
 * @pre Called on the object's affinity thread.
 * @post Any active forward request is aborted; queries of at least three
 * trimmed characters are scheduled after the debounce interval.
 */
void GeocodingClient::search(const QString &query)
{
    pendingQuery_ = query.trimmed();
    // Repeated keystrokes restart the delay so only a pause triggers network traffic.
    debounceTimer_.stop();

    // A newer query supersedes an in-flight search; its finished callback still owns cleanup.
    if (activeReply_ != nullptr) {
        QNetworkReply *const obsoleteReply = activeReply_.data();
        activeReply_.clear();
        obsoleteReply->abort();
    }

    if (pendingQuery_.size() >= minimumQueryLength) {
        debounceTimer_.start();
    }
}

/**
 * @brief Starts reverse geocoding for a previously validated location.
 * @param[in] latitude Latitude in decimal degrees.
 * @param[in] longitude Longitude in decimal degrees.
 * @pre Coordinates are valid and the call occurs on the affinity thread.
 * @post The previous reverse lookup is aborted and a new reply is observed.
 */
void GeocodingClient::resolveLocalizedLocation(
    const double latitude,
    const double longitude)
{
    if (!coordinatesAreValid(latitude, longitude)) {
        return;
    }

    // Only the newest localization request may update the selected location label.
    if (activeLocalizedLocationReply_ != nullptr) {
        activeLocalizedLocationReply_->abort();
    }

    QUrl endpoint{QStringLiteral("https://nominatim.openstreetmap.org/reverse")};
    QUrlQuery query;
    // Seven decimal places preserve sub-metre coordinate precision without
    // exposing locale-dependent number formatting to the HTTP API.
    query.addQueryItem(QStringLiteral("lat"), QString::number(latitude, 'f', 7));
    query.addQueryItem(QStringLiteral("lon"), QString::number(longitude, 'f', 7));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("jsonv2"));
    query.addQueryItem(QStringLiteral("zoom"), QStringLiteral("10"));
    query.addQueryItem(QStringLiteral("addressdetails"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("accept-language"), languageCode_);
    endpoint.setQuery(query);

    QNetworkRequest request{endpoint};
    request.setRawHeader("Accept-Language", languageCode_.toUtf8());
    request.setHeader(
        QNetworkRequest::UserAgentHeader,
        QStringLiteral("WeatherScope/0.1"));
    request.setTransferTimeout(networkTransferTimeout);
    const QString requestLanguageCode = languageCode_;
    QNetworkReply *const reply = networkAccessManager_.get(request);
    activeLocalizedLocationReply_ = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestLanguageCode]() {
        // Ignore completion from an aborted/replaced lookup, but always defer
        // deletion because QNetworkAccessManager owns the reply object.
        if (activeLocalizedLocationReply_ == reply) {
            handleLocalizedLocationReply(*reply, requestLanguageCode);
            activeLocalizedLocationReply_.clear();
        }
        reply->deleteLater();
    });
}

/**
 * @brief Normalizes the service locale and cancels requests using the old locale.
 * @param[in] languageCode ISO language code.
 * @pre Called on the object's affinity thread.
 * @post A non-empty changed code becomes active; otherwise state is unchanged.
 */
void GeocodingClient::setLanguageCode(const QString &languageCode)
{
    const QString normalizedCode = languageCode.trimmed().toLower();
    if (normalizedCode.isEmpty() || (languageCode_ == normalizedCode)) {
        return;
    }

    languageCode_ = normalizedCode;
    // Results from the previous locale must not be presented after a language switch.
    if (activeReply_ != nullptr) {
        activeReply_->abort();
    }
    if (activeLocalizedLocationReply_ != nullptr) {
        activeLocalizedLocationReply_->abort();
    }
}

/** @brief Installs the forward-search result callback. @param[in] handler Replacement callback or empty callable. @pre Affinity-thread access. @post The previous callback is released. */
void GeocodingClient::setResultsHandler(ResultsHandler handler) noexcept
{
    resultsHandler_ = std::move(handler);
}

/** @brief Installs the reverse-geocoding result callback. @param[in] handler Replacement callback or empty callable. @pre Affinity-thread access. @post The previous callback is released. */
void GeocodingClient::setLocationResolvedHandler(LocationResolvedHandler handler) noexcept
{
    locationResolvedHandler_ = std::move(handler);
}

/**
 * @brief Issues the latest debounced forward-geocoding request.
 * @pre The debounce timer fired on the affinity thread and pendingQuery_ is usable.
 * @post activeReply_ observes a manager-owned reply until completion cleanup.
 */
void GeocodingClient::requestLocations()
{
    QUrl endpoint{QStringLiteral("https://geocoding-api.open-meteo.com/v1/search")};
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("name"), pendingQuery_);
    query.addQueryItem(QStringLiteral("count"), QString::number(maximumResultCount));
    query.addQueryItem(QStringLiteral("language"), languageCode_);
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    endpoint.setQuery(query);

    QNetworkRequest request{endpoint};
    request.setTransferTimeout(networkTransferTimeout);
    QNetworkReply *const reply = networkAccessManager_.get(request);
    activeReply_ = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // Only the latest query may publish results or clear the tracked reply.
        if (activeReply_ == reply) {
            handleReply(*reply);
            activeReply_.clear();
        }
        reply->deleteLater();
    });
}

/**
 * @brief Publishes parsed forward-geocoding results through both callback APIs.
 * @param[in,out] reply Completed reply whose body is consumed.
 * @pre reply is valid and finished on the affinity thread.
 * @post Registered consumers and Qt observers receive the same result snapshot.
 */
void GeocodingClient::handleReply(QNetworkReply &reply)
{
    const std::vector<LocationResult> results = parseLocations(reply);

    if (resultsHandler_) {
        resultsHandler_(results);
    }
    // Preserve the Qt signal path for observers that do not use the service interface callback.
    emit locationResultsReady(results);
}

/**
 * @brief Parses and publishes a localized reverse-geocoding response.
 * @param[in,out] reply Completed reply whose body is consumed on success.
 * @pre reply is valid and finished on the affinity thread.
 * @post A callback occurs only when identity and coordinates are complete and valid.
 */
void GeocodingClient::handleLocalizedLocationReply(
    QNetworkReply &reply,
    const QString &requestLanguageCode)
{
    // Without a successful transport and consumer there is no useful work to perform.
    if ((reply.error() != QNetworkReply::NoError) || !locationResolvedHandler_) {
        return;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(reply.readAll(), &parseError);
    if ((parseError.error != QJsonParseError::NoError) || !document.isObject()) {
        return;
    }

    const QJsonObject resultObject = document.object();
    const QJsonValue addressValue = resultObject.value(QStringLiteral("address"));
    if (!addressValue.isObject()) {
        return;
    }

    const QJsonObject address = addressValue.toObject();
    const QStringList cityKeys{
        QStringLiteral("city"),
        QStringLiteral("town"),
        QStringLiteral("village"),
        QStringLiteral("municipality"),
        QStringLiteral("county")};
    QString cityName{};
    // Nominatim varies the locality key by settlement type; choose the most
    // specific available label in a deterministic preference order.
    for (const QString &cityKey : cityKeys) {
        const QJsonValue cityValue = address.value(cityKey);
        if (cityValue.isString() && !cityValue.toString().trimmed().isEmpty()) {
            cityName = cityValue.toString().trimmed();
            break;
        }
    }

    const QString country = address.value(QStringLiteral("country")).toString().trimmed();
    // A location label without both city and country is unsuitable for the HUD.
    if (cityName.isEmpty() || country.isEmpty()) {
        return;
    }

    bool latitudeValid{false};
    bool longitudeValid{false};
    const double latitude = resultObject.value(QStringLiteral("lat"))
                                .toString().toDouble(&latitudeValid);
    const double longitude = resultObject.value(QStringLiteral("lon"))
                                 .toString().toDouble(&longitudeValid);
    // Reject provider coordinates that cannot be converted losslessly to numeric values.
    if (!latitudeValid || !longitudeValid || !coordinatesAreValid(latitude, longitude)) {
        return;
    }

    LocationResult result{};
    result.cityName = cityName;
    result.country = country;
    result.countryCode = address.value(QStringLiteral("country_code"))
                             .toString().trimmed().toUpper();
    result.adminArea = address.value(QStringLiteral("state")).toString().trimmed();
    result.languageCode = requestLanguageCode;
    result.latitude = latitude;
    result.longitude = longitude;
    locationResolvedHandler_(result);
}
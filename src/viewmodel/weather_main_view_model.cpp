/**
 * @file     : weather_main_view_model.cpp
 * @brief Implements presentation state and weather refresh behavior.
 * @details Provides the function definitions declared by
 * weather_main_view_model.h. This implementation adapts asynchronous Model and
 * Service results into thread-affine Qt state for the QML presentation layer.
 * @author Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @date 2026-09-06
 * @version 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "viewmodel/weather_main_view_model.h"
#include "viewmodel/weather_presentation_formatter.h"

#include <QCoreApplication>
#include <QLocale>
#include <QMetaObject>
#include <QVariantMap>

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
constexpr double minimumLatitude{-90.0};
constexpr double maximumLatitude{90.0};
constexpr double minimumLongitude{-180.0};
constexpr double maximumLongitude{180.0};
constexpr std::int32_t refreshIntervalMilliseconds{900'000};
constexpr int supportedLanguageCount{11};
constexpr double localizedLocationCoordinateTolerance{1.0};

/**
 * @brief Converts a validated language-list index to its application enum.
 * @pre languageIndex is within the range of AppLanguage enumerators.
 * @return The AppLanguage represented by languageIndex.
 */
[[nodiscard]] AppLanguage appLanguageForIndex(const int languageIndex) noexcept
{
    // The caller validates the range before this deliberate enum conversion.
    return static_cast<AppLanguage>(languageIndex);
}

/**
 * @brief Maps an application language to the ISO code expected by services.
 * @return The service-compatible ISO 639 language code.
 */
[[nodiscard]] QString languageCode(const AppLanguage language)
{
    switch (language) {
    case AppLanguage::English:
        return QStringLiteral("en");
    case AppLanguage::German:
        return QStringLiteral("de");
    case AppLanguage::French:
        return QStringLiteral("fr");
    case AppLanguage::Chinese:
        return QStringLiteral("zh");
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

    // Preserve a deterministic service locale if an invalid enum value crosses the API boundary.
    return QStringLiteral("en");
}

/**
 * @brief Validates geographic coordinates before state or service mutation.
 * @return true only for finite values inside the WGS 84 latitude/longitude bounds.
 */
[[nodiscard]] bool coordinatesAreValid(const double latitude, const double longitude) noexcept
{
    // Explicit finiteness checks prevent NaN from bypassing ordered comparisons.
    return std::isfinite(latitude) && std::isfinite(longitude)
        && (latitude >= minimumLatitude) && (latitude <= maximumLatitude)
        && (longitude >= minimumLongitude) && (longitude <= maximumLongitude);
}

/**
 * @brief Formats signed coordinates as absolute magnitudes and compass hemispheres.
 * @pre latitude and longitude have passed coordinatesAreValid().
 * @return A stable four-decimal coordinate label for the presentation layer.
 */
[[nodiscard]] QString formatLocation(const double latitude, const double longitude)
{
    const QString latitudeDirection =
        (latitude >= 0.0) ? QStringLiteral("N") : QStringLiteral("S");
    const QString longitudeDirection =
        (longitude >= 0.0) ? QStringLiteral("E") : QStringLiteral("W");
    return QStringLiteral("%1 %2, %3 %4")
        .arg(std::abs(latitude), 0, 'f', 4)
        .arg(latitudeDirection)
        .arg(std::abs(longitude), 0, 'f', 4)
        .arg(longitudeDirection);
}

    /**
     * @brief Resolves a country name in the selected language without losing known data.
     * @return A localized territory name, or fallbackName when localization is unavailable.
     */
[[nodiscard]] QString localizedCountryName(
    const QString &languageCode,
    const QString &countryCode,
    const QString &fallbackName)
{
    const QString normalizedCountryCode = countryCode.trimmed().toUpper();
    // QLocale territory lookup requires a normalized ISO alpha-2 country code.
    if (normalizedCountryCode.size() != 2) {
        return fallbackName;
    }

    const QLocale targetLocale{
        QStringLiteral("%1_%2").arg(languageCode, normalizedCountryCode)};
    const QString localizedName = targetLocale.nativeTerritoryName();
    return localizedName.isEmpty() ? fallbackName : localizedName;
}

} // namespace

/**
 * @brief Initializes injected services, callback bridges, and initial polling.
 * @details Service callbacks copy their payloads into queued lambdas so all
 * QObject state changes and signal emissions occur on this object's affinity thread.
 * @post Valid dependencies have handlers installed and weather polling is active.
 */
WeatherMainViewModel::WeatherMainViewModel(
    std::shared_ptr<IWeatherRepository> repository,
    std::shared_ptr<INetworkReachabilityService> reachabilityService,
    std::shared_ptr<IGeocodingService> geocodingService,
    ILocalizationService &localizationService,
    QObject *parent)
    : QObject(parent)
    , repository_(std::move(repository))
    , reachabilityService_(std::move(reachabilityService))
    , geocodingService_(std::move(geocodingService))
    , localizationService_(localizationService)
{
    // Fail construction atomically rather than retain a partially usable ViewModel.
    if ((repository_ == nullptr) || (reachabilityService_ == nullptr)
        || (geocodingService_ == nullptr)) {
        throw std::invalid_argument{"WeatherMainViewModel dependencies must not be null."};
    }

    try {
        QObject::connect(
            &localizationService_,
            &ILocalizationService::languageChanged,
            this,
            &WeatherMainViewModel::handleLanguageChanged);
        repository_->setLanguageCode(QStringLiteral("en"));
        geocodingService_->setLanguageCode(QStringLiteral("en"));
        isOnline_ = reachabilityService_->isReachable();
        onlineStatusText_ = isOnline_
            ? QCoreApplication::translate("WeatherMainViewModel", "Online")
            : QCoreApplication::translate("WeatherMainViewModel", "Offline");
        repository_->setForecastHandler(
        [this](const WeatherForecastData &forecast, const bool isCachedData) {
            // Repository delivery may occur off-thread; the context object also
            // causes Qt to discard the queued functor after this object is destroyed.
            QMetaObject::invokeMethod(
                this,
                [this, forecast, isCachedData]() {
                    handleForecast(forecast, isCachedData);
                },
                Qt::QueuedConnection);
        });
        repository_->setErrorHandler(
        [this](const QString &) {
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    const QString newErrorText = QCoreApplication::translate(
                        "WeatherMainViewModel",
                        "Unable to load weather data. Check your connection and try again.");
                    if (errorText_ != newErrorText) {
                        errorText_ = newErrorText;
                        emit errorTextChanged();
                    }
                },
                Qt::QueuedConnection);
        });
        geocodingService_->setResultsHandler(
        [this](const std::vector<LocationResult> &locations) {
            // Copy the result set across the thread boundary before adapting it for QML.
            QMetaObject::invokeMethod(
                this,
                [this, locations]() {
                    QVariantList results{};
                    // Reserve once because one QVariantMap is emitted for each service result.
                    results.reserve(static_cast<qsizetype>(locations.size()));
                    for (const LocationResult &location : locations) {
                        const QString displayName = location.adminArea.isEmpty()
                            ? QStringLiteral("%1, %2").arg(location.cityName, location.country)
                            : QStringLiteral("%1, %2, %3")
                                  .arg(location.cityName, location.adminArea, location.country);
                        results.push_back(QVariantMap{
                            {QStringLiteral("name"), displayName},
                            {QStringLiteral("cityName"), location.cityName},
                            {QStringLiteral("country"), location.country},
                            {QStringLiteral("countryCode"), location.countryCode},
                            {QStringLiteral("adminArea"), location.adminArea},
                            {QStringLiteral("latitude"), location.latitude},
                            {QStringLiteral("longitude"), location.longitude}});
                    }
                    emit locationSearchResults(results);
                },
                Qt::QueuedConnection);
        });
        geocodingService_->setLocationResolvedHandler(
        [this](const LocationResult &location) {
            // Reverse-geocoding completion is asynchronous and must re-enter the UI thread.
            QMetaObject::invokeMethod(
                this,
                [this, location]() {
                    // Ignore stale responses from a location that was replaced while
                    // reverse geocoding was in flight; tolerance accommodates service rounding.
                    const bool matchesSelectedLocation =
                        std::abs(location.latitude - latitude_)
                            <= localizedLocationCoordinateTolerance
                        && std::abs(location.longitude - longitude_)
                            <= localizedLocationCoordinateTolerance;
                    if (!matchesSelectedLocation) {
                        return;
                    }

                    const QString localizedCityName = location.cityName.trimmed();
                    const QString localizedCountryName = location.country.trimmed();
                    // Keep the previous usable label when the service returns incomplete data.
                    if (localizedCityName.isEmpty() || localizedCountryName.isEmpty()) {
                        return;
                    }

                    selectedLocationName_ = localizedCityName;
                    selectedCountry_ = localizedCountryName;
                    if (!location.countryCode.isEmpty()) {
                        selectedCountryCode_ = location.countryCode;
                    }
                    emit locationChanged();
                },
                Qt::QueuedConnection);
        });
        const auto subscription = reachabilityService_->addReachabilityChangedHandler(
            [this](const bool isOnline) {
                QMetaObject::invokeMethod(
                    this,
                    [this, isOnline]() { handleOnlineStatusChanged(isOnline); },
                    Qt::QueuedConnection);
            });
        if (subscription == 0U) {
            throw std::runtime_error{"Unable to subscribe to network reachability."};
        }
        reachabilitySubscription_ = subscription;
        repository_->startPolling(latitude_, longitude_, refreshIntervalMilliseconds);
    } catch (...) {
        repository_->stopPolling();
        repository_->setForecastHandler({});
        repository_->setErrorHandler({});
        geocodingService_->setResultsHandler({});
        geocodingService_->setLocationResolvedHandler({});
        if (reachabilitySubscription_.has_value()) {
            reachabilityService_->removeReachabilityChangedHandler(
                reachabilitySubscription_.value());
            reachabilitySubscription_.reset();
        }
        throw;
    }
}

/**
 * @brief Detaches asynchronous producers before owned state is destroyed.
 * @post No service retains a callback intentionally targeting this ViewModel.
 */
WeatherMainViewModel::~WeatherMainViewModel()
{
    // Clear callbacks before releasing the reachability subscription to narrow
    // the window in which an external producer could capture this during teardown.
    repository_->setForecastHandler({});
    repository_->setErrorHandler({});
    geocodingService_->setResultsHandler({});
    geocodingService_->setLocationResolvedHandler({});
    if (reachabilitySubscription_.has_value()) {
        reachabilityService_->removeReachabilityChangedHandler(
            reachabilitySubscription_.value());
    }
}

/** @brief Reads the last reachability state applied on the QObject thread. */
bool WeatherMainViewModel::isOnline() const noexcept
{
    return isOnline_;
}

/** @brief Reads whether the active forecast came from persistent cache. */
bool WeatherMainViewModel::isCachedData() const noexcept
{
    return isCachedData_;
}

/** @brief Reads the one-way readiness latch set by the first forecast. */
bool WeatherMainViewModel::weatherDataReady() const noexcept
{
    return weatherDataReady_;
}

/** @brief Returns the fixed language list whose order matches AppLanguage values. */
const QStringList &WeatherMainViewModel::supportedLanguages() const noexcept
{
    static const QStringList languages{
        QStringLiteral("English"),
        QStringLiteral("Deutsch"),
        QStringLiteral("Français"),
        QStringLiteral("中文"),
        QStringLiteral("Nederlands"),
        QStringLiteral("Norsk"),
        QStringLiteral("Svenska"),
        QStringLiteral("日本語"),
        QStringLiteral("한국어"),
        QStringLiteral("Español"),
        QStringLiteral("Italiano")};
    return languages;
}

/** @brief Reads the index used consistently by the QML selector and AppLanguage. */
int WeatherMainViewModel::currentLanguageIndex() const noexcept
{
    return currentLanguageIndex_;
}

/** @brief Reads the current temperature supplied by the latest forecast. */
float WeatherMainViewModel::currentTemperature() const noexcept
{
    return currentTemperature_;
}

/** @brief Reads the WMO code used for text and icon selection. */
int WeatherMainViewModel::wmoWeatherCode() const noexcept
{
    return static_cast<int>(wmoWeatherCode_);
}

/** @brief Exposes the stable, internally owned daily model to QML. */
DailyForecastViewModel *WeatherMainViewModel::dailyModel() noexcept
{
    return &dailyModel_;
}

/** @brief Exposes the stable, internally owned hourly model to QML. */
HourlyForecastViewModel *WeatherMainViewModel::hourlyModel() noexcept
{
    return &hourlyModel_;
}

/** @brief Reads the most recently accepted localized city name. */
const QString &WeatherMainViewModel::selectedLocationName() const noexcept
{
    return selectedLocationName_;
}

/** @brief Reads the most recently accepted localized country name. */
const QString &WeatherMainViewModel::selectedCountry() const noexcept
{
    return selectedCountry_;
}

/** @brief Reads precipitation probability using the QML-compatible int type. */
int WeatherMainViewModel::currentRainChance() const noexcept
{
    return static_cast<int>(currentRainChance_);
}

/** @brief Reads relative humidity using the QML-compatible int type. */
int WeatherMainViewModel::currentHumidity() const noexcept
{
    return static_cast<int>(currentHumidity_);
}

/** @brief Reads the latitude-derived target rotation about the globe's X axis. */
float WeatherMainViewModel::targetGlobeRotationX() const noexcept
{
    return targetGlobeRotationX_;
}

/** @brief Reads the longitude-derived target rotation about the globe's Y axis. */
float WeatherMainViewModel::targetGlobeRotationY() const noexcept
{
    return targetGlobeRotationY_;
}

/** @brief Reads the cached coordinate label rebuilt after location or locale changes. */
const QString &WeatherMainViewModel::locationText() const noexcept
{
    return locationText_;
}

/** @brief Reads the cached and localized reachability label. */
const QString &WeatherMainViewModel::onlineStatusText() const noexcept
{
    return onlineStatusText_;
}

/** @brief Reads the cached presentation string for current temperature. */
const QString &WeatherMainViewModel::temperatureText() const noexcept
{
    return temperatureText_;
}

/** @brief Reads the cached localized interpretation of the current WMO code. */
const QString &WeatherMainViewModel::conditionDescription() const noexcept
{
    return conditionDescription_;
}

/** @brief Reads the cached presentation string for current wind speed. */
const QString &WeatherMainViewModel::windText() const noexcept
{
    return windText_;
}

/** @brief Reads the validated latitude used by polling and reverse geocoding. */
double WeatherMainViewModel::latitude() const noexcept
{
    return latitude_;
}

/** @brief Reads the validated longitude used by polling and reverse geocoding. */
double WeatherMainViewModel::longitude() const noexcept
{
    return longitude_;
}

/** @brief Exposes the stable, internally owned shader parameter object to QML. */
WeatherShaderParams *WeatherMainViewModel::shaderParams() noexcept
{
    return &shaderParams_;
}

const QString &WeatherMainViewModel::errorText() const noexcept
{
    return errorText_;
}

/**
 * @brief Applies a valid language selection across localization-aware services.
 * @details The state commits only after the translation catalog loads, then
 * polling restarts so newly fetched labels use the same locale as the UI.
 */
void WeatherMainViewModel::setLanguageIndex(const int languageIndex)
{
    // Guard the enum conversion and avoid redundant translation/service work.
    if ((languageIndex < 0) || (languageIndex >= supportedLanguageCount)
        || (languageIndex == currentLanguageIndex_)) {
        return;
    }

    const AppLanguage language = appLanguageForIndex(languageIndex);
    // Retain the previous coherent locale if the requested catalog cannot load.
    if (!localizationService_.switchLanguage(language)) {
        return;
    }

    const QString isoLanguageCode = languageCode(language);
    currentLanguageIndex_ = languageIndex;
    repository_->setLanguageCode(isoLanguageCode);
    geocodingService_->setLanguageCode(isoLanguageCode);
    selectedCountry_ = localizedCountryName(
        isoLanguageCode,
        selectedCountryCode_,
        selectedCountry_);
    emit locationChanged();
    geocodingService_->resolveLocalizedLocation(latitude_, longitude_);

    emit languageChanged();
    // Restart polling to prevent subsequent model data from mixing language contexts.
    repository_->startPolling(latitude_, longitude_, refreshIntervalMilliseconds);
}

/**
 * @brief Validates and commits coordinates before restarting forecast polling.
 * @post Valid coordinates are stored and polling targets the same location.
 */
void WeatherMainViewModel::requestRefresh(const double latitude, const double longitude)
{
    // Reject non-finite and out-of-domain values at the QML trust boundary.
    if (!coordinatesAreValid(latitude, longitude)) {
        return;
    }

    const bool locationChangedValue = (latitude_ != latitude) || (longitude_ != longitude);
    latitude_ = latitude;
    longitude_ = longitude;
    locationText_ = formatLocation(latitude_, longitude_);
    // Suppress redundant notifications while still allowing an explicit refresh.
    if (locationChangedValue) {
        emit locationChanged();
    }

    repository_->startPolling(latitude, longitude, refreshIntervalMilliseconds);
}

/**
 * @brief Delegates a city query to the asynchronous geocoding service.
 * @details Results are normalized for QML by the queued handler installed at construction.
 */
void WeatherMainViewModel::searchCity(const QString &query)
{
    geocodingService_->search(query);
}

/**
 * @brief Commits a geocoded selection and maps it to globe Euler targets.
 * @post Valid selections update identity, coordinates, polling, and rotation state.
 */
void WeatherMainViewModel::selectLocation(
    const QString &cityName,
    const QString &countryName,
    const QString &countryCode,
    const double lat,
    const double lon)
{
    // Validate external geocoder/QML input before narrowing values for rendering.
    if (!coordinatesAreValid(lat, lon)) {
        return;
    }

    const QString selectedCity = cityName.trimmed();
    const QString selectedCountry = countryName.trimmed();
    const bool nameChanged = (selectedLocationName_ != selectedCity)
        || (selectedCountry_ != selectedCountry);
    const bool coordinatesChanged = (latitude_ != lat) || (longitude_ != lon);
    // Latitude maps directly to pitch. Longitude is negated because geographic
    // east-positive coordinates oppose the scene's positive Y rotation direction.
    const float newRotationX = static_cast<float>(lat);
    const float newRotationY = static_cast<float>(-lon);
    const bool rotationChanged = (targetGlobeRotationX_ != newRotationX)
        || (targetGlobeRotationY_ != newRotationY);

    selectedLocationName_ = selectedCity;
    selectedCountry_ = selectedCountry;
    selectedCountryCode_ = countryCode.trimmed().toUpper();
    targetGlobeRotationX_ = newRotationX;
    targetGlobeRotationY_ = newRotationY;
    requestRefresh(lat, lon);
    // requestRefresh already notifies coordinate changes; emit here only for a name-only update.
    if (nameChanged && !coordinatesChanged) {
        emit locationChanged();
    }
    if (rotationChanged) {
        emit targetRotationChanged();
    }
}

/**
 * @brief Rebuilds every locale-dependent cached label after translator replacement.
 * @post Bound QML text receives notifications even when underlying numeric data is unchanged.
 */
void WeatherMainViewModel::handleLanguageChanged()
{
    locationText_ = formatLocation(latitude_, longitude_);
    onlineStatusText_ = isOnline_
        ? QCoreApplication::translate("WeatherMainViewModel", "Online")
        : QCoreApplication::translate("WeatherMainViewModel", "Offline");
    temperatureText_ = WeatherPresentationFormatter::temperatureText(
        currentTemperature_,
        1);
    conditionDescription_ = WeatherPresentationFormatter::conditionDescription(wmoWeatherCode_);
    windText_ = WeatherPresentationFormatter::windSpeedText(currentWindSpeedKph_);
    if (!errorText_.isEmpty()) {
        errorText_ = QCoreApplication::translate(
            "WeatherMainViewModel",
            "Unable to load weather data. Check your connection and try again.");
        emit errorTextChanged();
    }
    dailyModel_.retranslate();
    hourlyModel_.retranslate();

    // Locale changes affect presentation strings, so notify each relevant binding explicitly.
    emit onlineStatusChanged();
    emit weatherDataChanged();
    emit locationChanged();
}

/**
 * @brief Applies a queued reachability transition and refreshes its localized label.
 * @post Observers are notified only when the binary state actually changes.
 */
void WeatherMainViewModel::handleOnlineStatusChanged(const bool isOnline)
{
    // Coalesce duplicate service notifications to avoid unnecessary QML reevaluation.
    if (isOnline_ == isOnline) {
        return;
    }

    isOnline_ = isOnline;
    onlineStatusText_ = isOnline_
        ? QCoreApplication::translate("WeatherMainViewModel", "Online")
        : QCoreApplication::translate("WeatherMainViewModel", "Offline");
    emit onlineStatusChanged();
}

/**
 * @brief Atomically projects a repository forecast into all presentation models.
 * @details Change flags are captured before mutation so each Qt notification is
 * emitted once and only after related scalar and list-model state is coherent.
 */
void WeatherMainViewModel::handleForecast(
    const WeatherForecastData &forecast,
    const bool isCachedData)
{
    const WeatherTelemetry &telemetry = forecast.current;
    const bool weatherChanged = (currentTemperature_ != telemetry.temperatureC)
        || (currentWindSpeedKph_ != telemetry.windSpeedKph)
        || (wmoWeatherCode_ != telemetry.weatherCode)
        || (currentRainChance_ != telemetry.precipitationProbability)
        || (currentHumidity_ != telemetry.relativeHumidity);
    const bool cacheStatusChangedValue = isCachedData_ != isCachedData;
    const bool becameReady = !weatherDataReady_;

    // Commit the complete snapshot before signals allow QML to read dependent properties.
    currentTemperature_ = telemetry.temperatureC;
    currentWindSpeedKph_ = telemetry.windSpeedKph;
    wmoWeatherCode_ = telemetry.weatherCode;
    currentRainChance_ = telemetry.precipitationProbability;
    currentHumidity_ = telemetry.relativeHumidity;
    temperatureText_ = WeatherPresentationFormatter::temperatureText(
        currentTemperature_,
        1);
    conditionDescription_ = WeatherPresentationFormatter::conditionDescription(wmoWeatherCode_);
    windText_ = WeatherPresentationFormatter::windSpeedText(currentWindSpeedKph_);
    isCachedData_ = isCachedData;
    weatherDataReady_ = true;
    if (!errorText_.isEmpty()) {
        errorText_.clear();
        emit errorTextChanged();
    }
    hourlyModel_.updateForecastData(forecast.hourly);
    dailyModel_.updateForecastData(forecast.daily);

    // Emit the minimal notification set to limit binding and scene recomputation.
    if (weatherChanged) {
        emit weatherDataChanged();
    }
    if (cacheStatusChangedValue) {
        emit cacheStatusChanged();
    }
    if (becameReady) {
        emit weatherDataReadyChanged();
    }
}
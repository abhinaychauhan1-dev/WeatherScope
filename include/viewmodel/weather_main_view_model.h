/**
 * @file     : weather_main_view_model.h
 * @brief    : Declares the primary QML-facing weather ViewModel.
 * @details  : This ViewModel owns and presents the current weather, location,
 * language, connectivity, forecast-model, and shader state used by the QML
 * presentation layer. It coordinates injected Model and Service interfaces,
 * converts their asynchronous results into Qt properties and signals, and
 * keeps QML independent of network and persistence implementations.
 *
 * Architecture role: ViewModel in the Model-View-ViewModel (MVVM) layer.
 * Applied patterns: Dependency Injection, Observer, Adapter, and Model-View-
 * ViewModel.
 *
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @date     : 2026-09-06
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/**
 * @brief Prevents duplicate declarations when this header is included more than once.
 * @details The positive definition is paired with the closing annotated #endif.
 */
#ifndef WEATHER_MAIN_VIEW_MODEL_H
#define WEATHER_MAIN_VIEW_MODEL_H

#include "model/i_geocoding_service.h"
#include "model/i_localization_service.h"
#include "model/i_network_reachability_service.h"
#include "model/i_weather_repository.h"
#include "viewmodel/daily_forecast_view_model.h"
#include "viewmodel/hourly_forecast_view_model.h"
#include "viewmodel/weather_shader_params.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

#include <cstdint>
#include <memory>
#include <optional>

/**
 * @brief Exposes weather application state and commands to the QML view.
 * @details The class adapts repository, reachability, geocoding, localization,
 * and forecast data to QML-friendly properties. Service callbacks are queued
 * onto this object's Qt thread before they mutate observable state.
 *
 * @par Thread safety
 * Instances are not thread-safe. Construct the object and call its public API
 * only from its QObject-affinity thread. Asynchronous service callbacks are
 * marshalled to that thread with queued invocations.
 *
 * @par Lifecycle and ownership
 * The optional QObject parent owns the instance under Qt parent-child rules.
 * Shared pointers retain the injected service objects for this instance's
 * lifetime. The LocalizationManager is borrowed and must outlive this object.
 * Child ViewModels and shader parameters are owned directly by this object;
 * pointers returned for them are non-owning and remain valid until destruction.
 *
 * @par Copy and move semantics
 * Copy construction, copy assignment, move construction, and move assignment
 * are unavailable through the QObject base class. Instances therefore retain
 * stable identity, thread affinity, signal connections, and callback targets
 * throughout their lifetime.
 */
class WeatherMainViewModel final : public QObject
{
    /** @brief Enables Qt meta-object features required by properties, signals, and slots. */
    Q_OBJECT
    /** @brief Registers WeatherMainViewModel as a named type in the QML module. */
    QML_ELEMENT
    /** @brief Prevents direct QML construction because model services must be injected. */
    QML_UNCREATABLE("WeatherMainViewModel requires injected model services")

    /** @brief Indicates whether the network is currently reachable. @note Read-only; changes emit onlineStatusChanged(). */
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY onlineStatusChanged)
    /** @brief Indicates whether the displayed forecast originated from the cache. @note Read-only; changes emit cacheStatusChanged(). */
    Q_PROPERTY(bool isCachedData READ isCachedData NOTIFY cacheStatusChanged)
    /** @brief Indicates whether the first usable forecast has been received. @note Read-only; changes emit weatherDataReadyChanged(). */
    Q_PROPERTY(bool weatherDataReady READ weatherDataReady NOTIFY weatherDataReadyChanged)
    /** @brief Lists the localized language names offered by the application. @note Constant for the lifetime of the object. */
    Q_PROPERTY(QStringList supportedLanguages READ supportedLanguages NOTIFY supportedLanguagesChanged)
    /** @brief Holds the zero-based index of the active application language. @note Writable through setLanguageIndex(); changes emit languageChanged(). */
    Q_PROPERTY(int currentLanguageIndex READ currentLanguageIndex WRITE setLanguageIndex NOTIFY languageChanged)
    /** @brief Holds the current air temperature in degrees Celsius. @note Read-only; changes emit weatherDataChanged(). */
    Q_PROPERTY(float currentTemperature READ currentTemperature NOTIFY weatherDataChanged)
    /** @brief Holds the current World Meteorological Organization weather code. @note Read-only; changes emit weatherDataChanged(). */
    Q_PROPERTY(int wmoWeatherCode READ wmoWeatherCode NOTIFY weatherDataChanged)
    /** @brief Provides the non-owning daily forecast list model. @note Constant pointer owned by this ViewModel. */
    Q_PROPERTY(DailyForecastViewModel* dailyModel READ dailyModel NOTIFY dailyModelChanged)
    /** @brief Provides the non-owning hourly forecast list model. @note Constant pointer owned by this ViewModel. */
    Q_PROPERTY(HourlyForecastViewModel* hourlyModel READ hourlyModel NOTIFY hourlyModelChanged)
    /** @brief Holds the display name of the selected city. @note Read-only; changes emit locationChanged(). */
    Q_PROPERTY(QString selectedLocationName READ selectedLocationName NOTIFY locationChanged)
    /** @brief Holds the display name of the selected country. @note Read-only; changes emit locationChanged(). */
    Q_PROPERTY(QString selectedCountry READ selectedCountry NOTIFY locationChanged)
    /** @brief Holds the current precipitation probability as a percentage. @note Read-only; changes emit weatherDataChanged(). */
    Q_PROPERTY(int currentRainChance READ currentRainChance NOTIFY weatherDataChanged)
    /** @brief Holds the current relative humidity as a percentage. @note Read-only; changes emit weatherDataChanged(). */
    Q_PROPERTY(int currentHumidity READ currentHumidity NOTIFY weatherDataChanged)
    /** @brief Holds the target globe rotation about the horizontal axis in degrees. @note Read-only; changes emit targetRotationChanged(). */
    Q_PROPERTY(float targetGlobeRotationX READ targetGlobeRotationX NOTIFY targetRotationChanged)
    /** @brief Holds the target globe rotation about the vertical axis in degrees. @note Read-only; changes emit targetRotationChanged(). */
    Q_PROPERTY(float targetGlobeRotationY READ targetGlobeRotationY NOTIFY targetRotationChanged)
    /** @brief Holds the localized, formatted selected coordinates. @note Read-only; changes emit locationChanged(). */
    Q_PROPERTY(QString locationText READ locationText NOTIFY locationChanged)
    /** @brief Holds the localized network-reachability status text. @note Read-only; changes emit onlineStatusChanged(). */
    Q_PROPERTY(QString onlineStatusText READ onlineStatusText NOTIFY onlineStatusChanged)
    /** @brief Holds the localized, formatted current temperature. @note Read-only; changes emit weatherDataChanged(). */
    Q_PROPERTY(QString temperatureText READ temperatureText NOTIFY weatherDataChanged)
    /** @brief Holds the localized description of the current weather code. @note Read-only; changes emit weatherDataChanged(). */
    Q_PROPERTY(QString conditionDescription READ conditionDescription NOTIFY weatherDataChanged)
    /** @brief Holds the localized, formatted current wind speed. @note Read-only; changes emit weatherDataChanged(). */
    Q_PROPERTY(QString windText READ windText NOTIFY weatherDataChanged)
    /** @brief Holds the selected latitude in decimal degrees. @note Read-only; changes emit locationChanged(). */
    Q_PROPERTY(double latitude READ latitude NOTIFY locationChanged)
    /** @brief Holds the selected longitude in decimal degrees. @note Read-only; changes emit locationChanged(). */
    Q_PROPERTY(double longitude READ longitude NOTIFY locationChanged)
    /** @brief Provides the non-owning weather shader parameter object. @note Constant pointer owned by this ViewModel. */
    Q_PROPERTY(WeatherShaderParams* shaderParams READ shaderParams NOTIFY shaderParamsChanged)
    /** @brief Holds a localized recoverable error message, or an empty string. */
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    /**
     * @brief Constructs the primary weather ViewModel with its dependencies.
     * @param[in] repository Shared ownership of the weather repository.
     * @param[in] reachabilityService Shared ownership of the reachability service.
     * @param[in] geocodingService Shared ownership of the geocoding service.
     * @param[in] localizationManager Borrowed localization manager reference.
     * @param[in] parent Optional QObject owner; may be `nullptr`.
     * @note Must be called on the intended QObject-affinity thread. Throws
     * std::invalid_argument if any shared service dependency is null.
     */
    WeatherMainViewModel(
        std::shared_ptr<IWeatherRepository> repository,
        std::shared_ptr<INetworkReachabilityService> reachabilityService,
        std::shared_ptr<IGeocodingService> geocodingService,
        ILocalizationService &localizationService,
        QObject *parent = nullptr);

    /**
     * @brief Disconnects callbacks and destroys the ViewModel.
     * @note Call on the object's affinity thread. The destructor does not
     * propagate exceptions under the implicit noexcept destructor contract.
     */
    ~WeatherMainViewModel() override;

    /** @brief Reports network reachability. @return `true` when online. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] bool isOnline() const noexcept;
    /** @brief Reports whether current data is cached. @return `true` for cached data. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] bool isCachedData() const noexcept;
    /** @brief Reports whether forecast data is available. @return `true` after the first forecast. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] bool weatherDataReady() const noexcept;
    /** @brief Returns supported language names. @return Immutable application-lifetime list reference. @note Noexcept; the returned reference is non-owning. */
    [[nodiscard]] const QStringList &supportedLanguages() const noexcept;
    /** @brief Returns the active language index. @return Zero-based language index. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] int currentLanguageIndex() const noexcept;
    /** @brief Returns the current temperature. @return Temperature in degrees Celsius. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] float currentTemperature() const noexcept;
    /** @brief Returns the current WMO weather code. @return WMO weather interpretation code. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] int wmoWeatherCode() const noexcept;
    /** @brief Returns the daily forecast model. @return Non-null, non-owning pointer valid for this object's lifetime. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] DailyForecastViewModel *dailyModel() noexcept;
    /** @brief Returns the hourly forecast model. @return Non-null, non-owning pointer valid for this object's lifetime. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] HourlyForecastViewModel *hourlyModel() noexcept;
    /** @brief Returns the selected city name. @return Immutable borrowed string reference. @note Noexcept; invalidated when the value changes or this object is destroyed. */
    [[nodiscard]] const QString &selectedLocationName() const noexcept;
    /** @brief Returns the selected country name. @return Immutable borrowed string reference. @note Noexcept; invalidated when the value changes or this object is destroyed. */
    [[nodiscard]] const QString &selectedCountry() const noexcept;
    /** @brief Returns current rain probability. @return Percentage in the range supplied by the repository. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] int currentRainChance() const noexcept;
    /** @brief Returns current relative humidity. @return Percentage in the range supplied by the repository. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] int currentHumidity() const noexcept;
    /** @brief Returns target horizontal globe rotation. @return Rotation in degrees. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] float targetGlobeRotationX() const noexcept;
    /** @brief Returns target vertical globe rotation. @return Rotation in degrees. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] float targetGlobeRotationY() const noexcept;
    /** @brief Returns formatted selected coordinates. @return Immutable borrowed string reference. @note Noexcept; invalidated when the value changes or this object is destroyed. */
    [[nodiscard]] const QString &locationText() const noexcept;
    /** @brief Returns localized connectivity text. @return Immutable borrowed string reference. @note Noexcept; invalidated when the value changes or this object is destroyed. */
    [[nodiscard]] const QString &onlineStatusText() const noexcept;
    /** @brief Returns formatted temperature text. @return Immutable borrowed string reference. @note Noexcept; invalidated when the value changes or this object is destroyed. */
    [[nodiscard]] const QString &temperatureText() const noexcept;
    /** @brief Returns the localized weather description. @return Immutable borrowed string reference. @note Noexcept; invalidated when the value changes or this object is destroyed. */
    [[nodiscard]] const QString &conditionDescription() const noexcept;
    /** @brief Returns formatted wind-speed text. @return Immutable borrowed string reference. @note Noexcept; invalidated when the value changes or this object is destroyed. */
    [[nodiscard]] const QString &windText() const noexcept;
    /** @brief Returns the selected latitude. @return Latitude in decimal degrees. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] double latitude() const noexcept;
    /** @brief Returns the selected longitude. @return Longitude in decimal degrees. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] double longitude() const noexcept;
    /** @brief Returns weather shader parameters. @return Non-null, non-owning pointer valid for this object's lifetime. @note Noexcept; call on the object's affinity thread. */
    [[nodiscard]] WeatherShaderParams *shaderParams() noexcept;
    /** @brief Returns the current localized recoverable error text. */
    [[nodiscard]] const QString &errorText() const noexcept;

    /**
     * @brief Activates a supported application language.
     * @param[in] languageIndex Zero-based index into supportedLanguages().
     * @return Nothing.
     * @note Invoke on the object's affinity thread. Invalid, unchanged, or
     * unavailable language selections are ignored. Exceptions may propagate.
     */
    Q_INVOKABLE void setLanguageIndex(int languageIndex);

    /**
     * @brief Selects coordinates and requests periodic forecast refreshes.
     * @param[in] latitude Latitude in decimal degrees, in [-90, 90].
     * @param[in] longitude Longitude in decimal degrees, in [-180, 180].
     * @return Nothing.
     * @note Invoke on the object's affinity thread. Non-finite or out-of-range
     * coordinates are ignored. Exceptions may propagate.
     */
    Q_INVOKABLE void requestRefresh(double latitude, double longitude);
    /**
     * @brief Requests geocoding suggestions for a city query.
     * @param[in] query User-entered city search text.
     * @return Nothing.
     * @note Invoke on the object's affinity thread. Results arrive
     * asynchronously through locationSearchResults(). Exceptions may propagate.
     */
    Q_INVOKABLE void searchCity(const QString &query);
    /**
     * @brief Selects a geocoded location and refreshes its weather.
     * @param[in] cityName Display name of the city.
     * @param[in] countryName Display name of the country.
     * @param[in] countryCode ISO 3166-1 alpha-2 country code when available.
     * @param[in] lat Latitude in decimal degrees, in [-90, 90].
     * @param[in] lon Longitude in decimal degrees, in [-180, 180].
     * @return Nothing.
     * @note Invoke on the object's affinity thread. Invalid coordinates are
     * ignored. Exceptions may propagate.
     */
    Q_INVOKABLE void selectLocation(
        const QString &cityName,
        const QString &countryName,
        const QString &countryCode,
        double lat,
        double lon);

signals:
    /** @brief Retained for the immutable language-list property contract. */
    void supportedLanguagesChanged();
    /** @brief Retained for the stable daily-model pointer property contract. */
    void dailyModelChanged();
    /** @brief Retained for the stable hourly-model pointer property contract. */
    void hourlyModelChanged();
    /** @brief Retained for the stable shader-parameter pointer property contract. */
    void shaderParamsChanged();
    /** @brief Signals a change in recoverable user-facing error state. */
    void errorTextChanged();
    /** @brief Signals a change to network status and its display text. @return Nothing. @note Emitted on the object's affinity thread. */
    void onlineStatusChanged();
    /** @brief Signals a change to the forecast cache-source status. @return Nothing. @note Emitted on the object's affinity thread. */
    void cacheStatusChanged();
    /** @brief Signals that forecast readiness has changed. @return Nothing. @note Emitted on the object's affinity thread. */
    void weatherDataReadyChanged();
    /** @brief Signals a change to current weather values or formatted text. @return Nothing. @note Emitted on the object's affinity thread. */
    void weatherDataChanged();
    /** @brief Signals a change to selected location data or text. @return Nothing. @note Emitted on the object's affinity thread. */
    void locationChanged();
    /** @brief Signals a change to the target globe rotation. @return Nothing. @note Emitted on the object's affinity thread. */
    void targetRotationChanged();
    /** @brief Signals a successful application-language change. @return Nothing. @note Emitted on the object's affinity thread. */
    void languageChanged();
    /**
     * @brief Delivers QML-compatible geocoding search results.
     * @param[out] results Location maps containing names, country data, and coordinates.
     * @return Nothing.
     * @note Emitted asynchronously on the object's affinity thread.
     */
    void locationSearchResults(const QVariantList &results);

private slots:
    /**
     * @brief Rebuilds localized presentation strings after translation changes.
     * @return Nothing.
     * @note Executes on the object's affinity thread. Exceptions may propagate.
     */
    void handleLanguageChanged();

private:
    /**
     * @brief Applies an asynchronous network-reachability update.
     * @param[in] isOnline New reachability state.
     * @return Nothing.
     * @note Called on the object's affinity thread through a queued invocation.
     * Exceptions may propagate.
     */
    void handleOnlineStatusChanged(bool isOnline);
    /**
     * @brief Applies forecast data to current, hourly, and daily presentation state.
     * @param[in] forecast Complete forecast value received from the repository.
     * @param[in] isCachedData `true` when the repository supplied cached data.
     * @return Nothing.
     * @note Called on the object's affinity thread through a queued invocation.
     * Exceptions may propagate.
     */
    void handleForecast(const WeatherForecastData &forecast, bool isCachedData);

    std::shared_ptr<IWeatherRepository> repository_{};
    std::shared_ptr<INetworkReachabilityService> reachabilityService_{};
    std::shared_ptr<IGeocodingService> geocodingService_{};
    ILocalizationService &localizationService_;
    std::optional<INetworkReachabilityService::SubscriptionId> reachabilitySubscription_{};
    DailyForecastViewModel dailyModel_{};
    HourlyForecastViewModel hourlyModel_{};
    WeatherShaderParams shaderParams_{};
    bool isOnline_{false};
    bool isCachedData_{false};
    bool weatherDataReady_{false};
    int currentLanguageIndex_{0};
    float currentTemperature_{0.0F};
    float currentWindSpeedKph_{0.0F};
    std::int32_t wmoWeatherCode_{0};
    std::int32_t currentRainChance_{0};
    std::int32_t currentHumidity_{0};
    float targetGlobeRotationX_{28.6139F};
    float targetGlobeRotationY_{-77.209F};
    double latitude_{28.6139};
    double longitude_{77.2090};
    QString selectedLocationName_{QStringLiteral("New Delhi")};
    QString selectedCountry_{QStringLiteral("India")};
    QString selectedCountryCode_{QStringLiteral("IN")};
    QString locationText_{QStringLiteral("28.6139 N, 77.2090 E")};
    QString onlineStatusText_{QStringLiteral("Offline")};
    QString temperatureText_{QStringLiteral("0.0 C")};
    QString conditionDescription_{QStringLiteral("Unknown")};
    QString windText_{QStringLiteral("0.0 km/h")};
    QString errorText_{};
};

#endif // WEATHER_MAIN_VIEW_MODEL_H
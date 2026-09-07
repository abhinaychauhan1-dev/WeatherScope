/**
 * @file     : i_geocoding_service.h
 * @brief    : Declares the asynchronous location search abstraction.
 * @details  : Defines the Service-layer contract and transport-neutral value
 * type used to search for cities and resolve localized location labels.
 * Architecture role: Service interface in the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include <QString>

#include <functional>
#include <vector>

/**
 * @brief Represents one geocoded location returned by a search or lookup.
 * @details This independently owned value type can be copied or moved between
 * asynchronous producers and consumers without shared mutable state.
 * @note Individual instances are not synchronized; concurrent mutation of the
 * same instance requires external synchronization.
 */
struct LocationResult
{
    /** @brief Localized city or municipality name; empty when unavailable. */
    QString cityName{};
    /** @brief Localized country name; empty when unavailable. */
    QString country{};
    /** @brief ISO 3166-1 alpha-2 country code; empty when unavailable. */
    QString countryCode{};
    /** @brief Localized state, province, or administrative area. */
    QString adminArea{};
    /** @brief ISO language code used to resolve this search or lookup result. */
    QString languageCode{};
    /** @brief Latitude in decimal degrees, normally in the range [-90, 90]. */
    double latitude{0.0};
    /** @brief Longitude in decimal degrees, normally in the range [-180, 180]. */
    double longitude{0.0};
};

/**
 * @brief Defines asynchronous geocoding operations for ViewModel consumers.
 * @details Implementations accept searches and publish results through
 * replaceable callbacks, keeping callers independent of the network provider.
 * @note Thread safety, callback execution context, and request cancellation are
 * implementation-defined. Concrete implementations own their resources.
 * Copy and move semantics are intentionally not constrained by this interface.
 */
class IGeocodingService
{
public:
    /** @brief Callback receiving an immutable batch of search results. */
    using ResultsHandler = std::function<void(const std::vector<LocationResult> &)>;
    /** @brief Callback receiving one localized reverse-geocoding result. */
    using LocationResolvedHandler = std::function<void(const LocationResult &)>;

    /** @brief Enables polymorphic destruction. @note Implicitly noexcept. */
    virtual ~IGeocodingService() = default;

    /**
     * @brief Starts or updates an asynchronous location search.
     * @param[in] query User-entered location text.
     * @return Nothing.
     * @note Validation, debouncing, callback thread, and exceptions are implementation-defined.
     */
    virtual void search(const QString &query) = 0;
    /**
     * @brief Requests a localized label for geographic coordinates.
     * @param[in] latitude Latitude in decimal degrees.
     * @param[in] longitude Longitude in decimal degrees.
     * @return Nothing.
     * @note Validation, callback thread, and exceptions are implementation-defined.
     */
    virtual void resolveLocalizedLocation(double latitude, double longitude) = 0;
    /**
     * @brief Selects the language used by subsequent service results.
     * @param[in] languageCode ISO 639 language code.
     * @return Nothing.
     * @note Unsupported-code behavior and exceptions are implementation-defined.
     */
    virtual void setLanguageCode(const QString &languageCode) = 0;
    /**
     * @brief Replaces the callback used to publish search results.
     * @param[in] handler Callable to retain, or an empty callable to detach.
     * @return Nothing.
     * @note Callback ownership and synchronization are implementation-defined.
     */
    virtual void setResultsHandler(ResultsHandler handler) noexcept = 0;
    /**
     * @brief Replaces the callback used to publish localized locations.
     * @param[in] handler Callable to retain, or an empty callable to detach.
     * @return Nothing.
     * @note Callback ownership and synchronization are implementation-defined.
     */
    virtual void setLocationResolvedHandler(LocationResolvedHandler handler) noexcept = 0;
};
/**
 * @file     : sqlite_weather_store.h
 * @brief    : Declares SQLite-backed weather cache persistence.
 * @details  : Stores and retrieves the latest serialized weather payload using
 * isolated SQLite connections. Architecture role: persistence Service in the
 * MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include <QByteArray>
#include <QMutex>
#include <QString>

#include <optional>

/**
 * @brief Carries either the latest cached payload or a persistence error.
 * @details An empty error indicates a successful operation; a successful empty
 * payload means no cached row exists. Const members make the result immutable
 * after construction and disable assignment while preserving value construction.
 * @note Separate instances are safe to pass between threads.
 */
struct CachedWeatherPayloadResult final
{
    /** @brief Optional serialized payload; disengaged when no cache entry exists. */
    const std::optional<QByteArray> payload{};
    /** @brief Human-readable failure description; empty on success. */
    const QString error{};
};

/**
 * @brief Provides synchronized persistence for the latest weather payload.
 * @details Each operation uses a uniquely named temporary Qt SQL connection,
 * with access serialized by an internal mutex to avoid connection-cache races.
 * @note Thread-safe. The immutable database path is owned by value. Copy and
 * move operations are unavailable because QMutex is non-copyable and non-movable.
 */
class SqliteWeatherStore final
{
public:
    /** @brief Constructs a store for one SQLite database file. @param[in] databasePath Owned path to the cache database. @note Exceptions may propagate from string movement. */
    explicit SqliteWeatherStore(QString databasePath);

    /** @brief Persists the latest serialized weather payload as the single cache row. @param[in] payload Bytes to store. @return Empty string on success or an error description. @note Thread-safe; database failures are returned rather than thrown. */
    [[nodiscard]] QString save(const QByteArray &payload) const;
    /** @brief Loads the newest cached weather payload. @return Immutable payload/error result. @note Thread-safe; database failures are returned rather than thrown. */
    [[nodiscard]] CachedWeatherPayloadResult loadLatest() const;

private:
    const QString databasePath_;
    mutable QMutex mutex_;
};
/**
 * @file     : sqlite_weather_store.cpp
 * @brief    : Implements SQLite persistence for cached weather data.
 * @details  : Provides the definitions declared in sqlite_weather_store.h,
 * using serialized access and uniquely named, operation-scoped Qt SQL
 * connections so cache work can be dispatched from worker threads safely.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "model/sqlite_weather_store.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

#include <utility>

namespace
{
/** @brief Creates a collision-resistant Qt SQL connection identifier. @return Unique process-local connection name. @pre QUuid support is available. @post No database connection is created. */
[[nodiscard]] QString createConnectionName()
{
    return QStringLiteral("weather-cache-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

/**
 * @brief Ensures the single-row cache schema exists on an open connection.
 * @param[in,out] database Open SQLite connection used to execute schema DDL.
 * @return Empty string on success or the SQLite error description.
 * @pre database is open on the calling thread.
 * @post The cache table exists on success; its id constraint permits only row 1.
 */
[[nodiscard]] QString createTable(QSqlDatabase &database)
{
    // The fixed id enforces replacement of one latest snapshot instead of unbounded history.
    QSqlQuery query{database};
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS weather_cache ("
            "id INTEGER PRIMARY KEY CHECK (id = 1), "
            "payload BLOB NOT NULL, "
            "cached_at_utc TEXT NOT NULL)"))) {
        return query.lastError().text();
    }
    return {};
}
} // namespace

/** @brief Stores the immutable database path for later operation-scoped connections. @param[in] databasePath SQLite file path. @pre The path denotes an application-writable location. @post No connection or table is opened eagerly. */
SqliteWeatherStore::SqliteWeatherStore(QString databasePath)
    : databasePath_(std::move(databasePath))
{
}

/**
 * @brief Writes the payload as the sole latest-cache row.
 * @param[in] payload Serialized weather response.
 * @return Empty string on success or a database error description.
 * @pre The SQLite driver is available and databasePath_ is writable.
 * @post On success row 1 contains payload and a UTC timestamp; all connection
 * handles are destroyed and unregistered before return.
 */
QString SqliteWeatherStore::save(const QByteArray &payload) const
{
    // Serialize all operations because Qt SQL connections are thread-affine and
    // the store may be called concurrently by repository worker tasks.
    const QMutexLocker locker{&mutex_};
    const QString connectionName = createConnectionName();
    QString error{};

    {
        // A unique name prevents Qt's global connection registry from sharing a
        // connection created on another worker thread.
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(databasePath_);
        if (!database.open()) {
            error = database.lastError().text();
        } else {
            error = createTable(database);
            if (error.isEmpty()) {
                // SQLite auto-commits this single replacement statement; the fixed
                // primary key makes the cache update a bounded one-row operation.
                QSqlQuery query{database};
                query.prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO weather_cache (id, payload, cached_at_utc) "
                    "VALUES (1, :payload, :cachedAtUtc)"));
                query.bindValue(QStringLiteral(":payload"), payload);
                query.bindValue(
                    QStringLiteral(":cachedAtUtc"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
                if (!query.exec()) {
                    error = query.lastError().text();
                }
            }
        }
        database.close();
    }

    // removeDatabase must run only after every handle/query in the inner scope is destroyed.
    QSqlDatabase::removeDatabase(connectionName);
    return error;
}

/**
 * @brief Reads the newest serialized weather payload from the cache.
 * @return Payload/error result; an empty optional with no error means no row exists.
 * @pre The SQLite driver is available and databasePath_ is readable.
 * @post All temporary connection handles are destroyed and unregistered.
 */
CachedWeatherPayloadResult SqliteWeatherStore::loadLatest() const
{
    const QMutexLocker locker{&mutex_};
    const QString connectionName = createConnectionName();
    std::optional<QByteArray> payload{};
    QString error{};

    {
        // Keep the thread-local connection handle inside a scope that ends before removal.
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(databasePath_);
        if (!database.open()) {
            error = database.lastError().text();
        } else {
            error = createTable(database);
            if (error.isEmpty()) {
                QSqlQuery query{database};
                if (!query.exec(QStringLiteral(
                        "SELECT payload FROM weather_cache ORDER BY cached_at_utc DESC LIMIT 1"))) {
                    error = query.lastError().text();
                } else if (query.next()) {
                    // Absence of a row is a valid empty-cache state, not a SQL failure.
                    payload = query.value(QStringLiteral("payload")).toByteArray();
                }
            }
        }
        database.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return {std::move(payload), std::move(error)};
}
/**
 * @file     : network_reachability_service.h
 * @brief    : Declares the Qt network reachability service adapter.
 * @details  : Adapts QNetworkInformation into a synchronized observer service.
 * Architecture role: Service adapter in the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include "model/i_network_reachability_service.h"

#include <QMutex>
#include <QNetworkInformation>
#include <QObject>
#include <QPointer>

#include <cstdint>
#include <map>

/**
 * @brief Provides synchronized network reachability state and subscriptions.
 * @details State and handler registration are protected by a mutex; callbacks
 * are copied and invoked after unlocking to prevent re-entrant deadlocks.
 * @note Public query/subscription operations are thread-safe. Callback execution
 * follows the Qt reachability notification context. QObject disables copy and
 * move; QNetworkInformation is observed but owned by QCoreApplication.
 */
class NetworkReachabilityService final : public QObject, public INetworkReachabilityService
{
    /** @brief Enables Qt signal connections used for platform reachability updates. */
    Q_OBJECT

public:
    /** @brief Constructs the adapter and observes the application network-information instance. @param[in] parent Optional QObject owner. @note Construct on the intended QObject thread; exceptions may propagate. */
    explicit NetworkReachabilityService(QObject *parent = nullptr);
    /** @brief Destroys the adapter and registered handlers. @note Implicitly noexcept. */
    ~NetworkReachabilityService() override = default;

    /** @brief Returns the synchronized reachability snapshot. @return true when reachable. @note Thread-safe and non-throwing. */
    [[nodiscard]] bool isReachable() const noexcept override;
    /** @brief Registers a reachability observer. @param[in] handler Callable invoked after state changes. @return Non-zero subscription identifier, or zero when no identifier is available. @note Thread-safe; exceptions may propagate during storage allocation. */
    [[nodiscard]] SubscriptionId addReachabilityChangedHandler(
        ReachabilityChangedHandler handler) override;
    /** @brief Removes a reachability observer. @param[in] subscriptionId Identifier returned during registration. @return Nothing. @note Thread-safe and non-throwing; unknown identifiers are ignored. */
    void removeReachabilityChangedHandler(SubscriptionId subscriptionId) noexcept override;

private:
    /** @brief Converts a Qt reachability event into state and observer updates. @param[in] reachability Platform reachability value. @return Nothing. @note Runs in the QObject signal-delivery context and invokes handlers without holding the mutex. */
    void handleReachabilityChanged(QNetworkInformation::Reachability reachability);

    // QNetworkInformation is owned by QCoreApplication; QPointer observes its lifetime.
    QPointer<QNetworkInformation> networkInformation_{};
    mutable QMutex mutex_{};
    std::map<SubscriptionId, ReachabilityChangedHandler> handlers_{};
    SubscriptionId nextSubscriptionId_{1U};
    bool isReachable_{false};
};
/**
 * @file     : network_reachability_service.cpp
 * @brief    : Implements monitoring of network connectivity changes.
 * @details  : Provides the definitions declared in
 * network_reachability_service.h and adapts Qt platform reachability events to
 * a mutex-protected callback subscription API.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

#include "model/network_reachability_service.h"

#include <QMutexLocker>

#include <limits>
#include <utility>
#include <vector>

/**
 * @brief Loads and observes the platform reachability backend when available.
 * @param[in] parent Optional QObject lifecycle owner.
 * @pre Construction occurs on the intended Qt event-loop thread.
 * @post State reflects the current backend, or remains offline if no backend loads.
 */
NetworkReachabilityService::NetworkReachabilityService(QObject *parent)
    : QObject(parent)
{
    // Loading is process-global and only needed when Qt has not initialized a backend.
    if (QNetworkInformation::instance() == nullptr) {
        static_cast<void>(QNetworkInformation::loadDefaultBackend());
    }

    networkInformation_ = QNetworkInformation::instance();
    // Absence of a supported platform backend degrades conservatively to offline.
    if (networkInformation_ != nullptr) {
        isReachable_ =
            networkInformation_->reachability() == QNetworkInformation::Reachability::Online;
        connect(
            networkInformation_,
            &QNetworkInformation::reachabilityChanged,
            this,
            &NetworkReachabilityService::handleReachabilityChanged);
    }
}

/** @brief Reads the synchronized reachability snapshot. @return true when online. @pre None. @post State is unchanged. @note Thread-safe and non-throwing. */
bool NetworkReachabilityService::isReachable() const noexcept
{
    const QMutexLocker locker{&mutex_};
    return isReachable_;
}

INetworkReachabilityService::SubscriptionId
NetworkReachabilityService::addReachabilityChangedHandler(ReachabilityChangedHandler handler)
{
    // Zero is the invalid-subscription sentinel; empty callbacks are never registered.
    if (!handler) {
        return 0U;
    }

    const QMutexLocker locker{&mutex_};
    // Refuse wraparound so a live subscription identifier is never reused.
    if (nextSubscriptionId_ == std::numeric_limits<SubscriptionId>::max()) {
        return 0U;
    }

    const SubscriptionId subscriptionId = nextSubscriptionId_;
    ++nextSubscriptionId_;
    handlers_.emplace(subscriptionId, std::move(handler));
    return subscriptionId;
}

/** @brief Removes a registered observer. @param[in] subscriptionId Opaque identifier. @pre None. @post The identifier no longer receives updates. @note Thread-safe and non-throwing. */
void NetworkReachabilityService::removeReachabilityChangedHandler(
    const SubscriptionId subscriptionId) noexcept
{
    const QMutexLocker locker{&mutex_};
    static_cast<void>(handlers_.erase(subscriptionId));
}

/**
 * @brief Coalesces a platform reachability event and dispatches observers.
 * @param[in] reachability New Qt platform state.
 * @pre Invoked by the QNetworkInformation signal connection.
 * @post Changed state is stored and each snapshot observer is invoked once.
 */
void NetworkReachabilityService::handleReachabilityChanged(
    const QNetworkInformation::Reachability reachability)
{
    const bool isReachable = reachability == QNetworkInformation::Reachability::Online;
    std::vector<ReachabilityChangedHandler> handlers{};

    {
        const QMutexLocker locker{&mutex_};
        // Platform backends may repeat the same state; suppress redundant callbacks.
        if (isReachable_ == isReachable) {
            return;
        }

        isReachable_ = isReachable;
        handlers.reserve(handlers_.size());
        for (const auto &[subscriptionId, handler] : handlers_) {
            static_cast<void>(subscriptionId);
            handlers.push_back(handler);
        }
    }

    // Invoke outside the mutex so callbacks may query or unsubscribe without deadlocking.
    for (const ReachabilityChangedHandler &handler : handlers) {
        handler(isReachable);
    }
}
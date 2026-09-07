/**
 * @file     : i_network_reachability_service.h
 * @brief    : Declares the network reachability service abstraction.
 * @details  : Defines the Service-layer contract for querying connectivity and
 * subscribing to changes without exposing a platform network-information API.
 * Architecture role: Service interface in the MVVM Model layer.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

/** @brief Prevents duplicate declarations within a translation unit. */
#pragma once

#include <cstdint>
#include <functional>

/**
 * @brief Abstracts current network reachability and observer registration.
 * @details Subscribers receive identifiers that can later remove their handlers.
 * @note Thread safety and callback execution context are implementation-defined;
 * consumers must follow the guarantee of the injected implementation.
 * Copy and move semantics are intentionally not constrained by this interface.
 */
class INetworkReachabilityService
{
public:
    /** @brief Callback invoked with the new reachable state. */
    using ReachabilityChangedHandler = std::function<void(bool)>;
    /** @brief Opaque identifier associated with one registered callback. */
    using SubscriptionId = std::uint64_t;

    /** @brief Enables polymorphic destruction. @note Implicitly noexcept. */
    virtual ~INetworkReachabilityService() = default;

    /**
     * @brief Queries the most recently observed reachability state.
     * @return true when the implementation considers the network reachable.
     * @note This operation is non-throwing; thread safety is implementation-defined.
     */
    [[nodiscard]] virtual bool isReachable() const noexcept = 0;
    /**
     * @brief Registers a reachability-change observer.
     * @param[in] handler Callable to retain and invoke for future changes.
     * @return Opaque subscription identifier used for removal.
     * @note Callback thread and exception behavior are implementation-defined.
     */
    [[nodiscard]] virtual SubscriptionId addReachabilityChangedHandler(
        ReachabilityChangedHandler handler) = 0;
    /**
     * @brief Removes a previously registered observer.
     * @param[in] subscriptionId Identifier returned by addReachabilityChangedHandler().
     * @return Nothing.
     * @note Non-throwing; unknown identifiers are ignored by conforming implementations.
     */
    virtual void removeReachabilityChangedHandler(SubscriptionId subscriptionId) noexcept = 0;
};
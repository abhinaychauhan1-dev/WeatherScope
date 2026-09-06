/**
 * @file     : application_view_model.cpp
 * @brief    : Implements the application-level ViewModel foundation.
 * @details  : Provides the constructor declared in application_view_model.h;
 * the type currently establishes only a QObject and QML registration boundary
 * for future application-scoped presentation state.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

#include "viewmodel/application_view_model.h"

/**
 * @brief Establishes the application-level QObject lifecycle boundary.
 * @param[in] parent Optional QObject owner.
 * @pre parent, when non-null, belongs to the same thread as the new object.
 * @post The ViewModel is ready for affinity-thread use with no mutable state set.
 */
ApplicationViewModel::ApplicationViewModel(QObject *parent)
    : QObject(parent)
{
}
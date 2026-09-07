/*!
 * @file     : Main.qml
 * \qmltype Main
 * \inqmlmodule WeatherGlobe
 * \brief Defines the root application window and presentation composition.
 *
 * Main owns the top-level 1280 by 720 pixel window, with a minimum supported
 * viewport of 760 by 540 pixels. A required WeatherMainViewModel controls
 * startup visibility and is forwarded to both the 3D scene and HUD overlay.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

import QtQuick
import QtQuick.Window
import WeatherGlobe

Window {
    id: root

    /*!
     * \qmlproperty WeatherMainViewModel Main::viewModel
    * Required, non-null application ViewModel. Loading and recoverable error
    * state remains visible before the first forecast becomes available.
     */
    required property WeatherMainViewModel viewModel

    // =========================================================
    // Visual Background & Root Window Constraints
    // =========================================================
    width: 1280
    height: 720
    minimumWidth: 760
    minimumHeight: 540
    visible: true
    title: qsTr("Weather Globe", "Application window title")
    color: "#071015"

    // =========================================================
    // Child Layouts & Full-Viewport Composition
    // =========================================================
    // Both layers fill the same viewport: the HUD remains above the scene by
    // declaration order while preserving the globe as the visual background.
    GlobeView3D {
        anchors.fill: parent
        viewModel: root.viewModel
    }

    WeatherHud {
        anchors.fill: parent
        visible: root.viewModel.weatherDataReady
        viewModel: root.viewModel
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, Math.max(280, statusText.implicitWidth + 40))
        implicitHeight: statusText.implicitHeight + 32
        visible: !root.viewModel.weatherDataReady
        radius: 8
        color: "#D918242B"
        border.color: root.viewModel.errorText.length > 0 ? "#FF8E87" : "#55FFFFFF"
        border.width: 1

        Text {
            id: statusText

            anchors.fill: parent
            anchors.margins: 16
            color: "#F4F7F8"
            font.family: "Segoe UI Variable Text"
            font.pixelSize: 15
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: root.viewModel.errorText.length > 0
                ? root.viewModel.errorText
                : qsTr("Loading weather data...", "Initial weather loading status")
        }
    }
}
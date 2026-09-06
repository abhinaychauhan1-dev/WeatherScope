/*!
 * @file     : HourlyCardDelegate.qml
 * \qmltype HourlyCardDelegate
 * \inqmlmodule WeatherGlobe
 * \brief Defines one card in the horizontally scrolling hourly forecast.
 *
 * The delegate expects role properties from HourlyForecastViewModel. Its
 * default height is 116 pixels and its width adapts between 144 and 184 pixels
 * to translated condition text while preserving the timeline rhythm.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    // =========================================================
    // Delegate Properties & Model Contracts
    // =========================================================
    /*! \qmlproperty string HourlyCardDelegate::hourLabel
     * Required UTC hour label in HH:mm form; no default. */
    required property string hourLabel
    /*! \qmlproperty real HourlyCardDelegate::temperature
     * Required forecast temperature in degrees Celsius; no default. */
    required property real temperature
    /*! \qmlproperty int HourlyCardDelegate::conditionCode
     * Required WMO weather interpretation code; no default. */
    required property int conditionCode
    /*! \qmlproperty int HourlyCardDelegate::rainChance
     * Required precipitation probability in percent, normally 0 through 100. */
    required property int rainChance
    /*! \qmlproperty string HourlyCardDelegate::conditionDescription
     * Required localized weather summary supplied by the ViewModel. */
    required property string conditionDescription
    /*! \qmlproperty string HourlyCardDelegate::temperatureText
     * Required locale-aware temperature including its unit. */
    required property string temperatureText
    /*! \qmlproperty string HourlyCardDelegate::rainChanceText
     * Required locale-aware precipitation percentage. */
    required property string rainChanceText

    // =========================================================
    // Visual Background & Glassmorphic Card Styling
    // =========================================================
    // TextMetrics drives width within stable bounds so translations gain room
    // without allowing one card to dominate the horizontal timeline.
    implicitWidth: Math.max(144, Math.min(184, conditionTextMetrics.advanceWidth + 24))
    implicitHeight: 116
    radius: 8
    color: "#B8202C33"
    border.color: "#2FFFFFFF"
    border.width: 1

    // =========================================================
    // Child Layouts & Anchors
    // =========================================================
    Label {
        id: hourText

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 12
        color: "#B7C8CF"
        font.family: "Segoe UI Variable Text"
        font.pixelSize: 13
        font.weight: Font.DemiBold
        text: root.hourLabel
    }

    WeatherConditionIcon {
        id: conditionIcon

        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 9
        width: 42
        height: 42
        conditionCode: root.conditionCode
    }

    TextMetrics {
        id: conditionTextMetrics

        // Measure with the rendered font so width reacts to locale and font fallback.
        font: conditionText.font
        text: root.conditionDescription
    }

    Label {
        id: conditionText

        anchors.top: conditionIcon.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        height: 28
        color: "#F7FAFB"
        font.family: "Segoe UI Variable Display"
        font.pixelSize: 11
        // Fit down to 9 pixels, then wrap over at most two lines inside the fixed height.
        fontSizeMode: Text.Fit
        minimumPixelSize: 9
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        text: root.conditionDescription
    }

    Label {
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 9
        color: "#FFFFFF"
        font.family: "Segoe UI Variable Display"
        font.pixelSize: 22
        font.weight: Font.DemiBold
        text: root.temperatureText
    }

    Rectangle {
        anchors.right: parent.right
        anchors.rightMargin: 9
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        // Content width plus 14 pixels preserves balanced horizontal badge padding.
        width: rainText.implicitWidth + 14
        height: 24
        radius: 8
        color: "#334F91A8"
        border.color: "#5578D6F0"
        border.width: 1

        Label {
            id: rainText

            anchors.centerIn: parent
            color: "#AEEAFF"
            font.family: "Segoe UI Variable Text"
            font.pixelSize: 12
            font.weight: Font.DemiBold
            text: root.rainChanceText
        }
    }
}
/*!
 * @file     : DailyCardDelegate.qml
 * \qmltype DailyCardDelegate
 * \inqmlmodule WeatherGlobe
 * \brief Defines one locale-aware row in the seven-day forecast list.
 *
 * The delegate expects role properties from a DailyForecastViewModel and a
 * ListView parent. It fills the owning list width and divides the available
 * list height evenly among all rows; WeatherHud supplies the containing panel.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root

    // =========================================================
    // Delegate Properties & Model Contracts
    // =========================================================
    /*! \qmlproperty int DailyCardDelegate::index
     * Required zero-based row index supplied by ListView; no default. */
    required property int index
    /*! \qmlproperty string DailyCardDelegate::dateLabel
     * Required ISO 8601 date string; no default. */
    required property string dateLabel
    /*! \qmlproperty int DailyCardDelegate::conditionCode
     * Required WMO weather interpretation code; no default. */
    required property int conditionCode
    /*! \qmlproperty string DailyCardDelegate::conditionDescription
     * Required localized weather summary supplied by the ViewModel. */
    required property string conditionDescription
    /*! \qmlproperty string DailyCardDelegate::temperatureRangeText
     * Required locale-aware minimum/maximum temperature label. */
    required property string temperatureRangeText

    /*! \qmlproperty var DailyCardDelegate::owningList
     * Read-only owning ListView reference; null outside a delegate context. */
    readonly property var owningList: ListView.view

    // =========================================================
    // Child Layouts & Responsive Row Sizing
    // =========================================================
    width: owningList ? owningList.width : 0
    // Divide the list evenly and guard against division by zero during model reset.
    height: owningList ? owningList.height / Math.max(1, owningList.count) : 0

    Label {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 44
        color: "#F4F7F8"
        font.family: "Segoe UI Variable Text"
        font.pixelSize: 13
        fontSizeMode: Text.Fit
        minimumPixelSize: 9
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignLeft
        text: root.dateLabel
    }

    WeatherConditionIcon {
        id: conditionIcon

        anchors.left: parent.left
        anchors.leftMargin: 46
        anchors.verticalCenter: parent.verticalCenter
        width: 34
        height: 34
        conditionCode: root.conditionCode
    }

    Label {
        anchors.left: conditionIcon.right
        anchors.leftMargin: 5
        anchors.right: temperatureRange.left
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        color: "#AFC2CB"
        // Preserve temperature visibility by eliding variable-length translated text first.
        elide: Text.ElideRight
        maximumLineCount: 1
        font.family: "Segoe UI Variable Text"
        font.pixelSize: 12
        text: root.conditionDescription
    }

    Label {
        id: temperatureRange

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        color: "#FFFFFF"
        font.family: "Segoe UI Variable Text"
        font.pixelSize: 12
        font.weight: Font.DemiBold
          text: root.temperatureRangeText
    }

    // =========================================================
    // Visual Separator Styling
    // =========================================================
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        visible: root.owningList && (root.index < (root.owningList.count - 1))
        color: "#1FFFFFFF"
    }
}
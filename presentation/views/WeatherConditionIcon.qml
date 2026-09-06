/*!
 * @file     : WeatherConditionIcon.qml
 * \qmltype WeatherConditionIcon
 * \inqmlmodule WeatherGlobe
 * \brief Maps a WMO condition code to a scalable weather icon.
 *
 * The component has no ViewModel dependency. Its parent must provide width and
 * height in pixels; the image preserves aspect ratio and loads the embedded SVG
 * asynchronously from the WeatherGlobe resource namespace.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

import QtQuick

Image {
    id: root

    // =========================================================
    // Public Property Contract
    // =========================================================
    /*! \qmlproperty int WeatherConditionIcon::conditionCode
     * Required WMO weather interpretation code; no default. */
    required property int conditionCode

    /*! Returns the embedded SVG URL associated with a WMO weather family. */
    function iconSource(code) {
        if (code === 0) {
            return "qrc:/qt/qml/WeatherGlobe/assets/weather/clear.svg"
        }
        if ((code === 1) || (code === 2)) {
            return "qrc:/qt/qml/WeatherGlobe/assets/weather/partly-cloudy.svg"
        }
        if (code === 3) {
            return "qrc:/qt/qml/WeatherGlobe/assets/weather/cloudy.svg"
        }
        if ((code === 45) || (code === 48)) {
            return "qrc:/qt/qml/WeatherGlobe/assets/weather/fog.svg"
        }
        if (((code >= 71) && (code <= 77))
                || ((code >= 85) && (code <= 86))) {
            return "qrc:/qt/qml/WeatherGlobe/assets/weather/snow.svg"
        }
        if ((code >= 80) && (code <= 82)) {
            return "qrc:/qt/qml/WeatherGlobe/assets/weather/showers.svg"
        }
        if (code >= 95) {
            return "qrc:/qt/qml/WeatherGlobe/assets/weather/storm.svg"
        }
        // Rain is the conservative visual fallback for unrecognized precipitation codes.
        return "qrc:/qt/qml/WeatherGlobe/assets/weather/rain.svg"
    }

    // =========================================================
    // Visual Asset Loading & Scaling
    // =========================================================
    source: iconSource(conditionCode)
    // Preserve SVG proportions at every delegate size and avoid blocking UI creation.
    fillMode: Image.PreserveAspectFit
    asynchronous: true
    smooth: true
    mipmap: true
}
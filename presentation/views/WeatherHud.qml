/*!
 * @file     : WeatherHud.qml
 * \qmltype WeatherHud
 * \inqmlmodule WeatherGlobe
 * \brief Presents the interactive weather heads-up display over the 3D globe.
 *
 * WeatherHud composes the city search, language selector, connectivity status,
 * current conditions, daily forecast, and horizontally scrollable hourly
 * forecast. The component must be supplied with a live WeatherMainViewModel;
 * all displayed weather data and user commands flow through that binding.
 *
 * The component is intended to fill its parent viewport. Its implicit default
 * size is 960 by 640 pixels, while the contained panels apply bounded,
 * content-aware sizing to remain usable as the viewport changes.
 *
 * \author Abhinay Chauhan (email: chauhan089306@gmail.com)
 * \since WeatherGlobe 1.0
 * Version: 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

import QtQuick
import QtQuick.Controls.Basic
import WeatherGlobe

pragma ComponentBehavior: Bound

Item {
    id: root

    /*!
     * \qmlproperty WeatherMainViewModel WeatherHud::viewModel
     * Required, non-null ViewModel that supplies weather state, forecast models,
     * localization choices, search results, and invokable user commands.
     */
    required property WeatherMainViewModel viewModel

    /*!
     * \qmlproperty var WeatherHud::locationSuggestions
     * Current geocoding result array displayed by the search popup. Defaults to
     * an empty array and is replaced whenever the ViewModel publishes results.
     */
    property var locationSuggestions: []

    /*!
     * \qmlproperty bool WeatherHud::locationDropdownOpen
     * Controls search-popup visibility. Defaults to false; visibility also
     * requires at least one location suggestion.
     */
    property bool locationDropdownOpen: false

    /*!
     * \qmlproperty bool WeatherHud::acceptingLocationSuggestion
     * One-shot guard that distinguishes a programmatic search-field update from
     * user input. Defaults to false and resets on the next text change.
     */
    property bool acceptingLocationSuggestion: false

    implicitWidth: 960
    implicitHeight: 640

    // =========================================================
    // Child Layouts & Anchors: Top Control Bar
    // =========================================================
    Row {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 24
        height: 44
        spacing: 12
        z: 10

        Item {
            id: locationSearch

            // Three 12-pixel Row gaps are reserved after subtracting the fixed
            // controls; clamping prevents an invalid negative width in narrow viewports.
            width: Math.max(
                0,
                topBar.width - languageSelector.width - statusBadge.width
                    - refreshButton.width - 36)
            height: 40
            anchors.verticalCenter: parent.verticalCenter
            z: 10

            TextField {
                id: citySearchField

                anchors.fill: parent
                leftPadding: 42
                rightPadding: 14
                color: "#F4F7F8"
                font.family: "Segoe UI Variable Text"
                font.pixelSize: 15
                placeholderText: qsTr("Search city...", "City search input placeholder")
                placeholderTextColor: "#9DB0B9"
                selectByMouse: true

                background: Rectangle {
                    radius: 8
                    color: citySearchField.activeFocus ? "#CC18242B" : "#A618242B"
                    border.color: citySearchField.activeFocus ? "#8EDBC2" : "#55FFFFFF"
                    border.width: 1
                }

                // =========================================================
                // Interactive Event Handlers: Location Search
                // =========================================================
                onTextChanged: {
                    // Selecting a result writes its city into the field. Consume
                    // that one change so it does not immediately launch another search.
                    if (root.acceptingLocationSuggestion) {
                        root.acceptingLocationSuggestion = false
                        return
                    }

                    root.viewModel.searchCity(text)
                    // Fewer than three characters produce noisy matches; clear
                    // any previous results while the service handles the short query.
                    if (text.trim().length < 3) {
                        root.locationSuggestions = []
                        root.locationDropdownOpen = false
                    }
                }
                // Submission and Escape dismiss suggestions without discarding the query.
                onAccepted: root.locationDropdownOpen = false
                onActiveFocusChanged: {
                    // Restore existing suggestions when keyboard focus returns to the field.
                    if (activeFocus && (root.locationSuggestions.length > 0)) {
                        root.locationDropdownOpen = true
                    }
                }
                Keys.onEscapePressed: root.locationDropdownOpen = false
            }

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                color: "#BFD0D7"
                font.pixelSize: 17
                text: "\uD83D\uDD0D"
            }

            Rectangle {
                id: locationDropdown

                anchors.top: citySearchField.bottom
                anchors.topMargin: 6
                width: parent.width
                height: Math.min(240, suggestionList.contentHeight)
                visible: root.locationDropdownOpen && (root.locationSuggestions.length > 0)
                radius: 8
                color: "#F218242B"
                border.color: "#55FFFFFF"
                border.width: 1
                clip: true

                ListView {
                    id: suggestionList

                    /*!
                     * Commits a geocoding result to the ViewModel. Popup state is
                     * cleared first, and the text-change guard prevents a feedback search.
                     */
                    function selectSuggestion(suggestion) {
                        root.locationDropdownOpen = false
                        root.locationSuggestions = []
                        root.acceptingLocationSuggestion = true
                        citySearchField.text = suggestion.cityName
                        root.viewModel.selectLocation(
                            suggestion.cityName,
                            suggestion.country,
                            suggestion.countryCode,
                            suggestion.latitude,
                            suggestion.longitude)
                    }

                    anchors.fill: parent
                    model: root.locationSuggestions
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    delegate: ItemDelegate {
                        id: suggestionDelegate

                        /*! Index-free result object injected by the ListView delegate context. */
                        required property var modelData

                        width: suggestionList.width
                        height: 48
                        leftPadding: 14
                        rightPadding: 14
                        text: qsTr("%1, %2", "Location suggestion: city, country")
                            .arg(modelData.cityName)
                            .arg(modelData.country)

                        contentItem: Label {
                            color: "#F4F7F8"
                            // Long administrative names remain on one line without
                            // expanding the popup beyond its search-field width.
                            elide: Text.ElideRight
                            font.family: "Segoe UI Variable Text"
                            font.pixelSize: 14
                            text: suggestionDelegate.text
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: suggestionDelegate.hovered ? "#354852" : "transparent"
                        }

                        onClicked: {
                            suggestionList.selectSuggestion(modelData)
                        }
                    }
                }
            }
        }

        // =========================================================
        // State Machines, Transitions & Property Animations
        // =========================================================
        ComboBox {
            id: languageSelector

            anchors.verticalCenter: parent.verticalCenter
            // Keep every localized language name visible while reserving 36 pixels
            // for padding and the indicator; 140 pixels is the minimum control width.
            implicitWidth: Math.max(140, contentItem.implicitWidth + 36)
            height: 36
            leftPadding: 12
            rightPadding: 34
            model: root.viewModel.supportedLanguages

            Binding on currentIndex {
                // Maintain ViewModel authority over locale changes while allowing
                // ComboBox activation to send user selections in the opposite direction.
                value: root.viewModel.currentLanguageIndex
                restoreMode: Binding.RestoreBindingOrValue
            }

            contentItem: Label {
                leftPadding: 10
                rightPadding: 28
                color: "#EAF2F5"
                elide: Text.ElideNone
                font.family: "Segoe UI Variable Text"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                text: languageSelector.displayText
                verticalAlignment: Text.AlignVCenter
            }

            indicator: Label {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                color: languageSelector.popup.visible ? "#8EDBC2" : "#AFC2CB"
                font.pixelSize: 12
                text: "\u25BE"

                Behavior on color {
                    // A short color blend communicates popup state without moving layout.
                    ColorAnimation { duration: 140 }
                }
            }

            background: Rectangle {
                radius: 8
                color: languageSelector.hovered || languageSelector.popup.visible
                    ? "#D0203038"
                    : "#A618242B"
                border.color: languageSelector.popup.visible ? "#8EDBC2" : "#55FFFFFF"
                border.width: 1

                Behavior on color {
                    ColorAnimation { duration: 140 }
                }
                Behavior on border.color {
                    ColorAnimation { duration: 140 }
                }
            }

            delegate: ItemDelegate {
                id: languageDelegate

                /*! Zero-based language index injected by the ComboBox delegate model. */
                required property int index
                /*! Localized language label injected by the ComboBox delegate model. */
                required property var modelData

                width: parent ? parent.width : 0
                height: 38
                padding: 10
                highlighted: languageSelector.highlightedIndex === index

                contentItem: Label {
                    clip: false
                    color: languageDelegate.highlighted ? "#D5FFF2" : "#E4ECEF"
                    elide: Text.ElideNone
                    font.family: "Segoe UI Variable Text"
                    font.pixelSize: 13
                    font.weight: languageDelegate.highlighted ? Font.DemiBold : Font.Normal
                    text: languageDelegate.modelData
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 6
                          color: languageDelegate.highlighted ? "#485A716E"
                                                                          : (languageDelegate.hovered
                                                                              ? "#3045555D"
                                                                              : "transparent")

                    Behavior on color {
                        ColorAnimation { duration: 120 }
                    }
                }
            }

            popup: Popup {
                y: languageSelector.height + 6
                // Match the selector unless content needs the 160-pixel readability floor.
                width: Math.max(languageSelector.width, 160)
                // Follow content height up to 360 pixels, after which the ListView scrolls.
                implicitHeight: Math.min(contentItem.implicitHeight + 12, 360)
                padding: 6

                contentItem: ListView {
                    implicitHeight: contentHeight
                    clip: true
                    model: languageSelector.popup.visible
                        ? languageSelector.delegateModel
                        : null
                    currentIndex: languageSelector.highlightedIndex
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollIndicator.vertical: ScrollIndicator {}
                }

                background: Rectangle {
                    radius: 8
                    color: "#F018252C"
                    border.color: "#667B929D"
                    border.width: 1
                }

                enter: Transition {
                    // Fast asymmetric fades make opening legible and dismissal immediate.
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 120 }
                }
                exit: Transition {
                    NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 90 }
                }
            }

            onActivated: (index) => root.viewModel.setLanguageIndex(index)
        }

        Rectangle {
            id: statusBadge
            anchors.verticalCenter: parent.verticalCenter
            width: statusLabel.implicitWidth + 24
            height: 30
            radius: 15
            color: root.viewModel.isOnline ? "#244D3B" : "#593536"
            border.color: root.viewModel.isOnline ? "#69D19B" : "#FF8E87"
            border.width: 1

            Label {
                id: statusLabel
                anchors.centerIn: parent
                color: root.viewModel.isOnline ? "#B9F5D3" : "#FFD0CC"
                font.family: "Segoe UI Variable Text"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                text: root.viewModel.onlineStatusText
            }
        }

        Button {
            id: refreshButton
            anchors.verticalCenter: parent.verticalCenter
            height: 36
            text: qsTr("Refresh", "Refresh weather button")
            onClicked: root.viewModel.requestRefresh(
                root.viewModel.latitude,
                root.viewModel.longitude)

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Refresh weather", "Refresh button tooltip")
        }
    }

    // =========================================================
    // Interactive Event Handlers: Asynchronous Search Results
    // =========================================================
    Connections {
        target: root.viewModel

        function onLocationSearchResults(results) {
            root.locationSuggestions = results
            // Async results open the popup only while the originating field still
            // owns focus, preventing a late network response from stealing attention.
            root.locationDropdownOpen = citySearchField.activeFocus && (results.length > 0)
        }
    }

    // =========================================================
    // Visual Background & Glassmorphic Current-Conditions Panel
    // =========================================================
    Rectangle {
        id: currentWeatherCard

        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -30
        // Balance a 280-pixel readability floor against the space left by the
        // daily panel; cap at 340 pixels to preserve the globe as the visual focus.
        width: Math.min(340, Math.max(280, parent.width - dailyForecastCard.width - 96))
        height: 260
        radius: 8
        color: "#B31A242B"
        border.color: "#66FFFFFF"
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 5

            Label {
                width: parent.width
                color: "#FFFFFF"
                // City/country names are external data and may exceed the fixed card width.
                elide: Text.ElideRight
                font.family: "Segoe UI Variable Display"
                font.pixelSize: 20
                font.weight: Font.DemiBold
                text: qsTr("%1, %2", "Selected location: city, country")
                    .arg(root.viewModel.selectedLocationName)
                    .arg(root.viewModel.selectedCountry)
            }

            Label {
                width: parent.width
                color: "#91A7B1"
                // Coordinate text stays single-line to preserve the card's fixed height.
                elide: Text.ElideRight
                font.family: "Segoe UI Variable Text"
                font.pixelSize: 12
                text: root.viewModel.locationText
            }

            Label {
                color: "#AFC2CB"
                font.family: "Segoe UI Variable Text"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                text: root.viewModel.isCachedData
                    ? qsTr("LATEST CACHED OBSERVATION", "Current weather section heading")
                    : qsTr("CURRENT CONDITIONS", "Current weather section heading")
            }

            Label {
                color: "#FFFFFF"
                font.family: "Segoe UI Variable Display"
                font.pixelSize: 40
                font.weight: Font.Light
                text: root.viewModel.temperatureText
            }

            Label {
                width: parent.width
                color: "#F4F7F8"
                font.family: "Segoe UI Variable Text"
                font.pixelSize: 22
                fontSizeMode: Text.Fit
                minimumPixelSize: 14
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                maximumLineCount: 1
                text: root.viewModel.conditionDescription
            }

            Row {
                spacing: 7

                /*!
                 * \qmlproperty color rainColor
                 * Emphasis color for precipitation probability. Defaults dynamically
                 * to muted gray at 50 percent or below and cyan above 50 percent.
                 */
                readonly property color rainColor:
                    root.viewModel.currentRainChance > 50 ? "#79DDF2" : "#AFC2CB"

                Label {
                    color: parent.rainColor
                    font.family: "Segoe UI Emoji"
                    font.pixelSize: 15
                    text: "\uD83D\uDCA7"
                }

                Label {
                    color: parent.rainColor
                    font.family: "Segoe UI Variable Text"
                    font.pixelSize: 15
                    text: qsTr("Precipitation: %1%", "Current precipitation probability")
                        .arg(root.viewModel.currentRainChance)
                }
            }

            Row {
                spacing: 7

                Label {
                    color: "#AFC2CB"
                    font.family: "Segoe UI Variable Text"
                    font.pixelSize: 15
                    text: "\u25CF"
                }

                Label {
                    color: "#AFC2CB"
                    font.family: "Segoe UI Variable Text"
                    font.pixelSize: 15
                    text: qsTr("Humidity: %1%", "Current relative humidity")
                        .arg(root.viewModel.currentHumidity)
                }
            }

            Row {
                spacing: 7

                Label {
                    color: "#AFC2CB"
                    font.family: "Segoe UI Variable Text"
                    font.pixelSize: 15
                    text: "\u2192"
                }

                Label {
                    color: "#AFC2CB"
                    font.family: "Segoe UI Variable Text"
                    font.pixelSize: 15
                    text: qsTr("Wind: %1", "Current wind speed")
                        .arg(root.viewModel.windText)
                }
            }
        }
    }

    // =========================================================
    // Child Layouts & Anchors: Daily Forecast Panel
    // =========================================================
    Rectangle {
        id: dailyForecastCard

        anchors.top: topBar.bottom
        anchors.topMargin: 18
        anchors.right: parent.right
        anchors.rightMargin: 24
        anchors.bottom: hourlyTimeline.top
        anchors.bottomMargin: 18
        // Scale with viewport width while preserving practical 230-280 pixel bounds.
        width: Math.min(280, Math.max(230, parent.width * 0.23))
        radius: 8
        color: "#B818242B"
        border.color: "#3DFFFFFF"
        border.width: 1

        Label {
            id: dailyForecastTitle

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 17
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            color: "#F4F7F8"
            font.family: "Segoe UI Variable Display"
            font.pixelSize: 18
            font.weight: Font.DemiBold
            text: qsTr("7-Day Forecast", "Daily forecast panel title")
        }

        Rectangle {
            anchors.top: dailyForecastTitle.bottom
            anchors.topMargin: 12
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            height: 1
            color: "#2FFFFFFF"
        }

        ListView {
            id: dailyForecastList

            anchors.top: dailyForecastTitle.bottom
            anchors.topMargin: 18
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.bottomMargin: 10
            // The seven rows are designed to fit the bounded panel; disabling
            // interaction prevents this list from competing with hourly scrolling.
            interactive: false
            clip: true
            model: root.viewModel.dailyModel

            delegate: DailyCardDelegate {}
        }
    }

    // =========================================================
    // Child Layouts & Interactive Hourly Timeline
    // =========================================================
    Rectangle {
        id: hourlyTimeline

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        height: 144
        radius: 8
        color: "#A618242B"
        border.color: "#3DFFFFFF"
        border.width: 1

        ListView {
            id: hourlyForecast

            anchors.fill: parent
            anchors.margins: 10
            orientation: ListView.Horizontal
            spacing: 10
            clip: true
            boundsBehavior: Flickable.DragOverBounds
            boundsMovement: Flickable.StopAtBounds
            // Pixel-per-second tuning gives mouse and touchpad flicks momentum
            // without allowing the short hourly strip to become difficult to stop.
            flickDeceleration: 2600
            maximumFlickVelocity: 4200
            model: root.viewModel.hourlyModel
            delegate: HourlyCardDelegate {}

            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            // =========================================================
            // Interactive Event Handlers & Scroll Animation
            // =========================================================
            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

                onWheel: function(event) {
                    // Normalize horizontal/vertical pixel and angle deltas so
                    // conventional wheels and precision touchpads both pan the timeline.
                    var wheelDelta = event.pixelDelta.x
                    if (wheelDelta === 0) {
                        wheelDelta = event.pixelDelta.y
                    }
                    if (wheelDelta === 0) {
                        wheelDelta = event.angleDelta.x
                    }
                    if (wheelDelta === 0) {
                        wheelDelta = event.angleDelta.y
                    }

                    var maximumContentX = Math.max(
                        0,
                        hourlyForecast.contentWidth - hourlyForecast.width)
                    // Clamp before animating to avoid overscroll gaps at either boundary.
                    var requestedContentX = Math.max(
                        0,
                        Math.min(maximumContentX, hourlyForecast.contentX - wheelDelta))
                    wheelScroll.stop()
                    wheelScroll.from = hourlyForecast.contentX
                    wheelScroll.to = requestedContentX
                    wheelScroll.start()
                    event.accepted = true
                }
            }

            NumberAnimation {
                id: wheelScroll

                target: hourlyForecast
                property: "contentX"
                // Restartable 140-millisecond ease-out absorbs discrete wheel steps
                // while converging quickly enough for repeated navigation.
                duration: 140
                easing.type: Easing.OutCubic
            }
        }
    }
}
/*!
 * @file     : GlobeView3D.qml
 * \qmltype GlobeView3D
 * \inqmlmodule WeatherGlobe
 * \brief Defines the interactive weather globe and particle scene.
 *
 * GlobeView3D renders a full-parent 3D viewport with an orbit camera, selected
 * location marker, weather material, cloud shell, and rain/snow particles. It
 * requires WeatherMainViewModel coordinates and rotations in degrees, current
 * temperature in degrees Celsius, WMO condition code, connectivity/cache state,
 * and shader parameters. The parent should provide at least the component's
 * implicit 960 by 640 pixel size for the intended camera framing.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

import QtQuick
import QtQuick3D
import QtQuick3D.Particles3D
import WeatherGlobe

pragma ComponentBehavior: Bound

Item {
    id: root

    // =========================================================
    // Public API & Derived Weather State
    // =========================================================
    /*! \qmlproperty WeatherMainViewModel GlobeView3D::viewModel
     * Required, non-null source of weather, coordinates, and globe targets. */
    required property WeatherMainViewModel viewModel

    /*! \qmlproperty bool GlobeView3D::rainActive
     * Read-only WMO rain/drizzle/shower classification; false by default. */
    readonly property bool rainActive:
        ((viewModel.wmoWeatherCode >= 51) && (viewModel.wmoWeatherCode <= 67))
        || ((viewModel.wmoWeatherCode >= 80) && (viewModel.wmoWeatherCode <= 82))
    /*! \qmlproperty bool GlobeView3D::snowActive
     * Read-only WMO snow classification; false by default. */
    readonly property bool snowActive:
        ((viewModel.wmoWeatherCode >= 71) && (viewModel.wmoWeatherCode <= 77))
        || ((viewModel.wmoWeatherCode >= 85) && (viewModel.wmoWeatherCode <= 86))
    /*! \qmlproperty real GlobeView3D::rainEmissionRate
     * Rain emission rate in particles per second; defaults dynamically to 220
     * while rainActive is true and zero otherwise. */
    property real rainEmissionRate: rainActive ? 220 : 0
    /*! \qmlproperty real GlobeView3D::rainOpacity
     * Unitless rain-particle opacity in [0, 1]; defaults to 0.78 when active. */
    property real rainOpacity: rainActive ? 0.78 : 0.0
    /*! \qmlproperty real GlobeView3D::snowEmissionRate
     * Snow emission rate in particles per second; defaults dynamically to 72
     * while snowActive is true and zero otherwise. */
    property real snowEmissionRate: snowActive ? 72 : 0
    /*! \qmlproperty real GlobeView3D::snowOpacity
     * Unitless snow-particle opacity in [0, 1]; defaults to 0.92 when active. */
    property real snowOpacity: snowActive ? 0.92 : 0.0

    /*! Resets user orbit and pan offsets so ViewModel target rotations can center a selection. */
    function centerSelectedLocation() {
        cameraRig.orbitPitch = 0
        cameraRig.orbitYaw = 0
        cameraRig.panX = 0
        cameraRig.panY = 0
    }

    implicitWidth: 960
    implicitHeight: 640
    clip: true

    // =========================================================
    // State Transitions & Property Animations
    // =========================================================
    Connections {
        target: root.viewModel

        function onTargetRotationChanged() {
            // A selected location owns the new framing; discard prior manual offsets.
            root.centerSelectedLocation()
        }
    }

    Behavior on rainEmissionRate {
        // Emission ramps avoid abrupt particle-count discontinuities after code changes.
        NumberAnimation { duration: 700; easing.type: Easing.InOutCubic }
    }
    Behavior on rainOpacity {
        NumberAnimation { duration: 500; easing.type: Easing.InOutCubic }
    }
    Behavior on snowEmissionRate {
        // Snow transitions more slowly than rain to preserve its visually gentle onset.
        NumberAnimation { duration: 1100; easing.type: Easing.InOutCubic }
    }
    Behavior on snowOpacity {
        NumberAnimation { duration: 900; easing.type: Easing.InOutCubic }
    }

    // =========================================================
    // Visual Background, 3D Viewport & Camera Layout
    // =========================================================
    View3D {
        id: viewport
        anchors.fill: parent
        camera: sceneCamera

        environment: SceneEnvironment {
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.Medium
            backgroundMode: SceneEnvironment.Transparent
        }

        Node {
            id: cameraRig

            /*! User orbit pitch in degrees; defaults to 0. */
            property real orbitPitch: 0
            /*! User orbit yaw in degrees; defaults to 0. */
            property real orbitYaw: 0
            /*! Camera-rig horizontal translation in scene units; defaults to 0. */
            property real panX: 0
            /*! Camera-rig vertical translation in scene units; defaults to 0. */
            property real panY: 0
            /*! Camera distance from the globe in scene units; defaults to 520. */
            property real distance: 520

            position: Qt.vector3d(panX, panY, 0)

            Behavior on panX {
                SmoothedAnimation { velocity: 420 }
            }
            Behavior on panY {
                SmoothedAnimation { velocity: 420 }
            }
            Behavior on orbitPitch {
                SmoothedAnimation { velocity: 260 }
            }
            Behavior on orbitYaw {
                SmoothedAnimation { velocity: 260 }
            }
            Behavior on distance {
                SmoothedAnimation { velocity: 700 }
            }

            Node {
                eulerRotation.x: cameraRig.orbitPitch
                eulerRotation.y: cameraRig.orbitYaw

                PerspectiveCamera {
                    id: sceneCamera
                    position: Qt.vector3d(0, 0, cameraRig.distance)
                    clipNear: 10
                    clipFar: 1800
                    fieldOfView: 42
                }
            }
        }

        // =========================================================
        // Lighting
        // =========================================================
        DirectionalLight {
            eulerRotation: Qt.vector3d(-32, -38, 0)
            brightness: root.viewModel.isOnline ? 1.15 : 0.82
            castsShadow: true
            shadowFactor: 55
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(22, 146, 0)
            color: "#709BC1"
            brightness: 0.32
        }

        // =========================================================
        // Globe Model, Weather Material & Location Marker
        // =========================================================
        Node {
            id: latitudePivot

            eulerRotation.x: root.viewModel.targetGlobeRotationX

            Behavior on eulerRotation.x {
                SmoothedAnimation {
                    duration: 1200
                    easing.type: Easing.InOutCubic
                }
            }

            Node {
                id: longitudePivot

                eulerRotation.y: root.viewModel.targetGlobeRotationY

                Behavior on eulerRotation.y {
                    SmoothedAnimation {
                        duration: 1200
                        easing.type: Easing.InOutCubic
                    }
                }

                Model {
                    id: earth

                    /*! Globe-surface marker radius in scene units; constant 50.8. */
                    readonly property real markerRadius: 50.8
                    /*! Selected latitude in radians, derived from ViewModel degrees. */
                    readonly property real latitudeRadians: root.viewModel.latitude * Math.PI / 180.0
                    /*! Selected longitude in radians, derived from ViewModel degrees. */
                    readonly property real longitudeRadians: root.viewModel.longitude * Math.PI / 180.0

                    source: "#Sphere"
                    scale: Qt.vector3d(3, 3, 3)

                    materials: EarthWeatherMaterial {
                        shaderParams: root.viewModel.shaderParams
                        currentTemperature: root.viewModel.currentTemperature
                    }

                    Node {
                        id: locationMarker

                // Convert geographic latitude/longitude to the sphere's Cartesian
                // coordinate system so the marker remains on the rendered surface.
                position: Qt.vector3d(
                    earth.markerRadius * Math.cos(earth.latitudeRadians)
                        * Math.sin(earth.longitudeRadians),
                    earth.markerRadius * Math.sin(earth.latitudeRadians),
                    earth.markerRadius * Math.cos(earth.latitudeRadians)
                        * Math.cos(earth.longitudeRadians))

                    // Counter-rotate latitude and apply longitude so the flat marker
                    // remains tangent to the curved surface at the selected point.
                eulerRotation.x: -root.viewModel.latitude
                eulerRotation.y: root.viewModel.longitude

                Model {
                    id: markerIcon

                    source: "#Rectangle"
                    position: Qt.vector3d(0, 5.8, 0.35)
                    scale: Qt.vector3d(0.09, 0.15, 1.0)
                    opacity: 1.0

                    materials: PrincipledMaterial {
                        baseColorMap: Texture {
                            source: "assets/markers/location-pin.svg"
                            generateMipmaps: true
                            minFilter: Texture.Linear
                            mipFilter: Texture.Linear
                            magFilter: Texture.Linear
                        }
                        emissiveFactor: Qt.vector3d(0.75, 0.04, 0.06)
                        alphaMode: PrincipledMaterial.Blend
                        lighting: PrincipledMaterial.NoLighting
                        cullMode: Material.NoCulling
                    }
                }
                    }
                }
            }
        }

        // =========================================================
        // Animated Cloud Layer
        // =========================================================
        Model {
            id: cloudLayer
            source: "#Sphere"
            scale: Qt.vector3d(3.035, 3.035, 3.035)
            opacity: root.viewModel.isCachedData ? 0.34 : 0.58

            materials: PrincipledMaterial {
                baseColorMap: Texture {
                    source: "assets/earth/earth-clouds.png"
                    generateMipmaps: true
                    minFilter: Texture.Linear
                    mipFilter: Texture.Linear
                    magFilter: Texture.Linear
                }
                alphaMode: PrincipledMaterial.Blend
                opacity: 0.72
                roughness: 1.0
                lighting: PrincipledMaterial.FragmentLighting
                cullMode: Material.NoCulling
                depthDrawMode: Material.OpaqueOnlyDepthDraw
            }

            NumberAnimation on eulerRotation.y {
                // One 180-second revolution supplies slow atmospheric motion.
                from: 0
                to: 360
                duration: 180000
                loops: Animation.Infinite
                running: true
            }
        }

        // =========================================================
        // Rain Particle Mapping
        // =========================================================
        // WMO rain state drives emission and opacity; lifespan, downward velocity,
        // and gravity map to a dense, fast precipitation profile.
        ParticleSystem3D {
            id: rainSystem
            // Continue through the emission fade, then suspend simulation at zero.
            running: root.rainActive || (root.rainEmissionRate > 0)

            ModelParticle3D {
                id: rainParticle
                maxAmount: 640
                fadeInDuration: 120
                fadeOutDuration: 320

                delegate: Model {
                    source: "#Cube"
                    scale: Qt.vector3d(0.012, 0.18, 0.012)
                    materials: PrincipledMaterial {
                        baseColor: "#B8DDF2"
                        opacity: root.rainOpacity
                        alphaMode: PrincipledMaterial.Blend
                        roughness: 0.25
                        lighting: PrincipledMaterial.NoLighting
                    }
                }
            }

            ParticleEmitter3D {
                particle: rainParticle
                position: Qt.vector3d(0, 230, 105)
                emitRate: root.rainEmissionRate
                lifeSpan: 1900
                lifeSpanVariation: 260
                particleScale: 1.0
                particleScaleVariation: 0.22

                shape: ParticleShape3D {
                    type: ParticleShape3D.Cube
                    extents: Qt.vector3d(420, 80, 170)
                    fill: true
                }

                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, -82, 0)
                    directionVariation: Qt.vector3d(12, 18, 8)
                }
            }

            Gravity3D {
                particles: [rainParticle]
                direction: Qt.vector3d(0, -1, 0)
                magnitude: 118
            }
        }

        // =========================================================
        // Snow Particle Mapping
        // =========================================================
        // Lower emission/gravity, longer lifespan, and Wander3D produce sparse,
        // slow flakes distinct from the rain system's directional motion.
        ParticleSystem3D {
            id: snowSystem
            // Continue through the emission fade, then suspend simulation at zero.
            running: root.snowActive || (root.snowEmissionRate > 0)

            ModelParticle3D {
                id: snowParticle
                maxAmount: 360
                fadeInDuration: 650
                fadeOutDuration: 900

                delegate: Model {
                    source: "#Sphere"
                    scale: Qt.vector3d(0.035, 0.035, 0.035)
                    materials: PrincipledMaterial {
                        baseColor: "#F5FAFF"
                        emissiveFactor: Qt.vector3d(0.18, 0.20, 0.22)
                        opacity: root.snowOpacity
                        alphaMode: PrincipledMaterial.Blend
                        roughness: 0.9
                    }
                }
            }

            ParticleEmitter3D {
                particle: snowParticle
                position: Qt.vector3d(0, 220, 110)
                emitRate: root.snowEmissionRate
                lifeSpan: 6200
                lifeSpanVariation: 900
                particleScale: 1.0
                particleScaleVariation: 0.45

                shape: ParticleShape3D {
                    type: ParticleShape3D.Cube
                    extents: Qt.vector3d(430, 90, 180)
                    fill: true
                }

                velocity: VectorDirection3D {
                    direction: Qt.vector3d(0, -19, 0)
                    directionVariation: Qt.vector3d(14, 7, 10)
                }
            }

            Gravity3D {
                particles: [snowParticle]
                direction: Qt.vector3d(0, -1, 0)
                magnitude: 9
            }

            Wander3D {
                particles: [snowParticle]
                uniqueAmount: Qt.vector3d(24, 4, 18)
                uniqueAmountVariation: 0.35
                uniquePace: Qt.vector3d(0.16, 0.08, 0.13)
                uniquePaceVariation: 0.3
                fadeInDuration: 700
            }
        }
    }

    // =========================================================
    // Interactive Event Handlers: Touch, Mouse & Wheel
    // =========================================================
    PinchArea {
        id: touchZoom
        anchors.fill: parent

        /*! Camera distance in scene units captured at pinch start; defaults to 520. */
        property real initialDistance: 520

        onPinchStarted: function(pinch) {
            initialDistance = cameraRig.distance
        }
        onPinchUpdated: function(pinch) {
            // Invert pinch scale into distance and clamp to the camera's usable range.
            cameraRig.distance = Math.max(260, Math.min(900, initialDistance / pinch.scale))
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            /*! Previous pointer X coordinate in pixels; defaults to 0. */
            property real previousX: 0
            /*! Previous pointer Y coordinate in pixels; defaults to 0. */
            property real previousY: 0

            onPressed: function(mouse) {
                previousX = mouse.x
                previousY = mouse.y
            }
            onPositionChanged: function(mouse) {
                const deltaX = mouse.x - previousX
                const deltaY = mouse.y - previousY
                previousX = mouse.x
                previousY = mouse.y

                if ((mouse.buttons & Qt.LeftButton) !== 0) {
                    // Scale pixel deltas to Euler degrees and clamp pitch before
                    // the camera can cross a pole and invert drag direction.
                    cameraRig.orbitYaw += deltaX * 0.24
                    cameraRig.orbitPitch = Math.max(
                        -78,
                        Math.min(78, cameraRig.orbitPitch - (deltaY * 0.24)))
                } else if ((mouse.buttons & Qt.RightButton) !== 0) {
                    // Right-drag pans in screen space; Y is inverted to match pointer motion.
                    cameraRig.panX += deltaX * 0.35
                    cameraRig.panY -= deltaY * 0.35
                }
            }
            onWheel: function(wheel) {
                // Convert wheel angle units to scene distance and enforce the same
                // near/far interaction limits used by pinch zoom.
                cameraRig.distance = Math.max(
                    260,
                    Math.min(900, cameraRig.distance - (wheel.angleDelta.y * 0.32)))
                wheel.accepted = true
            }
        }
    }
}
/*!
 * @file     : EarthWeatherMaterial.qml
 * \qmltype EarthWeatherMaterial
 * \inqmlmodule WeatherGlobe
 * \brief Defines the custom temperature-driven Earth material.
 *
 * The material requires immutable WeatherShaderParams and a current temperature
 * in degrees Celsius. It is intended for the globe sphere in GlobeView3D and
 * exposes day, night, and roughness texture inputs to the custom shader stages.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * All rights reserved.
 */

import QtQuick
import QtQuick3D
import WeatherGlobe

CustomMaterial {
    id: material

    // =========================================================
    // Public Shader Parameter Contract
    // =========================================================
    /*! \qmlproperty WeatherShaderParams EarthWeatherMaterial::shaderParams
     * Required, non-null source of immutable threshold and weight vectors. */
    required property WeatherShaderParams shaderParams
    /*! \qmlproperty real EarthWeatherMaterial::currentTemperature
     * Required current air temperature in degrees Celsius; no default. */
    required property real currentTemperature

    /*! \qmlproperty vector4d EarthWeatherMaterial::weatherThresholds
     * Ordered Celsius breakpoints; defaults to shaderParams.temperatureThresholds. */
    property vector4d weatherThresholds: shaderParams.temperatureThresholds
    /*! \qmlproperty vector4d EarthWeatherMaterial::weatherWeights
     * Dimensionless blend weights; defaults to shaderParams.temperatureWeights. */
    property vector4d weatherWeights: shaderParams.temperatureWeights

    // =========================================================
    // Visual Texture Inputs & Filtering
    // =========================================================
    /*! \qmlproperty TextureInput EarthWeatherMaterial::baseEarthTexture
     * Day-side albedo texture; defaults to the embedded earth-day image. */
    property TextureInput baseEarthTexture: TextureInput {
        texture: Texture {
            source: "assets/earth/earth-day.jpg"
            generateMipmaps: true
            minFilter: Texture.Linear
            mipFilter: Texture.Linear
            magFilter: Texture.Linear
        }
    }
    /*! \qmlproperty TextureInput EarthWeatherMaterial::nightEarthTexture
     * Night-side illumination texture; defaults to the embedded earth-night image. */
    property TextureInput nightEarthTexture: TextureInput {
        texture: Texture {
            source: "assets/earth/earth-night.png"
            generateMipmaps: true
            minFilter: Texture.Linear
            mipFilter: Texture.Linear
            magFilter: Texture.Linear
        }
    }
    /*! \qmlproperty TextureInput EarthWeatherMaterial::roughnessEarthTexture
     * Surface roughness texture; defaults to the embedded earth-roughness image. */
    property TextureInput roughnessEarthTexture: TextureInput {
        texture: Texture {
            source: "assets/earth/earth-roughness.jpg"
            generateMipmaps: true
            minFilter: Texture.Linear
            mipFilter: Texture.Linear
            magFilter: Texture.Linear
        }
    }

    // =========================================================
    // Shader Pipeline Configuration
    // =========================================================
    // Shaded mode integrates the custom stages with Qt Quick 3D lighting while
    // stable module-relative paths keep shader lookup independent of source layout.
    shadingMode: CustomMaterial.Shaded
    vertexShader: "shaders/earth_weather.vert"
    fragmentShader: "shaders/earth_weather.frag"
}
/**
 * @filename : earth_weather.frag
 * @brief    : Blends Earth textures with temperature-driven color effects.
 * @author   : Abhinay Chauhan (email: chauhan089306@gmail.com)
 * @version  : 1.0.0
 *
 * Copyright (c) 2024
 * Abhinay Chauhan. All rights reserved.
 */

VARYING vec2 weatherUv;
VARYING float absoluteLatitude;

void MAIN()
{
    const vec3 polarColor = vec3(0.42, 0.70, 1.0);
    const vec3 temperateColor = vec3(0.42, 0.84, 0.58);
    const vec3 tropicalColor = vec3(1.0, 0.54, 0.24);

    vec4 earthSample = texture(baseEarthTexture, weatherUv);
    vec3 nightSample = texture(nightEarthTexture, weatherUv).rgb;
    float roughnessSample = texture(roughnessEarthTexture, weatherUv).r;

    float coldBand = 1.0 - smoothstep(
        weatherThresholds.x,
        weatherThresholds.y,
        currentTemperature);
    float warmBand = smoothstep(
        weatherThresholds.z,
        weatherThresholds.w,
        currentTemperature);
    float temperateBand = clamp(1.0 - coldBand - warmBand, 0.0, 1.0);

    vec3 latitudeGradient = mix(tropicalColor, polarColor, absoluteLatitude);
    vec3 temperatureGradient = (polarColor * coldBand)
        + (temperateColor * temperateBand)
        + (tropicalColor * warmBand);
    float temperatureInfluence = clamp(
        weatherWeights.w
            + (weatherWeights.x * coldBand)
            + (weatherWeights.y * temperateBand)
            + (weatherWeights.z * warmBand),
        0.0,
        1.0);
    vec3 combinedGradient = mix(latitudeGradient, temperatureGradient, 0.55);

    BASE_COLOR = vec4(
        mix(earthSample.rgb, earthSample.rgb * combinedGradient, temperatureInfluence),
        earthSample.a);
    EMISSIVE_COLOR = nightSample * 0.26;
    METALNESS = 0.0;
    ROUGHNESS = mix(0.34, 0.86, roughnessSample);
    SPECULAR_AMOUNT = 0.42;
}
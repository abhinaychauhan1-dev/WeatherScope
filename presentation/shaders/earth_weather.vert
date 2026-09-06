/**
 * @filename : earth_weather.vert
 * @brief    : Computes Earth texture coordinates and latitude data.
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
    weatherUv = UV0;
    absoluteLatitude = abs((UV0.y * 2.0) - 1.0);
}
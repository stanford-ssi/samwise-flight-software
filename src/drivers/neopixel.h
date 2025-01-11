/*
Header file created 01/11/2025
Sets neopixel states on Picube
*/
#pragma once

void neopixel_init(int ledPin);
void neopixel_set_color_rgb(uint8_t r, uint8_t g, uint8_t b, int ledPin);
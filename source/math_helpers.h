#pragma once
#include <citro2d.h>
#include "color_channels.h"

#define RGBA8(r, g, b, a) (a | (b << (u32)8) | (g << (u32)16) | (r << (u32)24))

float clampf(float d, float min, float max);
float positive_fmodf(float n, float divisor);
Color color_lerp(Color color1, Color color2, float fraction);
void draw_9_slice(const C2D_Image atlas, const float x, const float y, const int width, const int height, const float border, const u32 color);
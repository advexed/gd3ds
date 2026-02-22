#include <math.h>
#include "math_helpers.h"
#include "color_channels.h"

float clampf(float d, float min, float max) {
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

float positive_fmodf(float n, float divisor) {
    float value = fmodf(n, divisor);
    return value + (value < 0 ? divisor : 0);
}

Color color_lerp(Color color1, Color color2, float fraction) {
    unsigned char r1 = color1.r;
    unsigned char r2 = color2.r;
    unsigned char g1 = color1.g;
    unsigned char g2 = color2.g;
    unsigned char b1 = color1.b;
    unsigned char b2 = color2.b;

    Color returned;
    returned.r = (r2 - r1) * fraction + r1;
    returned.g = (g2 - g1) * fraction + g1;
    returned.b = (b2 - b1) * fraction + b1;

    return returned;
}

Tex3DS_SubTexture select_box(const C2D_Image *image, int x, int y, int endX, int endY) {
    Tex3DS_SubTexture tex = *image->subtex;
    if (x != endX) {
        int deltaX  = endX - x;
        float texRL = tex.left - tex.right;
        tex.left    = tex.left - (float)texRL / tex.width * x;
        tex.right   = tex.left - (float)texRL / tex.width * deltaX;
        tex.width   = deltaX;
    }
    if (y != endY) {
        float texTB = tex.top - tex.bottom;
        int deltaY  = endY - y;
        tex.top     = tex.top - (float)texTB / tex.height * y;
        tex.bottom  = tex.top - (float)texTB / tex.height * deltaY;
        tex.height  = deltaY;
    }
    return tex;
}

// Helper macro to select tile from 3x3 grid
#define TILE(ix, iy) select_box(&atlas, ix*b, iy*b, ix*b + b, iy*b + b)

// Draw a 9 slice rectangle (useful for things like windows)
void draw_9_slice(const C2D_Image atlas, const float x, const float y, const int width, const int height, const float border, const u32 color) {
    // Set color
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, color, 1.f);


    float cx = x;
    float cy = y;

    float w = width;
    float h = height;

    float b = border;

    if (w < 2*b) w = 2*b;
    if (h < 2*b) h = 2*b;

    float halfW = w * 0.5f;
    float halfH = h * 0.5f;

    float midW = w - 2*b;
    float midH = h - 2*b;

    C2D_Sprite spr;

    Tex3DS_SubTexture sub;
    C2D_Image img;

    // Corners

    C2D_SpriteSetScale(&spr, 1.0f, 1.0f);

    sub = TILE(0,0);
    img = atlas; img.subtex = &sub;
    C2D_SpriteFromImage(&spr, img);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C2D_SpriteSetPos(&spr, cx - halfW + b*0.5f, cy - halfH + b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,0);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx + halfW - b*0.5f, cy - halfH + b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(0,2);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx - halfW + b*0.5f, cy + halfH - b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,2);
    img.subtex = &sub;
    C2D_SpriteSetPos(&spr, cx + halfW - b*0.5f, cy + halfH - b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    // Edges

    sub = TILE(1,0);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, midW / b, 1.0f);
    C2D_SpriteSetPos(&spr, cx, cy - halfH + b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(1,2);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, midW / b, 1.0f);
    C2D_SpriteSetPos(&spr, cx, cy + halfH - b*0.5f);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(0,1);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, 1.0f, midH / b);
    C2D_SpriteSetPos(&spr, cx - halfW + b*0.5f, cy);
    C2D_DrawSpriteTinted(&spr, &tint);

    sub = TILE(2,1);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, 1.0f, midH / b);
    C2D_SpriteSetPos(&spr, cx + halfW - b*0.5f, cy);
    C2D_DrawSpriteTinted(&spr, &tint);

    // Center

    sub = TILE(1,1);
    img.subtex = &sub;
    C2D_SpriteSetScale(&spr, midW / b, midH / b);
    C2D_SpriteSetPos(&spr, cx, cy);
    C2D_DrawSpriteTinted(&spr, &tint);
}

#undef TILE
#include <citro2d.h>
#include "color_channels.h"
#include "color.h"
#include "math_helpers.h"
#include <math.h>

Color p1_color;
Color p2_color;

ColorChannel channels[COL_CHANNEL_NUM];

int convert_one_point_nine_channel(int channel) {
    switch (channel) {
        case 1: return CHANNEL_P1;
        case 2: return CHANNEL_P2;
        case 3: return COL_1;
        case 4: return COL_2;
        case 5: return CHANNEL_LBG;
        case 6: return COL_3;
        case 7: return COL_4;
        case 8: return CHANNEL_3DL;
    }

    return channel;
}

void init_col_channels() {
    p1_color.r = 0;
    p1_color.g = 255;
    p1_color.b = 0;

    p2_color.r = 0;
    p2_color.g = 255;
    p2_color.b = 255;
    
    channels[0].color.r = 0;
    channels[0].color.g = 0;
    channels[0].color.b = 0;
    channels[0].blending = false;

    for (size_t chan = 1; chan < COL_CHANNEL_NUM; chan++) {
        channels[chan].color.r = 255;
        channels[chan].color.g = 255;
        channels[chan].color.b = 255;
        channels[chan].blending = false;
    }

    channels[CHANNEL_BG].color.r = 0;
    channels[CHANNEL_BG].color.g = 5;
    channels[CHANNEL_BG].color.b = 100;
    
    channels[CHANNEL_GROUND].color.r = 40;
    channels[CHANNEL_GROUND].color.g = 125;
    channels[CHANNEL_GROUND].color.b = 255;
       
    channels[CHANNEL_LINE].color.r = 255;
    channels[CHANNEL_LINE].color.g = 255;
    channels[CHANNEL_LINE].color.b = 255;
    channels[CHANNEL_LINE].blending = true;
    
    channels[CHANNEL_OBJ].color.r = 255;
    channels[CHANNEL_OBJ].color.g = 255;
    channels[CHANNEL_OBJ].color.b = 255;
    
    channels[CHANNEL_3DL].color.r = 255;
    channels[CHANNEL_3DL].color.g = 255;
    channels[CHANNEL_3DL].color.b = 255;
    
    channels[CHANNEL_P1].color = p1_color;
    channels[CHANNEL_P1].blending = true;
        
    channels[CHANNEL_P2].color = p2_color;
    channels[CHANNEL_P2].blending = true;
    
    channels[CHANNEL_LBG].color.r = 255;
    channels[CHANNEL_LBG].color.g = 255;
    channels[CHANNEL_LBG].color.b = 255;
    channels[CHANNEL_LBG].blending = true;
}

// https://github.com/gd-programming/gd.docs/issues/87
void calculate_lbg() {
    ColorChannel channel = channels[CHANNEL_BG];
    float h,s,v;
    
    convertRGBtoHSV(channel.color.r, channel.color.g, channel.color.b, &h, &s, &v);

    s -= 0.20f;
    s = clampf(s, 0.f, 1.f);
    v += 0.20f;
    v = clampf(v, 0.f, 1.f);

    unsigned char r,g,b;

    convertHSVtoRGB(h, s, v, &r, &g, &b);

    float factor = (channel.color.r + channel.color.g + channel.color.b) / 150.f;

    if (factor < 1.f) {
        r = r * factor + p1_color.r * (1 - factor);
        g = g * factor + p1_color.g * (1 - factor);
        b = b * factor + p1_color.b * (1 - factor);
    }

    // Set here lerped LBG
    channels[CHANNEL_LBG].color.r = r;
    channels[CHANNEL_LBG].color.g = g;
    channels[CHANNEL_LBG].color.b = b;
    channels[CHANNEL_LBG].blending = true;
}
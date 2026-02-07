#pragma once

enum ColorChannelIDs {
    NONE,
    COL_1,
    COL_2,
    COL_3,
    COL_4,
    CHANNEL_BG = 1000,
    CHANNEL_GROUND,
    CHANNEL_LINE,
    CHANNEL_3DL,
    CHANNEL_OBJ,
    CHANNEL_P1,
    CHANNEL_P2,
    CHANNEL_LBG,
    COL_CHANNEL_NUM,
};

typedef struct {
    unsigned char r,g,b;
} Color;

typedef struct {
    Color color;
    unsigned char blending;
} ColorChannel;

extern ColorChannel channels[COL_CHANNEL_NUM];

void calculate_lbg();
void init_col_channels();
int convert_one_point_nine_channel(int channel);
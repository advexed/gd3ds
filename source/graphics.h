#pragma once
#include <citro2d.h>
#include "level_loading.h"

#define MAX_SPRITES   8192
#define SCALE (SCREEN_HEIGHT / (11.f * 30))

// Simple sprite struct
typedef struct
{
	C2D_Sprite spr;
	float dx, dy; // velocity
} Sprite;

typedef struct
{
	C2D_Sprite spr;
	Object *obj;
	int layer;
	int col_type;
	float opacity;
	int col_channel;
} SpriteObject;

typedef struct {
    SpriteObject *obj;
    uint32_t key;
} SortItem;

extern int sprite_count;
extern C2D_SpriteSheet spriteSheet;
extern C2D_SpriteSheet spriteSheet2;
extern C2D_SpriteSheet bgSheet;
extern C2D_Sprite bg;

void draw_objects();
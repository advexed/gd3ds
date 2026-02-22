#pragma once
#include "ui_element.h"

void ui_assets_init();
void ui_image_set_tint(UIElement* e, u32 color);
void ui_image_clear_tint(UIElement* e);
UIElement ui_create_image(int x, int y, int sprite_index, float sx, float sy, char (*tag)[TAG_LENGTH]);
extern C2D_SpriteSheet ui_sheet;
extern C2D_SpriteSheet bigFont_sheet;
extern C2D_SpriteSheet window_sheet;
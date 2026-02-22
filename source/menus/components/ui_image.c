#include "ui_element.h"
#include <citro2d.h>
#include "ui_screen.h"

C2D_SpriteSheet ui_sheet;
C2D_SpriteSheet window_sheet;
C2D_SpriteSheet bigFont_sheet;

void ui_assets_init() {
    ui_sheet = C2D_SpriteSheetLoad("romfs:/gfx/ui.t3x");
    window_sheet = C2D_SpriteSheetLoad("romfs:/gfx/windows.t3x");
    bigFont_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bigFont.t3x");
}

static void ui_image_update(UIElement* e, UIInput* touch) {
    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);
    
    // Mask background elements
    if (inside) touch->did_something = true;
}

static void ui_image_draw(UIElement* e) {
    C2D_SpriteSetCenter(&e->image.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->image.sprite, e->x, e->y);
    C2D_SpriteSetScale(&e->image.sprite, e->image.scaleX, e->image.scaleY);
    if (e->image.useTint) {
        C2D_DrawSpriteTinted(&e->image.sprite, &e->image.tint);
    } else {
        C2D_DrawSprite(&e->image.sprite);
    }
}

void ui_image_set_tint(UIElement* e, u32 color) {
    if (e->type != UI_IMAGE) return;

    C2D_PlainImageTint(&e->image.tint, color, 0.0f);
    e->image.useTint = true;
}

void ui_image_clear_tint(UIElement* e) {
    if (e->type != UI_IMAGE) return;
    
    e->image.useTint = false;
}

UIElement ui_create_image(int x, int y, int sprite_index, float sx, float sy, char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_IMAGE;
    e.x = x;
    e.y = y;
    e.enabled = false;
    e.image.useTint = false;

    C2D_SpriteFromSheet(&e.image.sprite, ui_sheet, sprite_index);

    // Copy tag
    copy_tag_array(&e, tag);

    e.w = e.image.sprite.params.pos.w * sx;
    e.h = e.image.sprite.params.pos.h * sy;

    e.image.scaleX = sx;
    e.image.scaleY = sy;

    e.update = ui_image_update;
    e.draw = ui_image_draw;

    return e;
}
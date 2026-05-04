#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "ui_window_button.h"
#include "easing.h"
#include "math_helpers.h"
#include "ui_screen.h"

#include "main.h"

typedef struct {
    const Charset *charset;
    C2D_SpriteSheet *sheet;    
} WindowButtonFont;

static WindowButtonFont fonts[] = {
    {
        .charset = &bigFont_fontCharset,
        .sheet = &bigFont_sheet
    },
    {
        .charset = &chatFont_fontCharset,
        .sheet = &chatFont_sheet
    },
    {
        .charset = &goldFont_fontCharset,
        .sheet = &goldFont_sheet
    }
};

#define NUM_FONTS (sizeof(fonts) / sizeof(WindowButtonFont))

static void ui_window_button_update(UIElement* e, UIInput* touch) {
    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);

    // Check if pressed the button
    if (inside && pressedTouch && !touch->did_something) {
        e->window_button.hovered = true;
        e->window_button.pressed = true;
    }

    // If previously window_button on it, hover
    if (inside && e->button.pressed) {
        e->window_button.hovered = true;
    }
    
    EaseTypes bounce_type;
    // Animation
    if (e->window_button.hovered) {
        e->window_button.hoverTimer += DT;
        bounce_type = BOUNCE_OUT;
    } else {
        e->window_button.hoverTimer -= DT;
        // As the animation plays in reverse, we just use bounce in
        bounce_type = BOUNCE_IN;
    }

    e->window_button.hoverTimer = clampf(e->window_button.hoverTimer, 0.f, WINDOW_BUTTON_HOVER_ANIM_TIME);
    e->window_button.hoverScale = easeValue(bounce_type, 1.0f, WINDOW_BUTTON_HOVER_SCALE, e->window_button.hoverTimer, WINDOW_BUTTON_HOVER_ANIM_TIME, 0);


    // If released on button, do its action
    if (e->window_button.hovered && releasedTouch) {
        e->window_button.pressed = false;
        e->window_button.hovered = false;
        e->window_button.hoverTimer = 0.f;
        e->window_button.hoverScale = 1.f;
        if (e->action)
            e->action(e);
    }
    
    // Unpress the button
    if (!inside) {
        e->window_button.hovered = false;
    }
    
    // Mask background elements
    if (inside) {
        touch->interacted = true;
        touch->did_something = true;
    }
}

static void ui_window_button_draw(UIElement* e) {
    int font_id = e->window_button.font;

    // Set to pusab if invalid
    if (font_id >= NUM_FONTS) font_id = 0;

    WindowButtonFont *font = &fonts[font_id];

    float scale = e->window_button.hoverScale;
    float text_scale;

    draw_9_slice(e->window_button.window.atlas, e->x, e->y, e->w * scale, e->h * scale, e->window_button.window.border, e->window_button.window.color);

    if (e->window_button.textScale == 0){
        // Get text length in pixels
        float length = get_text_length(font->charset, 1 / 0.85f, e->window_button.text);
    
        if (e->w < length) {
            text_scale = scale * (e->w / length);
        } else {
            text_scale = scale * 0.85f;
        }
    } else {
        text_scale = (e->window_button.textScale * scale);
    }
    

    draw_text(font->charset, font->sheet, e->x, e->y, text_scale, 0.5f, "%s", e->window_button.text);
}

void ui_window_button_set_style(UIElement *e, int style) {
    if (e->type != UI_WINDOW_BUTTON) return;

    e->window_button.window.atlas = C2D_SpriteSheetGetImage(window_sheet, style);

    e->window_button.window.border = e->window_button.window.atlas.subtex->width / 3;
}

UIElement ui_create_window_button(
    int x, int y, float w, float h, int style,
    UIActionFn action,
    char *text,
    int font,
    char (*tag)[TAG_LENGTH],
    float textScale
){
    UIElement e = {
        .type = UI_WINDOW_BUTTON,
        .x = x, .y = y,
        .w = w, .h = h,
        .enabled = true,
        .action = action,
        .update = ui_window_button_update,
        .draw = ui_window_button_draw
    };

    // Copy tag
    copy_tag_array(&e, tag);

    // Copy text
    strncpy(e.window_button.text, text, 63);

    e.window_button.window.color = C2D_Color32(255, 255, 255, 255);

    ui_window_button_set_style(&e, style);
    
    e.window_button.hoverScale = 1.f;

    e.window_button.font = font;
    e.window_button.textScale = textScale;

    return e;
}
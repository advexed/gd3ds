#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include "text.h"
#include "fonts/bigFont.h"
#include <stdarg.h>

const Glyph *get_glyph(Charset font, char character) {
    for (int i = 0; i < font.count; i++) {
        if (character == font.glyphs[i].id) {
            return &font.glyphs[i];
        }
    }
    // If not found and a lowercase letter, convert to uppercase
    if (character >= 'a' && character <= 'z') {
        character -= 32;

        // Search again
        for (int i = 0; i < font.count; i++) {
            if (character == font.glyphs[i].id) {
                return &font.glyphs[i];
            }
        }
    }
    
    return NULL;
}

float get_text_length(Charset font, const float zoom_x, const char *text, ...) {
    
    char tmp[1024];

    va_list argp;
    va_start(argp, text);
    const int size = vsnprintf(tmp, sizeof(tmp), text, argp);
    va_end(argp);
    
    float text_length = 0;
    for (int i = 0; i < size; i++) {
        const Glyph *character = get_glyph(font, tmp[i]);

        if (character != NULL) {
            float xadvance = character->xAdvance * zoom_x;

            text_length += xadvance;
        }
    }
    return text_length;
}

void draw_text(Charset font, C2D_SpriteSheet sheet, const float x, const float y, const float scale, float alignment, const char *text, ...) {
    if (!text || !sheet) {
        return;
    }

    char tmp[1024];

    va_list argp;
    va_start(argp, text);
    const int size = vsnprintf(tmp, sizeof(tmp), text, argp);
    va_end(argp);
    
    float length = get_text_length(font, fabsf(scale), tmp);
    
    float offset = 0;

    for (int i = 0; i < size; i++) {
        const Glyph *character = get_glyph(font, tmp[i]);
        
        if (character != NULL) {
            C2D_Sprite sprite = { 0 };

            int width = character->width;
            int height = character->height;
            float xoffset = (character->xOffset + (width / 2.f)) * scale;
            float yoffset = (character->yOffset + (height / 2.f)) * scale;
            float xadvance = character->xAdvance * scale;

            
            int index = character->spriteIndex;

            float final_x = x + offset + xoffset - length * alignment;
            float final_y = y + yoffset - 19 * scale;

            if (index >= 0) { 
                // Draw glyph so its center is at (final_x, final_y)
                C2D_SpriteFromSheet(&sprite, sheet, index);
                C2D_SpriteSetCenter(&sprite, 0.5f, 0.5f);
                C2D_SpriteSetPos(&sprite, roundf(final_x), roundf(final_y));
                C2D_SpriteSetScale(&sprite, scale, scale);
                C2D_DrawSprite(&sprite);
            }

            offset += xadvance;
        }
    }
}
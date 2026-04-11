#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include "text.h"
#include "fonts/bigFont.h"
#include <stdarg.h>

const Glyph *get_glyph(const Charset *font, char character) {
    for (int i = 0; i < font->count; i++) {
        if (character == font->glyphs[i].id) {
            return &font->glyphs[i];
        }
    }
    // If not found and a lowercase letter, convert to uppercase
    if (character >= 'a' && character <= 'z') {
        character -= 32;

        // Search again
        for (int i = 0; i < font->count; i++) {
            if (character == font->glyphs[i].id) {
                return &font->glyphs[i];
            }
        }
    }
    
    return NULL;
}

float get_text_length(const Charset *font, const float zoom_x, const char *text) {
    float text_length = 0;
    int size = strlen(text);
    for (int i = 0; i < size; i++) {
        const Glyph *character = get_glyph(font, text[i]);

        if (character != NULL) {
            float xadvance = character->xAdvance * zoom_x;

            text_length += xadvance;
        }
    }
    return text_length;
}

void draw_text(const Charset *font, C2D_SpriteSheet *sheet, const float x, const float y, const float scale, float alignment, const char *text, ...) {
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

            float xoffset = (character->xOffset) * scale;
            float yoffset = (character->yOffset) * scale;
            float xadvance = character->xAdvance * scale;

            int index = character->spriteIndex;

            float final_x = x + offset + xoffset - length * alignment;
            float final_y = y + yoffset - (int)(19 * scale);

            final_x = (scale == 1.0f) ? roundf(final_x) : final_x;
            final_y = (scale == 1.0f) ? roundf(final_y) : final_y;

            if (index >= 0) { 
                // Draw glyph so its center is at (final_x, final_y)
                C2D_SpriteFromSheet(&sprite, *sheet, index);
                C3D_TexSetFilter(sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
                C2D_SpriteSetPos(&sprite, final_x, final_y);
                C2D_SpriteSetScale(&sprite, scale, scale);
                C2D_DrawSprite(&sprite);
            }

            offset += xadvance;
        }
    }
}
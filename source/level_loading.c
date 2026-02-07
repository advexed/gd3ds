
#include <stdlib.h>
#include <stdbool.h>
#include "level_loading.h"
#include "defines.h"
#include <zlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "color_channels.h"
#include "objects.h"

Object *objectArray = NULL;

Section *section_hash[SECTION_HASH_SIZE] = {0};

static inline unsigned int section_hash_func(unsigned int x) {
    x ^= x >> 16;
    x *= 0x7feb352d;   // good mixing constant
    return (uint32_t)((uint64_t)x * SECTION_HASH_SIZE >> 32);
}

Section *get_or_create_section(int x) {
    unsigned int h = section_hash_func(x);
    Section *sec = section_hash[h];
    while (sec) {
        if (sec->x == x) return sec;
        sec = sec->next;
    }
    sec = malloc(sizeof(Section));
    sec->objects = malloc(sizeof(Object*) * 8);
    sec->object_count = 0;
    sec->object_capacity = 8;
    sec->x = x;
    sec->next = section_hash[h];
    section_hash[h] = sec;
    return sec;
}

void free_sections(void) {
    for (int i = 0; i < SECTION_HASH_SIZE; i++) {
        Section *sec = section_hash[i];
        while (sec) {
            Section *next = sec->next;
            free(sec->objects);
            free(sec);
            sec = next;
        }
        section_hash[i] = NULL;
    }
}

void assign_object_to_section(Object *obj) {
    int sx = (int)(obj->x / SECTION_SIZE);
    Section *sec = get_or_create_section(sx);
    if (sec->object_count >= sec->object_capacity) {
        sec->object_capacity *= 2;
        sec->objects = realloc(sec->objects, sizeof(Object*) * sec->object_capacity);
    }
    sec->objects[sec->object_count++] = obj;
}

char *read_file(const char *filepath, size_t *out_size) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        printf("Failed to open file: %s\n", filepath);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        printf("Failed to allocate file\n");
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, size, f);
    buffer[size] = '\0'; // Null-terminate for text files
    fclose(f);

    if (out_size) *out_size = size;
    return buffer;
}

char *extract_gmd_key(const char *data, const char *key, const char *type) {
    char key_tag[32];
    snprintf(key_tag, sizeof(key_tag), "<k>%s</k>", key);
    
    char *key_pos = strstr(data, key_tag);
    if (!key_pos) {
        return NULL;
    }

    // Move past the key tag
    char *start = key_pos + strlen(key_tag);

    // Skip whitespace (spaces, tabs, newlines, etc.)
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    char type_start_tag[16];
    snprintf(type_start_tag, sizeof(type_start_tag), "<%s>", type);

    // Confirm that the type start tag is here
    if (strncmp(start, type_start_tag, strlen(type_start_tag)) != 0) {
        printf("Expected start tag '%s' not found after key\n", type_start_tag);
        return NULL;
    }

    // Move past the type start tag
    start += strlen(type_start_tag);

    // Find the end tag
    char type_end_tag[16];
    snprintf(type_end_tag, sizeof(type_end_tag), "</%s>", type);
    char *end = strstr(start, type_end_tag);
    if (!end) {
        printf("Could not find end tag '%s'\n", type_end_tag);
        return NULL;
    }

    // Allocate and copy value
    int len = end - start;
    char *value = malloc(len + 1);
    if (!value) {
        printf("malloc for gmd key %s failed\n", key);
        return NULL;
    }
    strncpy(value, start, len);
    value[len] = '\0';
    return value;
}

bool is_ascii(const unsigned char *data, int len) {
    for (int i = 0; i < len; i++) {
        if (data[i] > 0x7F) {
            return FALSE; // Non-ASCII byte found
        }
    }
    return TRUE;
}

int b64_char(char c) {
    if ('A' <= c && c <= 'Z') return c - 'A';
    if ('a' <= c && c <= 'z') return c - 'a' + 26;
    if ('0' <= c && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

void fix_base64_url(char *b64) {
    for (int i = 0; b64[i]; i++) {
        if (b64[i] == '-') b64[i] = '+';
        else if (b64[i] == '_') b64[i] = '/';
    }
}

int base64_decode(const char *in, unsigned char *out) {
    int len = 0;
    for (int i = 0; in[i] && in[i+1] && in[i+2] && in[i+3]; i += 4) {
        int a = b64_char(in[i]);
        int b = b64_char(in[i+1]);
        int c = in[i+2] == '=' ? 0 : b64_char(in[i+2]);
        int d = in[i+3] == '=' ? 0 : b64_char(in[i+3]);

        if (a == -1 || b == -1 || c == -1 || d == -1) {
            printf("Invalid base64 character at position %d\n", i);
            return -1;
        }

        out[len++] = (a << 2) | (b >> 4);
        if (in[i+2] != '=') out[len++] = (b << 4) | (c >> 2);
        if (in[i+3] != '=') out[len++] = (c << 6) | d;
    }
    return len;
}

uLongf get_uncompressed_size(unsigned char *data, int data_len) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.next_in = data;
    strm.avail_in = data_len;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {  // auto-detect gzip/zlib
        return 0;
    }

    uLongf total_out = 0;
    unsigned char buf[4096];

    do {
        strm.next_out = buf;
        strm.avail_out = sizeof(buf);
        int ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return 0;
        }
        total_out += sizeof(buf) - strm.avail_out;
        if (ret == Z_STREAM_END) break;
    } while (strm.avail_in > 0);

    inflateEnd(&strm);
    return total_out;
}


char *decompress_data(unsigned char *data, int data_len, uLongf *out_len) {
    uLongf final_size = get_uncompressed_size(data, data_len);
    printf("Decompressing to a final size of %lu bytes...\n", (unsigned long)final_size);

    z_stream strm = {0};
    strm.next_in = data;
    strm.avail_in = data_len;

    if (inflateInit2(&strm, 15 | 32) != Z_OK) {   // auto-detect gzip/zlib
        printf("Failed to initialize zlib stream for GZIP\n");
        return NULL;
    }

    // Allocate exactly enough memory (+1 for null terminator if needed)
    char *out = malloc(final_size + 1);
    if (!out) {
        printf("malloc failed for %lu bytes\n", (unsigned long)final_size);
        inflateEnd(&strm);
        return NULL;
    }

    strm.next_out = (Bytef *)out;
    strm.avail_out = final_size;

    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        printf("inflate failed with code %d\n", ret);
        free(out);
        inflateEnd(&strm);
        return NULL;
    }

    *out_len = strm.total_out;
    out[*out_len] = '\0'; // Null-terminate if treating as string

    inflateEnd(&strm);

    printf("Decompressed %lu bytes successfully\n", (unsigned long)*out_len);
    return out;
}

char *get_metadata_value(const char *levelString, const char *key) {
    if (!levelString || !key) return NULL;

    // Find the first semicolon, which separates metadata from objects
    const char *end = strchr(levelString, ';');
    if (!end) return NULL;

    // We'll scan only the metadata portion
    size_t metadataLen = end - levelString;
    char *metadata = malloc(metadataLen + 1);
    if (!metadata) return NULL;

    strncpy(metadata, levelString, metadataLen);
    metadata[metadataLen] = '\0';

    // Tokenize metadata by comma
    char *token = strtok(metadata, ",");
    while (token) {
        if (strcmp(token, key) == 0) {
            char *value = strtok(NULL, ",");  // get value after key
            if (!value) break;

            // Copy and return value
            char *result = strdup(value);
            free(metadata);
            return result;
        }
        token = strtok(NULL, ",");
    }
    
    free(metadata);
    return NULL;
}

char *decompress_level(char *data) {
    printf("Loading level data...\n");

    char *b64 = extract_gmd_key((const char *) data, "k4", "s");
    if (!b64) {
        // Empty level
        return data;
    }

    fix_base64_url(b64);

    unsigned char *decoded = malloc(strlen(b64));
    int decoded_len = base64_decode(b64, decoded);
    if (decoded_len <= 0) {
        printf("Failed to decode base64\n");
        free(b64);
        free(decoded);
        return NULL;
    }

    uLongf decompressed_len;
    char *decompressed = decompress_data(decoded, decoded_len, &decompressed_len);
    if (!decompressed) {
        printf("Decompression failed (check zlib error above)\n");
        free(decoded);
        free(b64);
        return NULL;
    }

    free(decoded);
    free(b64);
    
    return decompressed;
}

char **split_string(const char *str, char delimiter, int *outCount) {
    char **result = NULL;
    int count = 0;
    const char *start = str;
    const char *ptr = str;

    while (*ptr) {
        if (*ptr == delimiter) {
            int len = ptr - start;
            if (len > 0) {
                char *token = (char *)malloc(len + 1);
                strncpy(token, start, len);
                token[len] = '\0';

                result = (char **)realloc(result, sizeof(char*) * (count + 1));
                result[count++] = token;
            }
            start = ptr + 1;
        }
        ptr++;
    }
    if (ptr > start) {
        int len = ptr - start;
        char *token = (char *)malloc(len + 1);
        strncpy(token, start, len);
        token[len] = '\0';
        result = (char **)realloc(result, sizeof(char*) * (count + 1));
        result[count++] = token;
    }

    *outCount = count;
    return result;
}

void free_string_array(char **arr, int count) {
    for (int i = 0; i < count; i++) free(arr[i]);
    free(arr);
}

bool parse_bool(const char *str) {
    return (str[0] == '1' && str[1] == '\0');
}

GDValueType get_value_type_for_key(int key) {
    switch (key) {
        case 1:  return GD_VAL_INT;    // Object ID
        case 2:  return GD_VAL_FLOAT;  // X Position
        case 3:  return GD_VAL_FLOAT;  // Y Position
        case 4:  return GD_VAL_BOOL;   // Flipped Horizontally
        case 5:  return GD_VAL_BOOL;   // Flipped Vertically
        case 6:  return GD_VAL_FLOAT;  // Rotation
//        case 7:  return GD_VAL_INT;    // (Color/Pulse trigger) Red
//        case 8:  return GD_VAL_INT;    // (Color/Pulse trigger) Green
//        case 9:  return GD_VAL_INT;    // (Color/Pulse trigger) Blue
//        case 10: return GD_VAL_FLOAT;  // (Color trigger) Duration
//        case 11: return GD_VAL_BOOL;   // (Triggers) Touch Triggered
//        case 14: return GD_VAL_BOOL;   // (Color trigger) Tint ground
//        case 15: return GD_VAL_BOOL;   // (Color trigger) Player 1 color
//        case 16: return GD_VAL_BOOL;   // (Color trigger) Player 2 color
//        case 17: return GD_VAL_BOOL;   // (Color trigger) Blending
        case 19: return GD_VAL_INT;    // 1.9 color channel
        case 21: return GD_VAL_INT;    // Main col channel
        case 22: return GD_VAL_INT;    // Detail col channel
//        case 23: return GD_VAL_INT;    // (Color trigger) Target color ID
        case 24: return GD_VAL_INT;    // Zlayer
        case 25: return GD_VAL_INT;    // Zorder
//        case 28: return GD_VAL_INT;    // (Move trigger) Offset X
//        case 29: return GD_VAL_INT;    // (Move trigger) Offset Y
//        case 30: return GD_VAL_INT;    // (Various) Easing
//        case 31: return GD_VAL_STRING; // (Text obj) Text
//        case 32: return GD_VAL_FLOAT;  // Scale
//        case 35: return GD_VAL_FLOAT;  // (Color trigger) Opacity
//        case 41: return GD_VAL_BOOL;   // Main col HSV enabled
//        case 42: return GD_VAL_BOOL;   // Detail col HSV enabled
//        case 43: return GD_VAL_HSV;    // Main col HSV
//        case 44: return GD_VAL_HSV;    // Detail col HSV
//        case 45: return GD_VAL_FLOAT;  // (Pulse trigger) Fade in
//        case 46: return GD_VAL_FLOAT;  // (Pulse trigger) Hold
//        case 47: return GD_VAL_FLOAT;  // (Pulse trigger) Fade out
//        case 48: return GD_VAL_INT;    // (Pulse trigger) Pulse mode
//        case 49: return GD_VAL_HSV;    // (Color/Pulse trigger) Copy color HSV
//        case 50: return GD_VAL_INT;    // (Color/Pulse trigger) Copy color id
//        case 51: return GD_VAL_INT;    // (Triggers) Target group id
//        case 52: return GD_VAL_INT;    // (Pulse trigger) Pulse target type
//        case 54: return GD_VAL_FLOAT;  // (Teleport portal) Y offset
//        case 56: return GD_VAL_BOOL;   // (Toggle trigger) Activate trigger
//        case 57: return GD_VAL_INT_ARRAY; // Groups
//        case 58: return GD_VAL_BOOL;   // (Move trigger) Lock to player x
//        case 59: return GD_VAL_BOOL;   // (Move trigger) Lock to player y
//        case 62: return GD_VAL_BOOL;   // (Triggers) Spawn triggered
//        case 63: return GD_VAL_FLOAT;  // (Spawn trigger) Spawn delay
//        case 64: return GD_VAL_BOOL;   // Don't fade
//        case 65: return GD_VAL_BOOL;   // (Pulse trigger) Main only
//        case 66: return GD_VAL_BOOL;   // (Pulse trigger) Detail only
//        case 67: return GD_VAL_BOOL;   // Don't enter
//        case 87: return GD_VAL_BOOL;   // (Triggers) Multi triggered
//        case 128: return GD_VAL_FLOAT; // Scale x
//        case 129: return GD_VAL_FLOAT; // Scale y
        default:
            return GD_VAL_INT; // Default fallback
    }
}

void fill_object_data(Object *object, int key, GDValueType type, GDValue val) {
    // Default members
    switch (key) {
        case 1:  // ID
            if (type == GD_VAL_INT) object->id = val.i;
            break;
        case 2:  // X
            if (type == GD_VAL_FLOAT) object->x = val.f;
            break;
        case 3:  // Y
            if (type == GD_VAL_FLOAT) object->y = val.f;
            break;
        case 4:  // FlippedH
            if (type == GD_VAL_BOOL) object->flippedH = val.b;
            break;
        case 5:  // FlippedV
            if (type == GD_VAL_BOOL) object->flippedV = val.b;
            break;
        case 6:  // Rotation
            if (type == GD_VAL_FLOAT) object->rotation = val.f;
            break;
        case 19: // 1.9 channel id
            if (type == GD_VAL_INT) object->detail_col_channel = convert_one_point_nine_channel(val.i);
            break;
        case 21: // Main col channel
            if (type == GD_VAL_INT) object->col_channel = val.i;
            break;
        case 22: // Detail col channel
            if (type == GD_VAL_INT) object->detail_col_channel = val.i;
            break;
        case 24: // Z layer
            if (type == GD_VAL_INT) object->zlayer = val.i;
            break;
        case 25: // Z order
            if (type == GD_VAL_INT) object->zorder = val.i;
            break;
//        case 32: // Scale
//            if (type == GD_VAL_FLOAT) object->scale_x = object->scale_y = val.f;
//            break;
//        case 57: // Groups
//            if (type == GD_VAL_INT_ARRAY) {
//                for (int i = 0; i < MAX_GROUPS_PER_OBJECT; i++) {
//                    object->groups[i] = val.int_array[i];
//                }
//            }
//            break;
//        
//        case 128: // Scale x
//            if (type == GD_VAL_FLOAT) object->scale_x = val.f;
//            break;
//        case 129: // Scale y
//            if (type == GD_VAL_FLOAT) object->scale_y = val.f;
//            break;
    }

//        // Col trigger members
//        if (*soa_type(object) == TYPE_NORMAL_OBJECT) {
//            switch (key) {
//                case 21: // Main col channel
//                    if (type == GD_VAL_INT) object->object.main_col_channel = val.i;
//                    break;
//                case 22: // Detail col channel
//                    if (type == GD_VAL_INT) object->object.detail_col_channel = val.i;
//                    break;
//                case 31: // Text
//                    if (type == GD_VAL_STRING) object->object.text = val.str;
//                    break;
//                case 41: // Main col HSV enabled
//                    if (type == GD_VAL_BOOL) object->object.main_col_HSV_enabled = val.b;
//                    break;
//                case 42: // Detail col HSV enabled
//                    if (type == GD_VAL_BOOL) object->object.detail_col_HSV_enabled = val.b;
//                    break;
//                case 43: // Main col HSV
//                    if (type == GD_VAL_HSV) object->object.main_col_HSV = val.hsv;
//                    break;
//                case 44: // Detail col HSV
//                    if (type == GD_VAL_HSV) object->object.detail_col_HSV = val.hsv;
//                    break;
//                case 54: // Teleport portal y offset
//                    if (type == GD_VAL_FLOAT) object->object.orange_tp_portal_y_offset = val.f;
//                    break;
//                case 64: // Don't fade
//                    if (type == GD_VAL_BOOL) object->object.dont_fade = val.b;
//                    break;
//                case 67: // Don't enter
//                    if (type == GD_VAL_BOOL) object->object.dont_enter = val.b;
//                    break;
//            }
//        } else {
//            if (key == 10) { // Duration
//                if (type == GD_VAL_FLOAT) object->trigger.trig_duration = val.f;
//            } else if (key == 11) { // Touch triggered
//                if (type == GD_VAL_BOOL) object->trigger.touch_triggered = val.b;
//            } else if (key == 62) { // Spawn triggered
//                if (type == GD_VAL_BOOL) object->trigger.spawn_triggered = val.b;
//            } else if (key == 87) { // Multi triggered
//                if (type == GD_VAL_BOOL) object->trigger.multi_triggered = val.b;
//            }
//            switch (*soa_type(object)) {
//                case TYPE_COL_TRIGGER:
//                    switch (key) {
//                        case 7:  // Color R
//                            if (type == GD_VAL_INT) object->trigger.col_trigger.trig_colorR = val.i;
//                            break;
//                        case 8:  // Color G
//                            if (type == GD_VAL_INT) object->trigger.col_trigger.trig_colorG = val.i;
//                            break;
//                        case 9:  // Color B
//                            if (type == GD_VAL_INT) object->trigger.col_trigger.trig_colorB = val.i;
//                            break;
//                        case 14: // Tint Ground
//                            if (type == GD_VAL_BOOL) object->trigger.col_trigger.tintGround = val.b;
//                            break;
//                        case 15: // Player 1 color
//                            if (type == GD_VAL_BOOL) object->trigger.col_trigger.p1_color = val.b;
//                            break;
//                        case 16: // Player 2 color
//                            if (type == GD_VAL_BOOL) object->trigger.col_trigger.p2_color = val.b;
//                            break;
//                        case 17: // Blending
//                            if (type == GD_VAL_BOOL) object->trigger.col_trigger.blending = val.b;
//                            break;
//                        case 23: // Target color ID
//                            if (type == GD_VAL_INT) object->trigger.col_trigger.target_color_id = val.i;
//                            break;
//                        case 35: // Opacity
//                            if (type == GD_VAL_FLOAT) object->trigger.col_trigger.opacity = val.f;
//                            break;
//                        case 49: // Copy color HSV
//                            if (type == GD_VAL_HSV) object->trigger.col_trigger.copied_hsv = val.hsv;
//                            break;
//                        case 50: // Copy color ID
//                            if (type == GD_VAL_INT) object->trigger.col_trigger.copied_color_id = val.i;
//                            break;
//                    }
//                    break;
//                case TYPE_ALPHA_TRIGGER:
//                    switch (key) {
//                        case 35: // Opacity
//                            if (type == GD_VAL_FLOAT) object->trigger.alpha_trigger.opacity = val.f;
//                            break;
//                        case 51: // Target group id
//                            if (type == GD_VAL_INT) object->trigger.alpha_trigger.target_group = val.i;
//                            break;
//                    }
//                    break;
//                case TYPE_TOGGLE_TRIGGER:
//                    switch (key) {
//                        case 51: // Target group id
//                            if (type == GD_VAL_INT) object->trigger.toggle_trigger.target_group = val.i;
//                            break;
//                        case 56: // Toggle mode
//                            if (type == GD_VAL_BOOL) object->trigger.toggle_trigger.activate_group = val.b;
//                            break;
//                    }
//                    break;
//                case TYPE_SPAWN_TRIGGER:
//                    switch (key) {
//                        case 51: // Target group id
//                            if (type == GD_VAL_INT) object->trigger.spawn_trigger.target_group = val.i;
//                            break;
//                        case 63: // Spawn delay
//                            if (type == GD_VAL_FLOAT) object->trigger.spawn_trigger.spawn_delay = val.f;
//                            break;
//                    }
//                    break;
//                case TYPE_MOVE_TRIGGER:
//                    switch (key) {
//                        case 28:  // Offset X
//                            if (type == GD_VAL_INT) object->trigger.move_trigger.offsetX = val.i;
//                            break;
//                        case 29:  // Offset Y
//                            if (type == GD_VAL_INT) object->trigger.move_trigger.offsetY = val.i;
//                            break;
//                        case 30:  // Easing
//                            if (type == GD_VAL_INT) object->trigger.move_trigger.easing = val.i;
//                            break;
//                        case 51: // Target group id
//                            if (type == GD_VAL_INT) object->trigger.move_trigger.target_group = val.i;
//                            break;
//                        case 58: // Lock to player x
//                            if (type == GD_VAL_BOOL) object->trigger.move_trigger.lock_to_player_x = val.b;
//                            break;
//                        case 59: // Lock to player y
//                            if (type == GD_VAL_BOOL) object->trigger.move_trigger.lock_to_player_y = val.b;
//                            break;
//                    }
//                    break;
//                case TYPE_PULSE_TRIGGER:
//                    switch (key) {
//                        case 7:  // Color R
//                            if (type == GD_VAL_INT) object->trigger.pulse_trigger.color.r = val.i;
//                            break;
//                        case 8:  // Color G
//                            if (type == GD_VAL_INT) object->trigger.pulse_trigger.color.g = val.i;
//                            break;
//                        case 9:  // Color B
//                            if (type == GD_VAL_INT) object->trigger.pulse_trigger.color.b = val.i;
//                            break;
//                        case 45: // Fade in
//                            if (type == GD_VAL_FLOAT) object->trigger.pulse_trigger.fade_in = val.f;
//                            break;
//                        case 46: // Hold
//                            if (type == GD_VAL_FLOAT) object->trigger.pulse_trigger.hold = val.f;
//                            break;
//                        case 47: // Fade out
//                            if (type == GD_VAL_FLOAT) object->trigger.pulse_trigger.fade_out = val.f;
//                            break;
//                        case 48: // Pulse mode
//                            if (type == GD_VAL_INT) object->trigger.pulse_trigger.pulse_mode = val.i;
//                            break;
//                        case 49: // Copy color HSV
//                            if (type == GD_VAL_HSV) object->trigger.pulse_trigger.copied_hsv = val.hsv;
//                            break;
//                        case 50: // Copy color ID
//                            if (type == GD_VAL_INT) object->trigger.pulse_trigger.copied_color_id = val.i;
//                            break;
//                        case 51: // Target group id
//                            if (type == GD_VAL_INT) object->trigger.pulse_trigger.target_group = val.i;
//                            break;
//                        case 52: // Pulse target type
//                            if (type == GD_VAL_INT) object->trigger.pulse_trigger.pulse_target_type = val.i;
//                            break;
//                        case 65: // Main only
//                            if (type == GD_VAL_BOOL) object->trigger.pulse_trigger.main_only = val.b;
//                            break;
//                        case 66: // Detail only
//                            if (type == GD_VAL_BOOL) object->trigger.pulse_trigger.detail_only = val.b;
//                            break;
//                    }
//                default:
//                    break;
//            }
//        }
//    }
    
    // Modify level ending pos
//    if (*soa_x(object) > level_info.last_obj_x) {
//        level_info.last_obj_x = *soa_x(object);
//    }

//    if (*soa_type(object) == TYPE_NORMAL_OBJECT) {
//        // Setup slope
//        if (objects[*soa_id(object)].is_slope) {
//            int orientation = object->rotation / 90;
//            if (object->flippedH && object->flippedV) orientation += 2;
//            else if (object->flippedH) orientation += 1;
//            else if (object->flippedV) orientation += 3;
//            
//            orientation = orientation % 4;
//            if (orientation < 0) orientation += 4;
//
//            object->object.orientation = orientation;
//        }
//    }
//    
//    ObjectHitbox hitbox = objects[*soa_id(object)].hitbox;
//
//    // Modify height and width depending on rotation
//    if ((int) fabsf(object->rotation) % 180 != 0) {
//        object->width = hitbox.height * object->scale_y;
//        object->height = hitbox.width * object->scale_x;
//    } else {
//        object->width = hitbox.width * object->scale_x;
//        object->height = hitbox.height * object->scale_y;
//    }
//
//    
//    *soa_delta_x(object) = 0;
//    *soa_delta_y(object) = 0;
//    *soa_touching_player(object) = 0;
//    *soa_prev_touching_player(object) = 0;
//
//    object->has_two_channels = object->object.main_col_channel > 0 && object->object.detail_col_channel > 0;
//    return object;
}

bool obj_has_main(const GameObject *obj) {
    if (obj->color_type == COLOR_TYPE_BASE) return true;
    
    for (int i = 0; i < obj->child_count; i++) {
        if (obj->children[i].color_type == COLOR_TYPE_BASE) return true;
    }
    return false;
}

bool obj_has_detail(const GameObject *obj) {
    if (obj->color_type == COLOR_TYPE_DETAIL) return true;
    
    for (int i = 0; i < obj->child_count; i++) {
        if (obj->children[i].color_type == COLOR_TYPE_DETAIL) return true;
    }
    return false;
}

int parse_gd_object(const char *objStr, Object *obj) {
    int count = 0;
    // Split object into each key
    char **tokens = split_string(objStr, ',', &count);
    if (count < 1) {
        free_string_array(tokens, count);
        return 0;
    }

    // Iterate through all keys, up to MAX_OBJECT_PROPERTIES
    for (int i = 0; i + 1 < count; i += 2) {
        int key = atoi(tokens[i]);
        const char *valStr = tokens[i + 1];
        GDValueType type = get_value_type_for_key(key);
        GDValue val;

        switch (type) {
            case GD_VAL_INT:
                val.i = atoi(valStr);
                fill_object_data(obj, key, GD_VAL_INT, val);
                break;
            case GD_VAL_FLOAT:
                val.f = atof(valStr);
                fill_object_data(obj, key, GD_VAL_FLOAT, val);
                break;
            case GD_VAL_BOOL:
                val.b = parse_bool(valStr);
                fill_object_data(obj, key, GD_VAL_BOOL, val);
                break;
            default:
                break;
        }
    }
    
    const GameObject *game_object = &game_objects[obj->id];

    if (!obj_has_main(game_object) && obj_has_detail(game_object) && !obj->col_channel) {
        obj->col_channel = game_object->base_color;
    } else if (game_object->swap_base_detail) {
        if (!obj->detail_col_channel) obj->detail_col_channel = game_object->base_color;
    } else {
        if (!obj->col_channel) obj->col_channel = game_object->base_color;
        if (!obj->detail_col_channel) obj->detail_col_channel = 1;
    }

    free_string_array(tokens, count);
    return 1;
}

Object *parse_string(const char *levelString) {
    int sectionCount = 0;

    // Split the string in object sections
    char **sections = split_string(levelString, ';', &sectionCount);

    if (sectionCount < 3) {
        printf("Level string missing sections!\n");
        free_string_array(sections, sectionCount);
        return NULL;
    }
    
    int objectCount = sectionCount - 1;
    printf("%d\n", objectCount);

    Object *objectArray = malloc(sizeof(Object) * objectCount);
    if (!objectArray) {
        printf("Failed to allocate object array\n");
        return NULL;
    }

    printf("Parsing string and converting objects...\n");
    printf("%d bytes of pure objects", sizeof(Object) * objectCount);

    for (int i = 0; i < objectCount; i++) {
        Object *object = &objectArray[i];
        memset(object, 0, sizeof(Object));

        // Parse
        if (!parse_gd_object(sections[i + 1], object)) {
            printf("Failed to parse object %d\n", i);
            free_string_array(sections, sectionCount);
            free(objectArray);
            return NULL;
        }

        assign_object_to_section(object);
    }
    
    free_string_array(sections, sectionCount);

    return objectArray;
}

int load_level(char *path) {
    size_t out;
	char *level = read_file(path, &out);
	if (!level) return 0;

	char *data = decompress_level(level);
	if (data) {
        objectArray = parse_string(data);
        free(data);
        if (!objectArray) return 0;
    }
    free(level);

    init_col_channels();

    return 1;
}

void unload_level() {
    free(objectArray);
    free_sections();
    objectArray = NULL;
}
#pragma once
#include "objects.h"

#define MAX_GROUPS_PER_OBJECT 20

#define SECTION_HASH_SIZE 600

#define SECTION_SIZE 100

typedef enum {
    GD_VAL_INT,
    GD_VAL_FLOAT,
    GD_VAL_BOOL,
    GD_VAL_UNKNOWN
} GDValueType;

typedef union {
    int i;
    float f;
    bool b;
} GDValue;

typedef struct {
    int id;
    float x, y;
    float rotation;
    int zlayer, zorder;
    unsigned short col_channel;
    unsigned short detail_col_channel;
    unsigned char flippedH, flippedV;
} Object;

typedef struct Section {
    Object **objects;
    int object_count;
    int object_capacity;

    int x; // Section coordinates
    struct Section *next; // For chaining in hash map
} Section;

extern Object *objectArray;

char *read_file(const char *filepath, size_t *out_size);
char *decompress_level(char *data);
int load_level(char *path);
void unload_level();
Section *get_or_create_section(int x);
bool obj_has_main(const GameObject *obj);
bool obj_has_detail(const GameObject *obj);
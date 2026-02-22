#pragma once
#include "ui_element.h"

#define UI_MAX_ELEMENTS 64

typedef struct {
    UIElement elements[UI_MAX_ELEMENTS];
    int count;
} UIScreen;

typedef struct {
    const char* name;
    UIActionFn fn;
} UIAction;

void copy_tag_array(UIElement *e, char (*tag)[TAG_LENGTH]);
void ui_load_screen(UIScreen* screen, const UIAction* actions, size_t count, const char* path);

void ui_screen_update(UIScreen* screen, UIInput* touch);
void ui_screen_draw(UIScreen* screen);
UIElement *ui_get_element_by_tag(UIScreen *screen, const char *tag);
void ui_run_func_on_tag(UIScreen *screen, const char *tag, void (*func)(UIElement *e));

// Premade functions for on "ui_run_func_on_tag"

void ui_enable_element(UIElement *e);
void ui_disable_element(UIElement *e);
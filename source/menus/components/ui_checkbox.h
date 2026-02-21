#pragma once
#include "ui_element.h"

UIElement ui_create_checkbox(
    int x, int y, bool enabled,
    UIActionFn action,
    void *action_data,
    char *tag
);
void set_checkbox_enabled(UIElement *e, bool enabled);
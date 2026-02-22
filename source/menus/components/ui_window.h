#pragma once
#include "ui_element.h"
UIElement ui_create_window(
    int x, int y, int w, int h, int style,
    char (*tag)[TAG_LENGTH]
);
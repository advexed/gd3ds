#pragma once
#include "ui_element.h"

UIElement ui_create_label(int x, int y, float scale, char *text, int font, float alignment, char (*tag)[TAG_LENGTH]);
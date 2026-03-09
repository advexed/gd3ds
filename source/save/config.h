#pragma once

#include "utils/json_config.h"

extern Config cfg;

void cfg_init();
void cfg_save();
void cfg_fini();
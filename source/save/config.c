#include "config.h"

#include <3ds.h>
#include "main.h"
#include "graphics.h"

#include <sys/stat.h>
#include <sys/types.h>

#include "utils/gfx.h"

Config cfg;

void cfg_init() {
	// Make the directories
    mkdir(CONFIG_PARENT, 0777);
    mkdir(CONFIG_ROOT, 0777);

	config_load(&cfg, CONFIG_FILE);

	config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "aaEnabled", config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "aaEnabled", false));
	config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", false));

	set_aa(config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "aaEnabled", false));
	set_wide(config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", false));

	config_save(&cfg);
}

void cfg_save() {
    config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "aaEnabled", aaEnabled);
    config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", wideEnabled);

    config_save(&cfg);
}

void cfg_fini() {
	config_free(&cfg);
}
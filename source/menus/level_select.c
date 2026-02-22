#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"

UIScreen screen;

volatile bool start_level = false;


void action_open_level(void* data) { 
	start_level = true; 
};

void action_open_win0(void* data) { 
	ui_run_func_on_tag(&screen, "win0", ui_enable_element);
};

void action_close_win0(void* data) { 
	ui_run_func_on_tag(&screen, "win0", ui_disable_element);
};

UIAction actions[] = {
    {"open_level", action_open_level},
    {"open_win0", action_open_win0},
    {"close_win0", action_close_win0}
};

void level_select_loop() {
	C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/level_select.txt");

	ui_run_func_on_tag(&screen, "win0", ui_disable_element);

	bool prev_checked = false;

	while (aptMainLoop()) {
		hidScanInput();
        
		if (start_level) break;

		UIInput touch;
		touchPosition touchPos;
		hidTouchRead(&touchPos);
		touch.touchPosition = touchPos;
		touch.did_something = false;

		ui_screen_update(&screen, &touch);

		UIElement *checkbox = ui_get_element_by_tag(&screen, "checky");
		if (checkbox) {
			bool checked = checkbox->checkbox.checked;
			if (checked != prev_checked) {
				printf("%d\n", checkbox->checkbox.checked);
				prev_checked = checked;
			}
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(bot, C2D_Color32(3, 177, 255, 255));
        C2D_SceneBegin(bot);

		ui_screen_draw(&screen);
		
		C3D_FrameEnd(0);
	}
}
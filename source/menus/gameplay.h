#pragma once

#define MAX_DECIMAL_PERCENT 17; 
extern bool game_paused;

void gameplay_screen_init();
int gameplay_screen_top_loop();
int gameplay_screen_bot_loop();
void unpause_game();
void pause_game();
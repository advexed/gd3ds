#include "player.h"
#include "state.h"
#include "icons.h"
#include "graphics.h"

#include "menus/icon_kit.h"
#include "collision.h"
#include "math_helpers.h"


const float player_speeds[SPEED_COUNT] = {
	251.16007972276924,
	311.580093712804,
	387.42014039710523,
	468.0001388338566
};

const float cube_jump_heights[SPEED_COUNT] = {
    573.48,
    603.72,
    616.68,
    606.42,
};

float player_get_vel(Player *player, float vel) {
    return vel * (player->upside_down ? -1 : 1);
}

void set_p_velocity(Player *player, float vel) {
    player->vel_y = vel * ((player->mini) ? 0.8 : 1);
}

void cube_gamemode(Player *player) {
    int mult = (player->upside_down ? -1 : 1);
    
    //trail.positionR = (Vec2){player->x, player->y};  
    //trail.startingPositionInitialized = true;

    player->gravity = -2794.1082;
    
    if (player->vel_y < -810) player->vel_y = -810;
    if (player->vel_y > 1080) player->vel_y = 1080;

    if (player->y > 2794.f) state.dead = true;

    if (player->inverse_rotation) {
        player->rotation -= (415.3848f / 2) * STEPS_DT * mult * (player->mini ? 1.2f : 1.f);
    } else {
        player->rotation += 415.3848f * STEPS_DT * mult * (player->mini ? 1.2f : 1.f);
    }
    
    
    if (player->on_ground) {
        //MotionTrail_StopStroke(&trail);
        //if (!player->slope_data.slope) player->rotation = round(player->rotation / 90.0f) * 90.0f;
        player->rotation = roundf(player->rotation / 90.0f) * 90.0f;
    }
    /*
    // Player on ground or just left the ground
    if ((player->time_since_ground < 0.05f) && (frame_counter & 0b11) == 0) {
        particle_templates[CUBE_DRAG].angle = (player->upside_down ? -90 : 90);
        particle_templates[CUBE_DRAG].gravity_y = (player->upside_down ? 300 : -300);
        spawn_particle(CUBE_DRAG, getLeft(player) + 4, (player->upside_down ? getTop(player) - 2 : getBottom(player) + 2), NULL);
    }

    SlopeData slope_data = player->slope_data;

    // If not currently on slope, look at the last frame
    if (!player->slope_data.slope && player->slope_slide_coyote_time) {
        slope_data = player->coyote_slope;
    }*/
    //if ((slope_data.slope || player->on_ground) && (state.input.holdJump)) {

    if (player->on_ground && (state.input.holdJump)) {
        /*if (slope_data.slope) {
            // Slope jump
            int orient = grav_slope_orient(slope_data.slope, player);
            if (orient == ORIENT_NORMAL_UP || orient == ORIENT_UD_UP) {
                float time = clampf(10 * (player->timeElapsed - slope_data.elapsed), 0.4f, 1.0f);
                set_p_velocity(player, 0.25f * time * slopeHeights[state.speed] + cube_jump_heights[state.speed]);
            } else {
                set_p_velocity(player, cube_jump_heights[state.speed]);
            }
        } else {*/
            // Normal jump
            set_p_velocity(player, cube_jump_heights[state.speed]);
        //}
        player->inverse_rotation = false;
    
        player->on_ground = false;
        player->buffering_state = BUFFER_END;
    
        if (!(state.input.pressedJump)) {
            // This simulates the holding jump
            player->vel_y -= player->gravity * STEPS_DT;
        }
    }
}

void update_ship_rotation(Player *player) {
    float diff_x = (player->x - state.old_player.x);
    float diff_y = (player->y - state.old_player.y);
    float angle_rad = atan2f(-diff_y, diff_x);
    if (player->snap_rotation) {
        player->rotation = RadToDeg(angle_rad);
    } else {
        player->rotation = iSlerp(player->rotation, RadToDeg(angle_rad), 0.05f, STEPS_DT);
    }
}

void ship_gamemode(Player *player) {
    if (state.dual) {
        // Make both dual players symetric
        if (state.input.holdJump) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= -101.541492f)
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= -101.541492f)
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    } else {
        if (state.input.holdJump) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= grav(player, 101.541492f))
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= grav(player, 101.541492f))
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    }
    
    float min = player->mini ? -406.566f : -345.6f;
    float max = player->mini ? 508.248f : 432.0f;

    if (player->gravity < 0 && player->vel_y < min) {
        player->vel_y = min;
    } else if (player->gravity > 0 && player->vel_y > max) {
        player->vel_y = max;
    }
}


static float ballJumpHeights[SPEED_COUNT] = {
    -172.044007,
    -181.11601,
    -185.00401,
    -181.92601
};

void ball_gamemode(Player *player) {
    int mult = (player->upside_down ? -1 : 1);

    player->gravity = -1676.46672f;  
    
    if (player->on_ground || player->on_ceiling) {
        player->ball_rotation_speed = 2.3;
    }

    // Jump
    if ((state.input.holdJump) && (player->on_ground || player->on_ceiling) && player->buffering_state == BUFFER_READY) {        
        float delta_y = player->vel_y;

        player->upside_down ^= 1;

        set_p_velocity(player, ballJumpHeights[state.speed]);

        player->vel_y -= (delta_y < 0) ? 0 : delta_y;
        player->buffering_state = BUFFER_END;
        
        player->ball_rotation_speed = -1.f;

        player->on_ground = false;
    }
    
    player->rotation += player->ball_rotation_speed * mult * (player_speeds[state.speed] / player_speeds[SPEED_NORMAL]) / (player->mini ? 0.8 : 1);

    if (player->vel_y < -810) {
        player->vel_y = -810;
    } else if (player->vel_y > 810) {
        player->vel_y = 810;
    }
}

void ufo_gamemode(Player *player) {
    int mult = (player->upside_down ? -1 : 1);
    bool buffering_check = ((state.old_player.gamemode == GAMEMODE_PLAYER || state.old_player.gamemode == GAMEMODE_SHIP || state.old_player.gamemode == GAMEMODE_DART) && (state.input.holdJump));
    if (player->buffering_state == BUFFER_READY && (state.input.pressedJump || buffering_check)) {
        player->vel_y = fmaxf(player->vel_y, player->mini ? 358.992 : 371.034);
        player->buffering_state = BUFFER_END;
        player->ufo_last_y = player->y;
    } else {
        if (!state.dual) {
            if (player->vel_y > grav(player, 103.485494)) {
                player->gravity = player->mini ? -1969.92 : -1676.84;
            } else {
                player->gravity = player->mini ? -1308.96 : -1117.56;
            }
        } else {   
            if (player->vel_y > -103.485494) {
                player->gravity = player->mini ? -1969.92 : -1676.84;
            } else {
                player->gravity = player->mini ? -1308.96 : -1117.56;
            }
        }
    }

    if (player->on_ground) {
        player->ufo_last_y = player->y;
    }

    //if (!player->slope_data.slope) {
        float y_diff = (player->y - player->ufo_last_y) * mult;

        if (y_diff >= 0) {
            player->rotation = map_range(y_diff, 0.f, 60.f, 0.f, 10.f) * mult;
        } else {
            player->rotation = -map_range(-y_diff, 0.f, 300.f, 0.f, 25.f) * mult;
        }
    //}

    float min = player->mini ? -406.566f : -345.6f;
    float max = player->mini ? 508.248f : 432.0f;

    if (player->vel_y < min) {
        player->vel_y = min;
    } else if (player->vel_y > max) {
        player->vel_y = max;
    }
}

void wave_gamemode(Player *player) {
    if (player->buffering_state == BUFFER_READY) player->buffering_state = BUFFER_END;

    bool input = (state.input.holdJump);
    player->gravity = 0;

    player->vel_y = (input * 2 - 1) * player_speeds[state.speed] * (player->mini ? 2 : 1);
}

void run_player(Player *player) {
    //float scale = (player->mini) ? 0.6f : 1.f;
    //trail.stroke = 10.f * scale;
    
    if (!player->left_ground) {
        // Ground
        if (getGroundBottom(player) <= state.ground_y) {
            if (player->upside_down) {
                player->on_ceiling = true;
                player->inverse_rotation = false;
            } else {
                player->on_ground = true;          
                player->inverse_rotation = false;
            }
            player->time_since_ground = 0; 
        } 

        // Ceiling
        if (getGroundTop(player) >= state.ceiling_y) {
            if (player->upside_down) {
                player->on_ground = true;
                player->inverse_rotation = false;
            } else {
                player->on_ceiling = true;     
                player->inverse_rotation = false;     
            } 
            player->time_since_ground = 0; 
        } 
    }

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            cube_gamemode(player);
            break;
        case GAMEMODE_SHIP:
            ship_gamemode(player);
            break;
        case GAMEMODE_PLAYER_BALL:
            ball_gamemode(player);
            break;
        case GAMEMODE_BIRD:
            ufo_gamemode(player);
            break;
        case GAMEMODE_DART:
            wave_gamemode(player);
            break;
    }
    
    player->time_since_ground += STEPS_DT;

    /*if (player->gamemode != GAMEMODE_DART || player->cutscene_timer > 0) {
        if (wave_trail.opacity > 0) wave_trail.opacity -= 0.02f;
        
        if (wave_trail.opacity <= 0) {
            wave_trail.opacity = 0;
            wave_trail.nuPoints = 0;
        }
    }*/

    if (player->cutscene_timer > 0) return;

    player->rotation = normalize_angle(player->rotation);
    
    if (player->snap_rotation) {
        player->lerp_rotation = player->rotation;
    } else {
        if (player->gamemode == GAMEMODE_BIRD) {
            if (player->slope_data.slope_id) {
                player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.05f, STEPS_DT);
            } else {
                player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.1f, STEPS_DT);
            }
        } else {
            player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.2f, STEPS_DT);
        }
    }
    
    player->vel_x = player_speeds[state.speed];
    player->vel_y += player->gravity * STEPS_DT;
    player->y += player_get_vel(player, player->vel_y) * STEPS_DT;
    
    player->x += player->vel_x * STEPS_DT;

    player->left_ground = false;

    if (player->ceiling_inv_time > 0) {
        player->ceiling_inv_time -= STEPS_DT;
    } else {
        player->ceiling_inv_time = 0;
    }

    
    bool slopeCheck = false; //player->slope_data.slope && (grav_slope_orient(player->slope_data.slope, player) == ORIENT_NORMAL_DOWN || grav_slope_orient(player->slope_data.slope, player) == ORIENT_UD_DOWN);/

    if (getGroundBottom(player) < state.ground_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_PLAYER && player->upside_down) {
            state.dead = true;
        }

        if (slopeCheck) {
            //clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_DART && grav(player, player->vel_y) <= 0) player->vel_y = 0;
        player->y = state.ground_y + (player->height / 2) + ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0);;
    }

    // Ceiling
    if (getGroundTop(player) > state.ceiling_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_PLAYER && !player->upside_down) {
            state.dead = true;
        }

        if (slopeCheck) {
            //clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_DART && grav(player, player->vel_y) >= 0) player->vel_y = 0;
        player->y = state.ceiling_y - (player->height / 2) - ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0);;
    } 
    /*
    if (player->slope_slide_coyote_time) {
        player->slope_slide_coyote_time--;
        if (!player->slope_slide_coyote_time) {
            player->coyote_slope.slope = NULL;
            player->coyote_slope.elapsed = 0;
            player->coyote_slope.snapDown = FALSE;
        }
    }

    if (player->slope_data.slope) {
        slope_calc(player->slope_data.slope, player);
    }
*/
    if (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_DART) update_ship_rotation(player);

    player->snap_rotation = false;
}

void handle_player(Player *player) {
    if (state.input.holdJump) {
        if (player->buffering_state == BUFFER_NONE) {
            player->buffering_state = BUFFER_READY;
        }
    } else {
        player->buffering_state = BUFFER_NONE;
    }
    
    player->on_ground = false;
    player->on_ceiling = false;

    player->gravObj_id = -1;
    
    player->timeElapsed += STEPS_DT;

    collide_with_objects(player);
    
    run_player(player);
    
    player->delta_y = player->y - state.old_player.y;

}

void draw_player(Player *player) {
    float calc_x = ((player->x - state.camera_x));
    float calc_y = GSP_SCREEN_WIDTH - ((player->y - state.camera_y));

    /*MotionTrail_Update(&trail, dt);
    MotionTrail_UpdateWaveTrail(&wave_trail, dt);

    MotionTrail_Draw(&trail);
    MotionTrail_DrawWaveTrail(&wave_trail);*/

    float scale = (player->mini) ? 0.6f : 1.f;

    switch (player->gamemode) {
        case GAMEMODE_PLAYER:
            spawn_icon_at(GAMEMODE_PLAYER, selected_cube, player_glow_enabled, calc_x, calc_y, player->lerp_rotation, false, false, scale, 
                C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
                C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
        case GAMEMODE_SHIP:
            spawn_icon_at(GAMEMODE_SHIP, selected_ship, player_glow_enabled, calc_x, calc_y, player->lerp_rotation, false, player->upside_down, scale, 
                C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
                C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
        case GAMEMODE_PLAYER_BALL:
            spawn_icon_at(GAMEMODE_PLAYER_BALL, selected_ball, player_glow_enabled, calc_x, calc_y, player->lerp_rotation, false, false, scale, 
                C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
                C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
        case GAMEMODE_BIRD:
            spawn_icon_at(GAMEMODE_BIRD, selected_ufo, player_glow_enabled, calc_x, calc_y, player->lerp_rotation, false, player->upside_down, scale, 
                C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
                C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;    
        case GAMEMODE_DART:
            spawn_icon_at(GAMEMODE_DART, selected_wave, player_glow_enabled, calc_x, calc_y, player->lerp_rotation, false, false, scale, 
                C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
                C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
                C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
            );
            break;
    }
}
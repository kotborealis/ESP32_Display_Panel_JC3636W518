/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include "lvgl_port_v8.h"

static lv_obj_t *bird;
static lv_obj_t *pipe_top;
static lv_obj_t *pipe_bottom;
static lv_obj_t *score_label;

static int16_t bird_y;
static float bird_vel;
static int score;

static const int BIRD_SIZE = 20;
static const int PIPE_WIDTH = 30;
static const int GAP_HEIGHT = 80;
static const int PIPE_SPEED = 2;
static const float GRAVITY = 0.4f;
static const float FLAP_STRENGTH = -6.0f;

static int pipe_x;

static bool check_collision(lv_obj_t *a, lv_obj_t *b)
{
    lv_area_t aa;
    lv_area_t ab;
    lv_obj_get_coords(a, &aa);
    lv_obj_get_coords(b, &ab);
    if (aa.x2 < ab.x1 || aa.x1 > ab.x2 || aa.y2 < ab.y1 || aa.y1 > ab.y2) {
        return false;
    }
    return true;
}

static void reset_game(void)
{
    bird_y = lv_obj_get_height(lv_scr_act()) / 2 - BIRD_SIZE / 2;
    bird_vel = 0;
    score = 0;
    lv_label_set_text_fmt(score_label, "Score: %d", score);
    pipe_x = lv_obj_get_width(lv_scr_act());
    int gap_y = lv_rand(GAP_HEIGHT, lv_obj_get_height(lv_scr_act()) - GAP_HEIGHT);
    lv_obj_set_pos(pipe_top, pipe_x, gap_y - GAP_HEIGHT - lv_obj_get_height(pipe_top));
    lv_obj_set_pos(pipe_bottom, pipe_x, gap_y + GAP_HEIGHT);
}

static void flap_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    bird_vel = FLAP_STRENGTH;
}

static void game_update_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    bird_vel += GRAVITY;
    bird_y += (int16_t)bird_vel;
    lv_obj_set_y(bird, bird_y);

    pipe_x -= PIPE_SPEED;
    lv_obj_set_x(pipe_top, pipe_x);
    lv_obj_set_x(pipe_bottom, pipe_x);

    if (pipe_x + PIPE_WIDTH < 0) {
        pipe_x = lv_obj_get_width(lv_scr_act());
        int gap_y = lv_rand(GAP_HEIGHT, lv_obj_get_height(lv_scr_act()) - GAP_HEIGHT);
        lv_obj_set_pos(pipe_top, pipe_x, gap_y - GAP_HEIGHT - lv_obj_get_height(pipe_top));
        lv_obj_set_pos(pipe_bottom, pipe_x, gap_y + GAP_HEIGHT);
        score++;
        lv_label_set_text_fmt(score_label, "Score: %d", score);
    }

    if (bird_y < 0 || bird_y + BIRD_SIZE > lv_obj_get_height(lv_scr_act())) {
        reset_game();
        return;
    }

    if (check_collision(bird, pipe_top) || check_collision(bird, pipe_bottom)) {
        reset_game();
    }
}

void setup()
{
    String title = "Flappy Bird";

    Serial.begin(115200);
    Serial.println(title + " start");

    Serial.println("Initialize panel device");
    ESP_Panel *panel = new ESP_Panel();
    panel->init();
#if LVGL_PORT_AVOID_TEAR
    ESP_PanelBus_RGB *rgb_bus = static_cast<ESP_PanelBus_RGB *>(panel->getLcd()->getBus());
    rgb_bus->configRgbFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
    rgb_bus->configRgbBounceBufferSize(LVGL_PORT_RGB_BOUNCE_BUFFER_SIZE);
#endif
    panel->begin();

    Serial.println("Initialize LVGL");
    lvgl_port_init(panel->getLcd(), panel->getTouch());

    lvgl_port_lock(-1);

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x87CEEB), 0);

    bird = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bird, BIRD_SIZE, BIRD_SIZE);
    lv_obj_set_style_bg_color(bird, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_border_width(bird, 0, 0);

    pipe_top = lv_obj_create(lv_scr_act());
    lv_obj_set_size(pipe_top, PIPE_WIDTH, lv_obj_get_height(lv_scr_act()));
    lv_obj_set_style_bg_color(pipe_top, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_width(pipe_top, 0, 0);

    pipe_bottom = lv_obj_create(lv_scr_act());
    lv_obj_set_size(pipe_bottom, PIPE_WIDTH, lv_obj_get_height(lv_scr_act()));
    lv_obj_set_style_bg_color(pipe_bottom, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_width(pipe_bottom, 0, 0);

    score_label = lv_label_create(lv_scr_act());
    lv_label_set_text(score_label, "Score: 0");
    lv_obj_align(score_label, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_add_event_cb(lv_scr_act(), flap_cb, LV_EVENT_CLICKED, NULL);

    reset_game();

    lv_timer_create(game_update_cb, 20, NULL);

    lvgl_port_unlock();

    Serial.println(title + " end");
}

void loop()
{
    delay(5);
}

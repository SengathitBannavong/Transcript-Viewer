#pragma once

/*
 * app_data.h — application data model and global context singleton.
 */

#include "raylib.h"
#include <sqlite3.h>
#include "struct_table.h"

#define DATA_WARN_BUF 2048
#define DYN_BUF_SIZE 32768

/* Target Ambitions within graduation honor bands */
typedef enum {
    FLEX_LOW = 0,
    FLEX_MED = 1,
    FLEX_HIGH = 2
} FlexLevel;

/* Graduation Honor Tiers */
typedef enum {
    HONOR_NONE = 0,
    HONOR_NORMAL = 1,
    HONOR_GOOD = 2,
    HONOR_EXCELLENT = 3,
    HONOR_GOD = 4
} HonorTier;

/* Unified App Context (Singleton Pattern) */
typedef struct AppContext {
    /* 1. Global Configurations */
    float font_scale;
    int   target_fps;
    bool  custom_font;
    Font  fonts[1];

    /* 2. SQLite Database handle */
    sqlite3 *db;

    /* 3. Session & User State */
    char     user_name[26];
    int      name_len;
    bool     name_input;
    bool     db_ready;
    Player   player;
    char     type_name[sizeSubjectType][64];
    GradRule grad_rules[sizeSubjectType];

    /* 4. UI Layout / Responsive State */
    int  screen_w;
    int  screen_h;
    bool is_mobile;
    bool drawer_open;
    bool row_hover;
    bool is_touch;
    int  active_nav;

    /* 5. Graduation Planner Target Configuration */
    HonorTier plan_target;
    FlexLevel plan_flex;

    /* 6. Command Palette & Result Toast State */
    bool  popup_open;
    char  cmd_buf[256];
    int   cmd_len;
    bool  has_result;
    float result_show_until;
    char  result_msg[256];

    /* 7. Click-to-Edit Score Popup State */
    bool edit_open;
    char edit_code[MAXSIZEID];
    char edit_mid_buf[32];
    int  edit_mid_len;
    char edit_fin_buf[32];
    int  edit_fin_len;
    int  edit_field;             /* 0 = mid, 1 = final */
    int  edit_ratio;             /* 1 = 50/50, 2 = 40/60, 3 = 30/70 */
    char edit_subject_name[MAXSIZENAME];
    char edit_error[64];

    /* 8. Warnings & Data Validation Buffers */
    char data_warnings_buf[DATA_WARN_BUF];
    int  data_warn_count;

    char filter_dept[64];

    /* 9. Per-Frame Temporary Dynamic Buffer Arena */
    char dyn_buf[DYN_BUF_SIZE];
    int  dyn_pos;
} AppContext;

/* Global Singleton Instance */
extern AppContext gApp;

/* Public Callbacks from main.c */
void RefreshPlayer(void);
void ShowToastFor(float seconds);
void ReturnToNameInput(void);

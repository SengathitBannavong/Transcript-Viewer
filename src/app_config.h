#pragma once

#include "raylib.h"
#include <stdbool.h>

typedef struct AppConfig {
    float font_scale;
    int target_fps;
} AppConfig;

typedef struct FontLoadResult {
    Font font;
    bool custom_font;
    bool has_configured_candidates;
} FontLoadResult;

AppConfig AppConfig_Default(void);
AppConfig AppConfig_Load(const char *path);
FontLoadResult AppConfig_LoadFont(const char *path);
void AppConfig_DrawMissingFontScreen(void);

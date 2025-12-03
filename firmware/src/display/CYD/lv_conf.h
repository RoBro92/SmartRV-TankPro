
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* General configuration */
#define LV_USE_ASSERT_NULL        0
#define LV_USE_ASSERT_MALLOC      0
#define LV_USE_ASSERT_STYLE       0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ         0

#define LV_COLOR_DEPTH            32
#define LV_COLOR_CHROMA_KEY       lv_color_hex(0xFF00FF)

#define LV_MEM_SIZE               (80U * 1024U)
#define LV_MEM_ADR                0
#define LV_MEM_BUF_MAX_NUM        16

/* Logging */
#define LV_USE_LOG                1
#define LV_LOG_LEVEL              LV_LOG_LEVEL_ERROR
#define LV_LOG_PRINTF             1

/* Tick configuration */
#define LV_TICK_CUSTOM            0
#define LV_DPI_DEF                130

/* Features */
#define LV_USE_DRAW_SW            1
#define LV_DRAW_SW_COMPLEX        1
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
#define LV_DRAW_SW_TEXT_CACHE     1

#define LV_BUILD_EXAMPLES         0
#define LV_USE_PERF_MONITOR       0
#define LV_USE_OS                 LV_OS_NONE

/* Fonts used by the generated UI */
#define LV_FONT_MONTSERRAT_10     1
#define LV_FONT_MONTSERRAT_12     1
#define LV_FONT_MONTSERRAT_16     1
#define LV_FONT_MONTSERRAT_18     1
#define LV_FONT_MONTSERRAT_20     1
#define LV_FONT_MONTSERRAT_24     1
#define LV_FONT_MONTSERRAT_26     1
#define LV_FONT_MONTSERRAT_30     1
#define LV_FONT_DEFAULT           &lv_font_montserrat_16

/* Widgets and themes */
#define LV_USE_THEME_DEFAULT      1
#define LV_USE_THEME_BASIC        0

#define LV_USE_USER_DATA          1
#define LV_USE_ANIMATION          1
#define LV_USE_IMG_TRANSFORM      1
#define LV_USE_TRANSFORM          1

#define LV_USE_ARC                1
#define LV_USE_BAR                1
#define LV_USE_BTN                1
#define LV_USE_BTNMATRIX          1
#define LV_USE_CANVAS             1
#define LV_USE_CHECKBOX           1
#define LV_USE_DROPDOWN           1
#define LV_USE_IMG                1
#define LV_USE_LABEL              1
#define LV_USE_LINE               1
#define LV_USE_ROLLER             1
#define LV_USE_SLIDER             1
#define LV_USE_SPINBOX            1
#define LV_USE_SWITCH             1
#define LV_USE_TEXTAREA           1

#define LV_USE_IMGFONT            0
#define LV_USE_MSGBOX             1
#define LV_USE_TABVIEW            1
#define LV_USE_WIN                1

/* Image decoders (kept lean) */
#define LV_USE_PNG                0
#define LV_USE_BMP                0
#define LV_USE_SJPG               0
#define LV_USE_GIF                0

#endif /* LV_CONF_H */

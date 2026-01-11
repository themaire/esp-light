/**
 * Configuration LVGL pour ESP32 + TFT 2.4" ILI9341
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Paramètres de couleur */
#define LV_COLOR_DEPTH 16        // RGB565 (écran ILI9341)
#define LV_COLOR_16_SWAP 0       // Pas de swap des bytes

/* Paramètres mémoire */
#define LV_MEM_CUSTOM 0          // Utiliser malloc/free standard
#define LV_MEM_SIZE (48U * 1024U) // 48KB pour LVGL (ESP32 a 520KB RAM)

/* Paramètres display */
#define LV_DPI_DEF 130           // DPI par défaut

/* Features à activer */
#define LV_USE_PERF_MONITOR 0    // Désactiver affichage FPS
#define LV_USE_MEM_MONITOR 0     // Désactiver monitor RAM

/* Animations */
#define LV_USE_ANIMATION 1

/* Widgets standards */
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_SLIDER 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_SWITCH 1

/* Thème par défaut */
#define LV_USE_THEME_DEFAULT 1

/* Font par défaut */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1

/* Paramètres de log (pour debug) */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
#define LV_LOG_PRINTF 1

/* Autres paramètres requis */
#define LV_TICK_CUSTOM 0
#define LV_USE_USER_DATA 1

#endif /* LV_CONF_H */

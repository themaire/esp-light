#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <FastLED.h>

#include "images.h"

// ===== CONFIGURATION MULTI-PLATEFORME =====
#if defined(ESP8266_BOARD)
    #define MCU_NAME "ESP8266 (D1 Mini)"
    // LED_PIN défini dans platformio.ini (GPIO4 = D2)
#elif defined(ESP32_BOARD)
    #define MCU_NAME "ESP32 (LOLIN D32 PRO)"
    // LED_PIN défini dans platformio.ini (GPIO25)
#else
    #error "Plateforme non supportée ! Utiliser ESP8266_BOARD ou ESP32_BOARD"
#endif

// Écran TFT
TFT_eSPI tft = TFT_eSPI();

// LEDs WS2812B
#define NUM_LEDS 16
CRGB leds[NUM_LEDS];

// Variables globales
bool ledsOn = false;
uint8_t numLedsActive = 16;  // Nombre de LEDs allumées (1-16)
uint8_t colorTemp = 1;       // Température: 0=chaud, 1=neutre, 2=froid
uint8_t brightness = 10;     // Puissance: 1-10 (10%-100%)

// Températures de couleur
struct ColorPreset {
    const char* name;
    int kelvin;
    uint8_t r, g, b;
};

ColorPreset colorTemps[] = {
    {"Chaud",  3000, 255, 147, 41},   // Orange/doré
    {"Neutre", 5000, 255, 214, 170},  // Blanc naturel
    {"Froid",  6500, 201, 226, 255}   // Bleuté
};

// Calibration tactile avec inversion X
struct TouchCalibration {
    float scaleX, scaleY;
    int16_t offsetX, offsetY;
} touchCal = {
    -1.02,  // scaleX négatif = inversion X
    1.05,   // scaleY
    326,    // offsetX
    -4      // offsetY
};

// Buffers LVGL
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[320 * 10];  // Buffer pour 10 lignes
static lv_color_t buf2[320 * 10];

// Objets LVGL
lv_obj_t *btnOnOff;
lv_obj_t *btnPlus;
lv_obj_t *btnMinus;
lv_obj_t *labelLedCount;
lv_obj_t *btnTempLeft;
lv_obj_t *btnTempRight;
lv_obj_t *labelTemp;
lv_obj_t *labelTempValue;
lv_obj_t *colorPreview;
lv_obj_t *slider;
lv_obj_t *labelBrightness;

// Fonction de flush pour LVGL
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

// Fonction de lecture tactile pour LVGL (avec calibration affine)
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint16_t rawX, rawY;
    bool touched = tft.getTouch(&rawX, &rawY);

    if (touched) {
        // Application de la transformation affine inverse
        // Pour retrouver les coordonnées écran depuis les coordonnées tactiles
        // touchX = screenX * scaleX + offsetX
        // donc screenX = (touchX - offsetX) / scaleX
        int16_t screenX = (rawX - touchCal.offsetX) / touchCal.scaleX;
        int16_t screenY = (rawY - touchCal.offsetY) / touchCal.scaleY;
        
        data->state = LV_INDEV_STATE_PR;
        data->point.x = screenX;
        data->point.y = screenY;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// Mise à jour des LEDs
void updateLEDs() {
    // Appliquer la puissance
    FastLED.setBrightness(brightness * 25.5);  // 10*25.5=255
    
    if (!ledsOn) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    } else {
        ColorPreset ct = colorTemps[colorTemp];
        CRGB color = CRGB(ct.r, ct.g, ct.b);
        
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            if (i < numLedsActive) {
                leds[i] = color;
            } else {
                leds[i] = CRGB::Black;
            }
        }
    }
    FastLED.show();
}

// Callback pour le bouton ON/OFF
void onoff_btn_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ledsOn = !ledsOn;
        
        // Changer la couleur et le texte du bouton
        lv_obj_t *label = lv_obj_get_child(btnOnOff, 0);
        if (ledsOn) {
            lv_obj_set_style_bg_color(btnOnOff, lv_color_hex(0x00AA00), 0);  // Vert
            lv_label_set_text(label, "ON");
            Serial.println("LEDs ON");
        } else {
            lv_obj_set_style_bg_color(btnOnOff, lv_color_hex(0xAA0000), 0);  // Rouge
            lv_label_set_text(label, "OFF");
            Serial.println("LEDs OFF");
        }
        updateLEDs();
    }
}

// Callback pour le bouton +
void plus_btn_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (numLedsActive < 16) {
            numLedsActive++;
            String count = String(numLedsActive) + "/16";
            lv_label_set_text(labelLedCount, count.c_str());
            Serial.print("LEDs actives: ");
            Serial.println(numLedsActive);
            updateLEDs();
        }
    }
}

// Callback pour le bouton -
void minus_btn_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (numLedsActive > 1) {
            numLedsActive--;
            String count = String(numLedsActive) + "/16";
            lv_label_set_text(labelLedCount, count.c_str());
            Serial.print("LEDs actives: ");
            Serial.println(numLedsActive);
            updateLEDs();
        }
    }
}

// Callback pour le bouton température gauche (<)
void temp_left_btn_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (colorTemp > 0) {
            colorTemp--;
            ColorPreset ct = colorTemps[colorTemp];
            lv_label_set_text(labelTempValue, ct.name);
            
            // Changer la couleur de l'aperçu
            lv_obj_set_style_bg_color(colorPreview, lv_color_make(ct.r, ct.g, ct.b), 0);
            
            Serial.print("Température: ");
            Serial.println(ct.name);
            updateLEDs();
        }
    }
}

// Callback pour le bouton température droite (>)
void temp_right_btn_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (colorTemp < 2) {
            colorTemp++;
            ColorPreset ct = colorTemps[colorTemp];
            lv_label_set_text(labelTempValue, ct.name);
            
            // Changer la couleur de l'aperçu
            lv_obj_set_style_bg_color(colorPreview, lv_color_make(ct.r, ct.g, ct.b), 0);
            
            Serial.print("Température: ");
            Serial.println(ct.name);
            updateLEDs();
        }
    }
}

// Callback pour le slider
void slider_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        int32_t value = lv_slider_get_value(slider);
        brightness = value;
        
        String percent = String(brightness * 10) + "%";
        lv_label_set_text(labelBrightness, percent.c_str());
        
        Serial.print("Puissance: ");
        Serial.print(brightness * 10);
        Serial.println("%");
        
        updateLEDs();
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== Selfie Light LVGL ===");
    Serial.print("MCU: ");
    Serial.println(MCU_NAME);
    Serial.print("LED Pin: GPIO");
    Serial.println(LED_PIN);

    // Initialiser TFT
    tft.init();
    tft.setRotation(1);  // Paysage
    tft.fillScreen(TFT_BLACK);

    // Calibration tactile
    uint16_t calData[5] = {275, 3620, 264, 3532, 1};
    tft.setTouch(calData);

    // Initialiser FastLED
    Serial.println("Initialisation WS2812B...");
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(255);
    FastLED.clear();
    FastLED.show();

    // Initialiser LVGL
    lv_init();
    Serial.println("LVGL initialisé");

    // Configurer le display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 320 * 10);

    // Configurer le driver d'affichage
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("Display configuré");

    // Configurer le driver tactile avec calibration
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    Serial.println("Tactile configuré (calibration affine)");
    
    // ===== FOND GRIS CLAIR =====
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xD0D0D0), 0);  // Gris clair

    // ===== IMAGE DE FOND =====
    lv_obj_t *imgFond = lv_img_create(lv_scr_act());
    lv_img_set_src(imgFond, &fond_lvgl_small);
    lv_obj_set_pos(imgFond, 120, 0);  // Position en haut à gauche
    Serial.println("Image de fond chargée");

    // ===== CRÉER L'INTERFACE =====
    
    // Bouton ON/OFF (gauche)
    btnOnOff = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnOnOff, 80, 50);
    lv_obj_set_pos(btnOnOff, 20, 10);
    lv_obj_set_style_bg_color(btnOnOff, lv_color_hex(0xAA0000), 0);  // Rouge par défaut
    lv_obj_set_style_radius(btnOnOff, 8, 0);
    lv_obj_add_event_cb(btnOnOff, onoff_btn_event, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *labelOnOff = lv_label_create(btnOnOff);
    lv_label_set_text(labelOnOff, "OFF");
    lv_obj_set_style_text_font(labelOnOff, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(labelOnOff, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(labelOnOff);

    // Bouton - (milieu gauche)
    btnMinus = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnMinus, 50, 50);
    lv_obj_set_pos(btnMinus, 120, 10);
    lv_obj_set_style_bg_color(btnMinus, lv_color_hex(0xDD6600), 0);  // Orange
    lv_obj_set_style_radius(btnMinus, 8, 0);
    lv_obj_add_event_cb(btnMinus, minus_btn_event, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *labelMinus = lv_label_create(btnMinus);
    lv_label_set_text(labelMinus, "-");
    lv_obj_set_style_text_font(labelMinus, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(labelMinus, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(labelMinus);

    // Bouton + (milieu droit)
    btnPlus = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnPlus, 50, 50);
    lv_obj_set_pos(btnPlus, 180, 10);
    lv_obj_set_style_bg_color(btnPlus, lv_color_hex(0x00AA00), 0);  // Vert
    lv_obj_set_style_radius(btnPlus, 8, 0);
    lv_obj_add_event_cb(btnPlus, plus_btn_event, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *labelPlus = lv_label_create(btnPlus);
    lv_label_set_text(labelPlus, "+");
    lv_obj_set_style_text_font(labelPlus, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(labelPlus, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(labelPlus);

    // Label "LEDs" et compteur (droite)
    lv_obj_t *labelLeds = lv_label_create(lv_scr_act());
    lv_label_set_text(labelLeds, "LEDs");
    lv_obj_set_style_text_font(labelLeds, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(labelLeds, lv_color_hex(0xEB02A5), 0);
    lv_obj_set_pos(labelLeds, 240, 15);
    
    labelLedCount = lv_label_create(lv_scr_act());
    lv_label_set_text(labelLedCount, "16/16");
    lv_obj_set_style_text_font(labelLedCount, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(labelLedCount, lv_color_hex(0xEB02A5), 0);
    lv_obj_set_pos(labelLedCount, 240, 35);

    // ===== DEUXIÈME RANGÉE: TEMPÉRATURE =====
    
    // Bouton < température (gauche)
    btnTempLeft = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnTempLeft, 50, 50);
    lv_obj_set_pos(btnTempLeft, 30, 90);
    lv_obj_set_style_bg_color(btnTempLeft, lv_color_hex(0x444444), 0);  // Gris foncé
    lv_obj_set_style_radius(btnTempLeft, 8, 0);
    lv_obj_add_event_cb(btnTempLeft, temp_left_btn_event, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *labelTempLeft = lv_label_create(btnTempLeft);
    lv_label_set_text(labelTempLeft, "<");
    lv_obj_set_style_text_font(labelTempLeft, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(labelTempLeft, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(labelTempLeft);

    // Bouton > température (droite)
    btnTempRight = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnTempRight, 50, 50);
    lv_obj_set_pos(btnTempRight, 160, 90);
    lv_obj_set_style_bg_color(btnTempRight, lv_color_hex(0x444444), 0);  // Gris foncé
    lv_obj_set_style_radius(btnTempRight, 8, 0);
    lv_obj_add_event_cb(btnTempRight, temp_right_btn_event, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *labelTempRight = lv_label_create(btnTempRight);
    lv_label_set_text(labelTempRight, ">");
    lv_obj_set_style_text_font(labelTempRight, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(labelTempRight, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(labelTempRight);

    // Label "Temp" (centre)
    labelTemp = lv_label_create(lv_scr_act());
    lv_label_set_text(labelTemp, "Temp");
    lv_obj_set_style_text_font(labelTemp, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(labelTemp, lv_color_hex(0xEB02A5), 0);
    lv_obj_set_pos(labelTemp, 95, 90);

    // Valeur température
    labelTempValue = lv_label_create(lv_scr_act());
    ColorPreset ct = colorTemps[colorTemp];
    lv_label_set_text(labelTempValue, ct.name);
    lv_obj_set_style_text_font(labelTempValue, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(labelTempValue, lv_color_hex(0xEB02A5), 0);
    lv_obj_set_pos(labelTempValue, 90, 110);

    // Aperçu couleur (cercle)
    colorPreview = lv_obj_create(lv_scr_act());
    lv_obj_set_size(colorPreview, 30, 30);
    lv_obj_set_pos(colorPreview, 105, 145);
    lv_obj_set_style_radius(colorPreview, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(colorPreview, lv_color_make(ct.r, ct.g, ct.b), 0);
    lv_obj_set_style_border_width(colorPreview, 0, 0);

    // ===== SLIDER DE PUISSANCE =====
    
    // Titre "Puissance"
    lv_obj_t *labelPuissance = lv_label_create(lv_scr_act());
    lv_label_set_text(labelPuissance, "Puissance");
    lv_obj_set_style_text_font(labelPuissance, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(labelPuissance, lv_color_hex(0xEB02A5), 0);
    lv_obj_set_pos(labelPuissance, 20, 190);

    // Slider
    slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider, 280, 20);
    lv_obj_set_pos(slider, 20, 210);
    lv_slider_set_range(slider, 1, 10);
    lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xFFD700), LV_PART_INDICATOR);  // Or
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xFFD700), LV_PART_KNOB);
    lv_obj_add_event_cb(slider, slider_event, LV_EVENT_VALUE_CHANGED, NULL);

    // Valeur en %
    labelBrightness = lv_label_create(lv_scr_act());
    String percent = String(brightness * 10) + "%";
    lv_label_set_text(labelBrightness, percent.c_str());
    lv_obj_set_style_text_font(labelBrightness, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(labelBrightness, lv_color_hex(0xEB02A5), 0);
    lv_obj_set_pos(labelBrightness, 140, 212);

    Serial.println("=== Interface créée ===");
}

void loop() {
    lv_tick_inc(5);       // Incrémenter le tick LVGL (5ms par cycle)
    lv_timer_handler();  // Gestion LVGL
    delay(5);             // LVGL a besoin d'être appelé régulièrement
}

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <FastLED.h>

// ===== CONFIGURATION MULTI-PLATEFORME =====
// Détection automatique du microcontrôleur
#if defined(ESP8266_BOARD)
    #define MCU_NAME "ESP8266 (D1 Mini)"
    // LED_PIN défini dans platformio.ini (GPIO4 = D2)
#elif defined(ESP32_BOARD)
    #define MCU_NAME "ESP32 (LOLIN D32 PRO)"
    // LED_PIN défini dans platformio.ini (GPIO23 par défaut)
#else
    #error "Plateforme non supportée ! Utiliser ESP8266_BOARD ou ESP32_BOARD"
#endif

// Écran TFT LOLIN 2.4"
TFT_eSPI tft = TFT_eSPI();

// LEDs WS2812B
#define NUM_LEDS 16         // Anneau de 16 LEDs
CRGB leds[NUM_LEDS];

// Variables globales - État du système
bool ledsOn = false;
uint8_t numLedsActive = 16;  // Nombre de LEDs allumées (1-16)
uint8_t colorTemp = 2;       // Température: 0=chaud, 1=neutre, 2=froid
uint8_t brightness = 10;     // Puissance: 1-10 (10%-100%, par pas de 10%)
bool debugTouch = false;     // Activer le debug pour recalibrer

// Variables pour répétition automatique au maintien du doigt
unsigned long lastActionTime = 0;
unsigned long touchStartTime = 0;
int lastTouchButton = -1;  // -1=aucun, 0=onoff, 1=plus, 2=minus, 3=tempLeft, 4=tempRight
const unsigned long REPEAT_INITIAL_DELAY = 500;  // Délai avant première répétition (ms)
const unsigned long REPEAT_RATE = 150;            // Délai entre répétitions (ms)

// ===== CALIBRATION TACTILE =====
// Points de référence pour transformation affine (basés sur calibration manuelle)
// Ces valeurs doivent être ajustées si le comportement tactile change
struct TouchCalibration {
    float scaleX, scaleY;    // Facteurs d'échelle
    int16_t offsetX, offsetY; // Offsets
} touchCal = {
    -1.02,  // scaleX (inversion X + compression)
    1.05,   // scaleY (légère expansion Y)
    326,    // offsetX
    -4      // offsetY
};

// Fonction de transformation: coordonnées écran → coordonnées tactiles
void screenToTouch(int16_t screenX, int16_t screenY, int16_t screenW, int16_t screenH,
                   int16_t &touchX, int16_t &touchY, int16_t &touchW, int16_t &touchH) {
    // Transformation affine du centre
    int16_t centerScreenX = screenX + screenW / 2;
    int16_t centerScreenY = screenY + screenH / 2;
    
    int16_t centerTouchX = centerScreenX * touchCal.scaleX + touchCal.offsetX;
    int16_t centerTouchY = centerScreenY * touchCal.scaleY + touchCal.offsetY;
    
    // Transformation des dimensions (valeur absolue car X peut être inversé)
    touchW = abs(screenW * touchCal.scaleX);
    touchH = abs(screenH * touchCal.scaleY);
    
    // Calcul du coin supérieur gauche de la zone tactile
    touchX = centerTouchX - touchW / 2;
    touchY = centerTouchY - touchH / 2;
}

// Températures de couleur (RGB)
struct ColorPreset {
    const char* name;
    int kelvin;
    uint8_t r, g, b;
};

ColorPreset colorTemps[] = {
    {"Chaud",  3000, 255, 147, 41},  // Orange/doré
    {"Neutre", 5000, 255, 214, 170},  // Blanc naturel
    {"Froid",  6500, 201, 226, 255}   // Bleuté
};

// Boutons de l'interface
struct Button {
    int16_t x, y, w, h;
    int16_t touchX, touchY, touchW, touchH;
    const char* label;
    uint16_t color;
    
    // Constructeur qui calcule automatiquement les coordonnées tactiles
    Button(int16_t px, int16_t py, int16_t pw, int16_t ph, const char* lbl, uint16_t col) 
        : x(px), y(py), w(pw), h(ph), label(lbl), color(col) {
        screenToTouch(x, y, w, h, touchX, touchY, touchW, touchH);
    }
};

// Définition des boutons (les coordonnées tactiles sont auto-calculées!)
Button btnOnOff(20, 10, 80, 50, "ON/OFF", TFT_RED);
Button btnPlus(170, 10, 40, 50, "+", TFT_GREEN);
Button btnMinus(120, 10, 40, 50, "-", TFT_ORANGE);
Button btnTempLeft(30, 90, 50, 50, "<", TFT_DARKGREY);
Button btnTempRight(160, 90, 50, 50, ">", TFT_DARKGREY);

// ===== FONCTIONS =====

// ===== FONCTIONS =====

bool isTouchInButton(Button &btn, uint16_t touchX, uint16_t touchY) {
    return (touchX >= btn.touchX && touchX <= (btn.touchX + btn.touchW) &&
            touchY >= btn.touchY && touchY <= (btn.touchY + btn.touchH));
}

void updateLEDs() {
    // Appliquer la puissance (brightness 1-10 = 10%-100%)
    FastLED.setBrightness(brightness * 25.5); // 10*25.5=255
    
    if (!ledsOn) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    } else {
        ColorPreset ct = colorTemps[colorTemp];
        CRGB color = CRGB(ct.r, ct.g, ct.b);
        
        // Allumer seulement le nombre de LEDs actives
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

void drawSlider() {
    // Position du slider
    int16_t sliderY = 180;
    int16_t sliderX = 20;
    int16_t sliderW = 280;
    int16_t sliderH = 30;
    int16_t segmentW = sliderW / 10;
    
    // Titre
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("Puissance", sliderX, sliderY - 20, 2);
    
    // Dessiner les 10 segments
    for (int i = 0; i < 10; i++) {
        int16_t segX = sliderX + i * segmentW;
        uint16_t segColor = TFT_DARKGREY;
        
        // Segment actuel = curseur doré
        if (i == brightness - 1) {
            segColor = TFT_GOLD;
        }
        
        tft.fillRoundRect(segX + 2, sliderY, segmentW - 4, sliderH, 4, segColor);
    }
    
    // Afficher le pourcentage
    tft.setTextDatum(TC_DATUM);
    String percent = String(brightness * 10) + "%";
    tft.drawString(percent, sliderX + sliderW / 2, sliderY + sliderH + 5, 2);
}

bool isTouchInSlider(uint16_t touchX, uint16_t touchY, uint8_t &newBrightness) {
    // Zone du slider en coordonnées écran
    int16_t sliderY = 180;
    int16_t sliderX = 20;
    int16_t sliderW = 280;
    int16_t sliderH = 30;
    
    // Transformer en coordonnées tactiles
    int16_t tX, tY, tW, tH;
    screenToTouch(sliderX, sliderY, sliderW, sliderH, tX, tY, tW, tH);
    
    // Vérifier si dans la zone
    if (touchX >= tX && touchX <= (tX + tW) &&
        touchY >= tY && touchY <= (tY + tH)) {
        
        // Calculer quel segment (1-10)
        // IMPORTANT: L'axe X est inversé (scaleX négatif), donc on inverse le calcul
        int16_t relativeX = abs(touchX - tX);
        int16_t segmentW = tW / 10;
        newBrightness = 10 - (relativeX / segmentW);  // Inversion pour compenser scaleX négatif
        if (newBrightness > 10) newBrightness = 10;
        if (newBrightness < 1) newBrightness = 1;
        
        return true;
    }
    return false;
}

void drawButton(Button &btn, bool pressed = false) {
    uint16_t color = pressed ? TFT_DARKGREY : btn.color;
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 8, color);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    
    // Texte plus épais (simuler le gras en dessinant plusieurs fois)
    int16_t centerX = btn.x + btn.w/2;
    int16_t centerY = btn.y + btn.h/2;
    tft.drawString(btn.label, centerX, centerY, 4);
    tft.drawString(btn.label, centerX+1, centerY, 4);
}

void drawInterface() {
    tft.fillScreen(TFT_BLACK);
    
    // État ON/OFF
    btnOnOff.color = ledsOn ? TFT_GREEN : TFT_RED;
    btnOnOff.label = ledsOn ? "ON" : "OFF";
    drawButton(btnOnOff);
    
    // Boutons quantité
    drawButton(btnPlus);
    drawButton(btnMinus);
    
    // Info quantité
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("LEDs", 240, 20, 2);
    String ledCount = String(numLedsActive) + "/16";
    tft.drawString(ledCount, 240, 40, 2);
    
    // Boutons température
    drawButton(btnTempLeft);
    drawButton(btnTempRight);
    
    // Info température
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Temp", 120, 90, 2);
    ColorPreset ct = colorTemps[colorTemp];
    tft.drawString(ct.name, 120, 110, 2);
    String kelvin = String(ct.kelvin) + "K";
    tft.drawString(kelvin, 120, 125, 2);
    
    // Aperçu couleur
    tft.fillCircle(120, 150, 15, tft.color565(ct.r, ct.g, ct.b));
    
    // Slider de puissance
    drawSlider();
}

// ===== SETUP & LOOP =====

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== Selfie Light Pro ===");
    Serial.print("Microcontrôleur: ");
    Serial.println(MCU_NAME);
    Serial.print("LED Pin: GPIO");
    Serial.println(LED_PIN);
    
    // Initialiser l'écran TFT
    Serial.println("Initialisation TFT...");
    tft.init();
    tft.setRotation(1); // Paysage
    tft.fillScreen(TFT_BLACK);
    
    // Calibrer le tactile (valeurs par défaut pour LOLIN TFT 2.4")
    uint16_t calData[5] = {275, 3620, 264, 3532, 1};
    tft.setTouch(calData);
    
    // Initialiser les LEDs WS2812B
    Serial.println("Initialisation WS2812B...");
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(255);
    FastLED.clear();
    FastLED.show();
    
    // Dessiner l'interface
    Serial.println("Création de l'interface...");
    drawInterface();
    
    Serial.println("=== Système prêt ! ===");
}

void loop() {
    uint16_t touchX, touchY;
    bool isTouched = tft.getTouch(&touchX, &touchY);
    unsigned long currentTime = millis();
    
    // Vérifier si l'écran est touché
    if (isTouched) {
        if (debugTouch) {
            // Afficher les coordonnées en haut de l'écran
            tft.fillRect(0, 0, 320, 30, TFT_BLACK);
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.setTextDatum(TL_DATUM);
            tft.drawString("X:" + String(touchX) + " Y:" + String(touchY), 5, 5, 2);
            Serial.print("Touch X:");
            Serial.print(touchX);
            Serial.print(" Y:");
            Serial.println(touchY);
        }
        
        bool needsUpdate = false;
        int currentButton = -1;
        uint8_t newBrightness = 0;
        
        // Vérifier d'abord le slider
        if (isTouchInSlider(touchX, touchY, newBrightness)) {
            if (newBrightness != brightness) {
                brightness = newBrightness;
                Serial.print("Puissance: ");
                Serial.print(brightness * 10);
                Serial.println("%");
                needsUpdate = true;
            }
        }
        // Déterminer quel bouton est touché
        else if (isTouchInButton(btnOnOff, touchX, touchY)) {
            currentButton = 0;
        } else if (isTouchInButton(btnPlus, touchX, touchY)) {
            currentButton = 1;
        } else if (isTouchInButton(btnMinus, touchX, touchY)) {
            currentButton = 2;
        } else if (isTouchInButton(btnTempLeft, touchX, touchY)) {
            currentButton = 3;
        } else if (isTouchInButton(btnTempRight, touchX, touchY)) {
            currentButton = 4;
        }
        
        // Nouveau toucher ou changement de bouton
        if (currentButton != lastTouchButton) {
            lastTouchButton = currentButton;
            touchStartTime = currentTime;
            lastActionTime = currentTime;
            
            // Exécuter l'action immédiatement pour le premier toucher
            if (currentButton == 0) {
                ledsOn = !ledsOn;
                Serial.println(ledsOn ? "LEDs ON" : "LEDs OFF");
                needsUpdate = true;
            } else if (currentButton == 1 && numLedsActive < 16) {
                numLedsActive++;
                Serial.print("LEDs actives: ");
                Serial.println(numLedsActive);
                needsUpdate = true;
            } else if (currentButton == 2 && numLedsActive > 1) {
                numLedsActive--;
                Serial.print("LEDs actives: ");
                Serial.println(numLedsActive);
                needsUpdate = true;
            } else if (currentButton == 3 && colorTemp > 0) {
                colorTemp--;
                Serial.print("Température: ");
                Serial.println(colorTemps[colorTemp].name);
                needsUpdate = true;
            } else if (currentButton == 4 && colorTemp < 2) {
                colorTemp++;
                Serial.print("Température: ");
                Serial.println(colorTemps[colorTemp].name);
                needsUpdate = true;
            }
        }
        // Toucher maintenu - répétition automatique
        else if (currentButton != -1 && currentButton != 0) {  // Pas de répétition pour ON/OFF
            unsigned long timeSinceStart = currentTime - touchStartTime;
            unsigned long timeSinceLastAction = currentTime - lastActionTime;
            
            // Attendre le délai initial, puis répéter à intervalles réguliers
            if (timeSinceStart > REPEAT_INITIAL_DELAY && timeSinceLastAction > REPEAT_RATE) {
                lastActionTime = currentTime;
                
                if (currentButton == 1 && numLedsActive < 16) {
                    numLedsActive++;
                    Serial.print("LEDs actives: ");
                    Serial.println(numLedsActive);
                    needsUpdate = true;
                } else if (currentButton == 2 && numLedsActive > 1) {
                    numLedsActive--;
                    Serial.print("LEDs actives: ");
                    Serial.println(numLedsActive);
                    needsUpdate = true;
                } else if (currentButton == 3 && colorTemp > 0) {
                    colorTemp--;
                    Serial.print("Température: ");
                    Serial.println(colorTemps[colorTemp].name);
                    needsUpdate = true;
                } else if (currentButton == 4 && colorTemp < 2) {
                    colorTemp++;
                    Serial.print("Température: ");
                    Serial.println(colorTemps[colorTemp].name);
                    needsUpdate = true;
                }
            }
        }
        
        if (needsUpdate) {
            updateLEDs();
            drawInterface();
        }
    } else {
        // Plus de toucher - réinitialiser
        lastTouchButton = -1;
    }
    
    delay(50);
}

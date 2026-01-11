# ESP Light - Éclairage Photo (comme un) professionnel

Système d'éclairage LED contrôlable avec interface tactile pour photographie.

Projet non terminé car il reste à concevoir la partie à imprimer en 3D qui servira de boitier / support.

## 📋 Prérequis

### Environnement de développement
- **Visual Studio Code** : Éditeur de code recommandé
- **Extension PlatformIO IDE** : Pour la compilation et l'upload du firmware ESP8266
- **Python 3** : Requis par PlatformIO (version 3.7 ou supérieure)
- **Drivers USB** : CH340/CP2102 pour la communication série avec le Wemos D1 Mini

### Système d'exploitation
- macOS 15.1.1 (testé et validé)
- Compatible Linux et Windows

### Installation
```bash
# 1. Installer VS Code
# Télécharger depuis: https://code.visualstudio.com/

# 2. Installer l'extension PlatformIO
# Dans VS Code: Extensions (Cmd+Shift+X) → Rechercher "PlatformIO IDE" → Installer

# 3. Créer l'environnement Python virtuel (optionnel mais recommandé)
python3 -m venv venv
source venv/bin/activate  # Sur macOS/Linux
# venv\Scripts\activate   # Sur Windows

# 4. Installer les outils Python
pip install platformio esptool
```

## 🎥 Vidéo de démonstration

[![Démonstration ESP Light](https://img.youtube.com/vi/Rhpj27oxEJo/maxresdefault.jpg)](https://www.youtube.com/watch?v=Rhpj27oxEJo)

![](./picts/UI_1.jpg)
![](./picts/UI_2.jpg)
![](./picts/UI_3.jpg)

## 🎯 Caractéristiques

- **Contrôle tactile** : Interface graphique intuitive sur écran TFT 2.4" (240x320 pixels)
- **16 LEDs WS2812B** : Anneau RGB addressable avec contrôle individuel
- **Intensité variable** : De 1 à 16 LEDs allumées progressivement
- **Puissance réglable** : Slider tactile avec 10 niveaux de luminosité (10%-100%)
- **Balance des blancs** : 3 températures de couleur (3000K / 5000K / 6500K)
- **Mode ON/OFF** : Activation/désactivation instantanée
- **Calibration tactile** : Mapping précis entre écran LCD et capteur tactile XPT2046

## 📦 Hardware

- **Microcontrôleur** : [ESP8266 (Wemos D1 Mini)](https://fr.aliexpress.com/item/32529101036.html) - 80MHz, 80KB RAM, 4MB Flash
- **Écran** : [LOLIN TFT 2.4" Shield (ILI9341 240x320, 16-bit color)](https://fr.aliexpress.com/item/32919729730.html?pdp_npi=4%40dis%21EUR%21€%2017%2C04%21€%2016%2C99%21%21%2119.38%2119.32%21%4021038e4017681636976566811db158%2166057397051%21sh%21FR%211709736453%21X&spm=a2g0o.store_pc_home.productList_2009695634913.32919729730&gatewayAdapt=glo2fra)
- **Capteur tactile (intégré dans l'écran LILIN TFT)** : XPT2046 (résistif, nécessite calibration)
- **LEDs** : [Anneau 16 LEDs WS2812B](https://fr.aliexpress.com/item/1005007748593752.html) sur GPIO4 (D2)
- **Port série** : `/dev/tty.usbserial-0206A689` (sur mon ordinateur) @ 115200 bauds

## 🎨 Interface

### Contrôles disponibles
- **ON/OFF** (rouge/vert) : Allumer/éteindre toutes les LEDs
- **+** (vert) : Augmenter le nombre de LEDs actives (1-16)
- **-** (orange) : Diminuer le nombre de LEDs actives (1-16)
- **◀** (gris) : Température précédente (Froid → Neutre → Chaud)
- **▶** (gris) : Température suivante (Chaud → Neutre → Froid)
- **Slider de puissance** (en bas) : 10 segments tactiles pour ajuster la luminosité de 10% à 100% par pas de 10%. Le segment actif est affiché en doré, les autres en gris foncé.

### Affichage en temps réel
- État du système : "LEDs ON" / "LEDs OFF"
- Nombre de LEDs actives : "X/16 LEDs"
- Température actuelle : "Chaud (3000K)" / "Neutre (5000K)" / "Froid (6500K)"
- Puissance : Affichage du pourcentage sous le slider (ex: "50%")
- Aperçu couleur : Cercle de prévisualisation de la température actuelle

### Température de couleur
| Mode | Kelvin | Couleur | Usage |
|------|--------|---------|-------|
| **Chaud** | 3000K | Orange/Doré | Ambiance chaleureuse, lever/coucher de soleil |
| **Neutre** | 5000K | Blanc naturel | Lumière du jour, portraits naturels |
| **Froid** | 6500K | Bleuté | Studio photo, éclairage technique |

## 🎯 Calibration tactile

### Pourquoi calibrer ?
Le capteur tactile **XPT2046** utilise son propre système de coordonnées qui **ne correspond pas** directement aux coordonnées de l'écran LCD **ILI9341**. Les raisons :
- Rotation différente entre LCD et capteur tactile
- Offset/décalage des origines
- Résolution native différente
- Possible inversion d'axes (X/Y)

### Transformation automatique
Le projet utilise une **transformation affine** pour convertir automatiquement les coordonnées graphiques en coordonnées tactiles. Plus besoin de calibrer manuellement chaque bouton!

#### Coefficients de calibration actuels
```cpp
struct TouchCalibration {
    float scaleX = -1.02;    // Inversion X + compression
    float scaleY = 1.05;     // Légère expansion Y
    int16_t offsetX = 326;   // Décalage X
    int16_t offsetY = -4;    // Décalage Y
};
```

#### Positions des boutons (auto-calibrées)
```cpp
// Les coordonnées tactiles sont calculées automatiquement via screenToTouch()
Button btnOnOff(20, 10, 80, 50, "ON/OFF", TFT_RED);
Button btnPlus(170, 10, 40, 50, "+", TFT_GREEN);
Button btnMinus(120, 10, 40, 50, "-", TFT_ORANGE);
Button btnTempLeft(30, 90, 50, 50, "<", TFT_DARKGREY);
Button btnTempRight(160, 90, 50, 50, ">", TFT_DARKGREY);
```

### Recalibration (si nécessaire)
Si vous devez ajuster la calibration pour votre écran :
1. Activer le mode debug : `debugTouch = true`
2. Compiler et uploader le firmware
3. Toucher plusieurs boutons et noter les coordonnées affichées
4. Calculer les nouveaux coefficients :
   - `scaleX = (touchX2 - touchX1) / (screenX2 - screenX1)`
   - `scaleY = (touchY2 - touchY1) / (screenY2 - screenY1)`
   - `offsetX` et `offsetY` : ajuster pour centrer
5. Mettre à jour la structure `TouchCalibration`
6. Désactiver le mode debug : `debugTouch = false`

## 🚀 Installation

### 1. Environnement Python (comme cité au tout debut du README)
```bash
python3 -m venv venv
source venv/bin/activate
pip install platformio esptool
```

### 2. Compilation et Upload
```bash
# Compiler
pio run

# Compiler et uploader
pio run -t upload

# Moniteur série
pio device monitor --baud 115200
```

## 📁 Structure du projet

```
esp-light/
├── platformio.ini      # Configuration PlatformIO
├── src/
│   └── main.cpp       # Code source principal
├── include/
│   └── lv_conf.h      # Configuration LVGL (non utilisée)
├── lib/               # Bibliothèques locales
├── test/              # Tests unitaires
└── venv/              # Environnement Python
```

## 🔧 Configuration matérielle

### Broches TFT Shield
| Pin D1 Mini | GPIO | Fonction |
|-------------|------|----------|
| D0 | 16 | TFT_CS |
| D3 | 0 | TOUCH_CS |
| D5 | 14 | SCK |
| D6 | 12 | MISO |
| D7 | 13 | MOSI |
| D8 | 15 | TFT_DC |

### LEDs WS2812B
- **GPIO** : D2 (GPIO4)
- **Nombre** : 16 LEDs en anneau
- **Alimentation** : 5V (via D1 Mini ou externe)

## 📚 Bibliothèques utilisées

- `TFT_eSPI@2.5.43` : Driver d'écran ILI9341 optimisé pour ESP8266
- `FastLED@3.5.0` : Contrôle des LEDs WS2812B (version compatible GCC 4.8.2)

## ⚙️ Architecture technique

### Mémoire
- **RAM totale** : 80KB (utilisation ~34% = 28KB)
- **Flash** : 4MB (utilisation ~31% = 324KB)
- **Contrainte** : Pas de LVGL possible (trop gourmand en RAM)

### Communication
- **SPI** : Écran TFT (27MHz)
- **Tactile** : XPT2046 via TFT_eSPI
- **LEDs** : Protocol WS2812B (timing précis 800kHz)

### Structure du code
```cpp
// Structure pour préréglages de couleurs
struct ColorPreset {
    const char* name;
    int kelvin;
    uint8_t r, g, b;
};

// Structure pour boutons avec double mapping et auto-calibration
struct Button {
    int16_t x, y, w, h;                    // Coordonnées graphiques (LCD)
    int16_t touchX, touchY, touchW, touchH; // Coordonnées tactiles (XPT2046)
    const char* label;
    uint16_t color;
    
    // Constructeur qui calcule automatiquement les coordonnées tactiles
    Button(int16_t px, int16_t py, int16_t pw, int16_t ph, const char* lbl, uint16_t col);
};
```

## 💡 Utilisation

1. Connecter l'anneau de LEDs sur D2 (GPIO4)
2. Monter le shield TFT sur le D1 Mini
3. Alimenter via USB ou batterie
4. Utiliser l'interface tactile pour contrôler l'éclairage

## 🎬 Application Photo

Idéal pour :
- Selfies et portraits
- Vidéos YouTube/streaming
- Éclairage d'appoint macro
- Éclairage de studio mobile

## 📝 Développé avec

- **IDE** : Visual Studio Code + PlatformIO
- **Framework** : Arduino pour ESP8266
- **Langage** : C++

## 🎨 Couleurs TFT_eSPI disponibles

Couleurs prédéfinies de la bibliothèque TFT_eSPI (format RGB565) :

### Couleurs de base
- `TFT_BLACK` (0, 0, 0)
- `TFT_WHITE` (255, 255, 255)
- `TFT_RED` (255, 0, 0)
- `TFT_GREEN` (0, 255, 0)
- `TFT_BLUE` (0, 0, 255)
- `TFT_CYAN` (0, 255, 255)
- `TFT_MAGENTA` (255, 0, 255)
- `TFT_YELLOW` (255, 255, 0)

### Couleurs étendues
- `TFT_ORANGE` (255, 180, 0)
- `TFT_GREENYELLOW` (180, 255, 0)
- `TFT_PINK` (255, 192, 203)
- `TFT_BROWN` (150, 75, 0)
- `TFT_GOLD` (255, 215, 0)
- `TFT_SILVER` (192, 192, 192)
- `TFT_SKYBLUE` (135, 206, 235)
- `TFT_VIOLET` (180, 46, 226)
- `TFT_PURPLE` (128, 0, 128)
- `TFT_OLIVE` (128, 128, 0)
- `TFT_LIGHTGREY` (211, 211, 211)
- `TFT_DARKGREY` (128, 128, 128)

### Couleur personnalisée
Pour créer vos propres couleurs RGB :
```cpp
uint16_t customColor = tft.color565(r, g, b);  // r, g, b: 0-255
```

---

*Projet créé en janvier 2026*

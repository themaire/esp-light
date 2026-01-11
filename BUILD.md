# Guide de compilation multi-plateforme

## 🎯 Microcontrôleurs supportés

Ce projet supporte deux microcontrôleurs :
- **ESP8266** (Wemos D1 Mini) - Configuration `d1_mini`
- **ESP32** (WEMOS LOLIN D32 PRO) - Configuration `lolin_d32_pro`

## 🔧 Compilation pour ESP8266 (D1 Mini)

### Compiler uniquement
```bash
pio run -e d1_mini
```

### Compiler et uploader
```bash
pio run -e d1_mini -t upload
```

### Moniteur série
```bash
pio device monitor -e d1_mini
```

### Nettoyage du cache
```bash
pio run -e d1_mini -t clean
```

## 🔧 Compilation pour ESP32 (LOLIN D32 PRO)

### ⚠️ Important : Pins à vérifier !
Les pins du shield TFT sur ESP32 peuvent être différents. Vérifiez et adaptez dans [platformio.ini](platformio.ini) :
```ini
[env:lolin_d32_pro]
build_flags = 
    -D LED_PIN=23          ; GPIO pour anneau WS2812B
    -D TFT_MISO=19         ; VSPI MISO
    -D TFT_MOSI=23         ; VSPI MOSI
    -D TFT_SCLK=18         ; VSPI SCK
    -D TFT_CS=5            ; Chip Select TFT
    -D TFT_DC=2            ; Data/Command
    -D TOUCH_CS=15         ; Chip Select Touch
```

### Compiler uniquement
```bash
pio run -e lolin_d32_pro
```

### Compiler et uploader
```bash
pio run -e lolin_d32_pro -t upload
```

### Moniteur série
```bash
pio device monitor -e lolin_d32_pro
```

### Nettoyage du cache
```bash
pio run -e lolin_d32_pro -t clean
```

## 🔄 Changement de microcontrôleur

### ⚠️ Important : Nettoyage obligatoire !
Quand vous passez d'un microcontrôleur à l'autre, **il faut impérativement nettoyer le cache** :

```bash
# Nettoyer le cache de compilation
pio run -t clean

# OU nettoyer tout le projet
rm -rf .pio
```

### Pourquoi nettoyer ?
- Les bibliothèques compilées pour ESP8266 ne sont pas compatibles avec ESP32
- Les fichiers `.o` et `.a` dans `.pio/build/` contiennent du code spécifique à chaque plateforme
- PlatformIO peut réutiliser les anciens binaires si vous ne nettoyez pas

### Workflow recommandé
```bash
# 1. Nettoyer le cache
pio run -t clean

# 2. Compiler pour la plateforme cible
pio run -e lolin_d32_pro -t upload

# 3. Vérifier le bon microcontrôleur dans le moniteur série
pio device monitor -e lolin_d32_pro
# Vous devriez voir : "Microcontrôleur: ESP32 (LOLIN D32 PRO)"
```

## 🔍 Différences entre ESP8266 et ESP32

| Caractéristique | ESP8266 (D1 Mini) | ESP32 (LOLIN D32 PRO) |
|----------------|-------------------|----------------------|
| **CPU** | 80 MHz (1 cœur) | 240 MHz (2 cœurs) |
| **RAM** | 80 KB | 520 KB |
| **Flash** | 4 MB | 4 MB |
| **GPIO** | ~11 GPIO | ~30 GPIO |
| **SPI Max** | 27 MHz | 40 MHz |
| **WiFi** | 802.11 b/g/n | 802.11 b/g/n + Bluetooth |
| **Prix** | ~2-3€ | ~8-10€ |

### Avantages ESP32 pour ce projet
- ✅ Plus de RAM (pas de contrainte mémoire)
- ✅ Plus rapide (affichage plus fluide)
- ✅ Plus de GPIO (plus de possibilités d'extension)
- ✅ SPI plus rapide (refresh écran plus rapide)
- ✅ Bluetooth intégré (contrôle sans fil possible)

### Limitations ESP8266
- ⚠️ RAM limitée (80 KB) - pas de LVGL possible
- ⚠️ Un seul cœur (moins de multitâche)
- ⚠️ Moins de GPIO disponibles

## 📋 Configuration des pins par défaut

### ESP8266 (D1 Mini)
```cpp
LED_PIN = GPIO4 (D2)
TFT_MISO = GPIO12 (D6)
TFT_MOSI = GPIO13 (D7)
TFT_SCLK = GPIO14 (D5)
TFT_CS = GPIO16 (D0)
TFT_DC = GPIO15 (D8)
TOUCH_CS = GPIO0 (D3)
```

### ESP32 (LOLIN D32 PRO)
```cpp
LED_PIN = GPIO23 (à vérifier selon votre montage)
TFT_MISO = GPIO19 (VSPI MISO)
TFT_MOSI = GPIO23 (VSPI MOSI)
TFT_SCLK = GPIO18 (VSPI SCK)
TFT_CS = GPIO5
TFT_DC = GPIO2
TOUCH_CS = GPIO15
```

⚠️ **Note** : Ces pins ESP32 sont à adapter selon votre montage réel ! Consultez la documentation de votre shield TFT.

## 🧪 Tests après compilation

### Vérifications à faire :
1. **Écran** : L'interface s'affiche correctement
2. **Tactile** : Les boutons répondent aux touches
3. **LEDs** : Les LEDs s'allument avec la bonne couleur
4. **Console** : Vérifier le microcontrôleur détecté

### Exemple de sortie console attendue (ESP32)
```
=== Selfie Light Pro ===
Microcontrôleur: ESP32 (LOLIN D32 PRO)
LED Pin: GPIO23
Initialisation TFT...
Initialisation WS2812B...
Création de l'interface...
=== Système prêt ! ===
```

## 🐛 Dépannage

### Erreur "Platform not supported"
```
#error "Plateforme non supportée ! Utiliser ESP8266_BOARD ou ESP32_BOARD"
```
➜ **Solution** : Nettoyer le cache avec `pio run -t clean`

### L'écran reste noir (ESP32)
➜ **Solution** : Vérifier les pins SPI dans [platformio.ini](platformio.ini)

### Les LEDs ne s'allument pas (ESP32)
➜ **Solution** : Changer `LED_PIN` dans [platformio.ini](platformio.ini) selon votre montage

### Le tactile ne répond pas (ESP32)
➜ **Solution** : Vérifier `TOUCH_CS` et recalibrer avec `debugTouch = true`

## 📝 Contribution

Si vous testez avec un autre shield TFT ou un autre ESP32, merci de partager vos configurations de pins !

#!/bin/bash
# Script de compilation et upload multi-plateforme

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   ESP Light - Build & Upload Script   ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""

# Vérifier que PlatformIO est installé
if ! command -v pio &> /dev/null; then
    echo -e "${RED}❌ Erreur : PlatformIO n'est pas installé !${NC}"
    echo -e "${YELLOW}➜ Installation : pip install platformio${NC}"
    exit 1
fi

# Menu de sélection
echo -e "${YELLOW}Sélectionnez le microcontrôleur :${NC}"
echo "1) ESP8266 (Wemos D1 Mini)"
echo "2) ESP32 (WEMOS LOLIN D32 PRO)"
echo "3) ESP32 + LVGL"
echo "4) Les deux standards (ESP8266 + ESP32)"
echo ""
read -p "Votre choix [1-4] : " choice

# Fonction de build
build_platform() {
    local env=$1
    local name=$2
    
    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}🔨 Compilation pour ${name}${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Nettoyer le cache
    echo -e "${YELLOW}🧹 Nettoyage du cache...${NC}"
    pio run -e $env -t clean
    
    # Compiler
    echo -e "${YELLOW}⚙️  Compilation...${NC}"
    if pio run -e $env; then
        echo -e "${GREEN}✅ Compilation réussie !${NC}"
        
        # Demander si on doit uploader
        read -p "Uploader le firmware maintenant ? [O/n] : " upload
        if [[ $upload != "n" && $upload != "N" ]]; then
            echo -e "${YELLOW}📤 Upload en cours...${NC}"
            if pio run -e $env -t upload; then
                echo -e "${GREEN}✅ Upload réussi !${NC}"
                
                # Demander si on veut ouvrir le moniteur série
                read -p "Ouvrir le moniteur série ? [O/n] : " monitor
                if [[ $monitor != "n" && $monitor != "N" ]]; then
                    echo -e "${YELLOW}📡 Moniteur série (Ctrl+C pour quitter)${NC}"
                    pio device monitor -e $env
                fi
            else
                echo -e "${RED}❌ Erreur lors de l'upload !${NC}"
                return 1
            fi
        fi
    else
        echo -e "${RED}❌ Erreur lors de la compilation !${NC}"
        return 1
    fi
}

# Exécuter selon le choix
case $choice in
    1)
        build_platform "d1_mini" "ESP8266 (D1 Mini)"
        ;;
    2)
        echo -e "${YELLOW}⚠️  Vérifiez que le câble SH1.0 est bien connecté !${NC}"
        read -p "Continuer ? [O/n] : " continue
        if [[ $continue != "n" && $continue != "N" ]]; then
            build_platform "lolin_d32_pro" "ESP32 (LOLIN D32 PRO)"
        fi
        ;;
    3)
        echo -e "${YELLOW}⚠️  Test LVGL - Version expérimentale${NC}"
        echo -e "${YELLOW}⚠️  Nécessite ESP32 (520KB RAM) + câble SH1.0 bien connecté${NC}"
        read -p "Continuer ? [O/n] : " continue
        if [[ $continue != "n" && $continue != "N" ]]; then
            build_platform "lolin_d32_pro_lvgl" "ESP32 + LVGL (Expérimental)"
        fi
        ;;
    4)
        build_platform "d1_mini" "ESP8266 (D1 Mini)"
        echo ""
        echo -e "${YELLOW}⚠️  Changement de plateforme : ESP32${NC}"
        echo -e "${YELLOW}⚠️  Vérifiez que le câble SH1.0 (WEMOS TFT) est bien connecté !${NC}"
        read -p "Continuer avec ESP32 ? [O/n] : " continue
        if [[ $continue != "n" && $continue != "N" ]]; then
            build_platform "lolin_d32_pro" "ESP32 (LOLIN D32 PRO)"
        fi
        ;;
    *)
        echo -e "${RED}❌ Choix invalide !${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}🎉 Terminé !${NC}"

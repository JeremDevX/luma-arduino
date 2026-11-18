# Luma Arduino

Programme ESP32-C3 Mini pour une matrice RGB WS2812B de 32 × 8 (256 LED), avec simulateur OBD pour macOS.

## Installation

1. Ouvrir [LumaESP32.ino](LumaESP32/LumaESP32.ino) dans Arduino IDE. Installer la bibliothèque **Adafruit NeoPixel**, sélectionner **ESP32C3 Dev Module** et activer **USB CDC On Boot** pour la connexion USB native.
2. Relier **GPIO4** à **DIN**. Utiliser un convertisseur **74AHCT125** si la matrice exige un signal 5 V ou si sa version est inconnue. Alimenter l’ESP32 par USB-C et la matrice par son alimentation 5 V. Relier **V−** aux **GND** de l’ESP32 et de la matrice. Protéger le fil **+5 V** par un fusible adapté au courant et au câblage.
3. Téléverser le programme puis connecter la version WS2812B de Luma sur iPhone. À faible luminosité, afficher un L dans le coin supérieur gauche pour vérifier l’orientation.

Le programme suppose un zigzag par colonnes, avec la première LED en haut à gauche et un ordre de couleur GRB. Les réglages `MATRICE_PAR_COLONNES`, `DIN_A_DROITE` et `DIN_EN_BAS` sont en haut du programme.

Compilation ESP32-C3 et tests du moteur réussis. L’ordre des LED et le rendu restent à vérifier sur le montage.

## Simulateur OBD

Sur Mac, ouvrir `OBDSimulator/LancerSimulateur.command`. Sur le même réseau Wi-Fi, saisir dans Luma l’adresse indiquée par le simulateur et régler le régime dans le navigateur. Utiliser seulement un réseau local de confiance : la connexion OBD n’est pas authentifiée. Ce simulateur ne lit pas les données d’un véhicule.

[Application iOS Luma](https://github.com/JeremDevX/luma-ios)

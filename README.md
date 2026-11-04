# Luma Arduino

Firmware pour ESP32-C3 et quatre matrices MAX7219, avec simulateur OBD pour macOS.

## Installation

Ouvrir `LumaESP32/LumaESP32.ino` dans Arduino IDE, sélectionner la carte ESP32-C3 et téléverser. Branchements : DIN sur GPIO4, CS sur GPIO5 et CLK sur GPIO6. Relier les masses et vérifier les niveaux logiques.

## Simulateur OBD

Sur Mac, ouvrir `OBDSimulator/LancerSimulateur.command`. Sur le même réseau Wi-Fi, saisir dans Luma l’adresse indiquée par le simulateur et régler le régime dans le navigateur. Utiliser seulement un réseau local de confiance : la connexion OBD n’est pas authentifiée. Ce simulateur ne lit pas les données d’un véhicule.

Application iOS : https://github.com/JeremDevX/luma-ios

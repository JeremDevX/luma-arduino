#pragma once
#include <stdint.h>

constexpr int LARGEUR_MATRICE = 32;
constexpr int HAUTEUR_MATRICE = 8;

constexpr int indiceLED(int x, int y, bool parColonnes, bool dinADroite, bool dinEnBas) {
  if (dinADroite) x = LARGEUR_MATRICE - 1 - x;
  if (dinEnBas) y = HAUTEUR_MATRICE - 1 - y;
  const int bande = parColonnes ? x : y;
  const int position = parColonnes ? y : x;
  const int longueur = parColonnes ? HAUTEUR_MATRICE : LARGEUR_MATRICE;
  return bande * longueur + (bande % 2 ? longueur - 1 - position : position);
}

constexpr uint8_t canalLED(uint8_t canal, uint8_t luminosite) {
  return (canal * luminosite + 50) / 100;
}

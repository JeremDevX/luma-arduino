#pragma once

void masquerRevelation(byte image[8][4], unsigned long ecoule) {
  if (ecoule >= 800) return;
  int visibles = ecoule * 32 / 800;
  for (int y = 0; y < 8; y++) for (int x = visibles; x < 32; x++)
    image[y][x / 8] &= ~(0x80 >> (x % 8));
}

void composerPage(byte image[8][4], const byte colonnes[32]) {
  for (int ligne = 0; ligne < 8; ligne++) {
    for (int matrice = 0; matrice < 4; matrice++) {
      byte valeur = 0;
      for (int colonne = 0; colonne < 8; colonne++) {
        if (colonnes[matrice * 8 + colonne] & (0x80 >> ligne)) valeur |= 0x80 >> colonne;
      }
      image[ligne][matrice] = valeur;
    }
  }
}

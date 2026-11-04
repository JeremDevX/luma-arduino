#pragma once

bool composerSpectre(byte image[8][4], const byte *donnees) {
  for (int i = 0; i < 16; i++) {
    if ((donnees[i] >> 4) > 8 || (donnees[i] & 0x0f) > 8) return false;
  }
  memset(image, 0, 32);
  for (int x = 0; x < 32; x++) {
    int hauteur = x % 2 ? donnees[x / 2] & 0x0f : donnees[x / 2] >> 4;
    for (int y = 7; y >= 8 - hauteur; y--) image[y][x / 8] |= 0x80 >> (x % 8);
  }
  return true;
}

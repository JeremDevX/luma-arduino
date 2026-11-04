#pragma once

bool composerRegime(byte image[8][4], unsigned int regime, unsigned int maximum) {
  if (maximum < 1000 || maximum > 16000 || regime > 16383) return false;
  unsigned int colonnes = (min(regime, maximum) * 32UL) / maximum;
  memset(image, 0, 32);
  for (unsigned int x = 0; x < colonnes; x++) {
    for (int y = 0; y < 8; y++) image[y][x / 8] |= 0x80 >> (x % 8);
  }
  return true;
}

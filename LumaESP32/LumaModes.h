#pragma once
#include <time.h>

// Generated from the app pixel font, keeping the panel and preview consistent.
const char fontChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ:/ ";
const byte fontColumns[][5] = {
  {0x3e,0x45,0x49,0x51,0x3e}, // 0
  {0x0,0x21,0x7f,0x1,0x0}, // 1
  {0x21,0x43,0x45,0x49,0x31}, // 2
  {0x41,0x49,0x49,0x49,0x36}, // 3
  {0xc,0x14,0x24,0x7f,0x4}, // 4
  {0x79,0x49,0x49,0x49,0x46}, // 5
  {0x3e,0x49,0x49,0x49,0x6}, // 6
  {0x40,0x47,0x48,0x50,0x60}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x30,0x49,0x49,0x49,0x3e}, // 9
  {0x3f,0x48,0x48,0x48,0x3f}, // A
  {0x7f,0x49,0x49,0x49,0x36}, // B
  {0x3e,0x41,0x41,0x41,0x41}, // C
  {0x7f,0x41,0x41,0x41,0x3e}, // D
  {0x7f,0x49,0x49,0x49,0x41}, // E
  {0x7f,0x48,0x48,0x48,0x40}, // F
  {0x3e,0x41,0x49,0x49,0x2e}, // G
  {0x7f,0x8,0x8,0x8,0x7f}, // H
  {0x41,0x41,0x7f,0x41,0x41}, // I
  {0x2,0x1,0x41,0x7e,0x40}, // J
  {0x7f,0x8,0x14,0x22,0x41}, // K
  {0x7f,0x1,0x1,0x1,0x1}, // L
  {0x7f,0x20,0x18,0x20,0x7f}, // M
  {0x7f,0x20,0x10,0x8,0x7f}, // N
  {0x3e,0x41,0x41,0x41,0x3e}, // O
  {0x7f,0x48,0x48,0x48,0x30}, // P
  {0x3e,0x41,0x45,0x42,0x3d}, // Q
  {0x7f,0x48,0x4c,0x4a,0x31}, // R
  {0x31,0x49,0x49,0x49,0x46}, // S
  {0x40,0x40,0x7f,0x40,0x40}, // T
  {0x7e,0x1,0x1,0x1,0x7e}, // U
  {0x7c,0x2,0x1,0x2,0x7c}, // V
  {0x7f,0x2,0xc,0x2,0x7f}, // W
  {0x63,0x14,0x8,0x14,0x63}, // X
  {0x60,0x10,0xf,0x10,0x60}, // Y
  {0x43,0x45,0x49,0x51,0x61}, // Z
  {0x0,0x0,0x36,0x0,0x0}, // :
  {0x1,0x6,0x8,0x30,0x40}, // /
  {0x0,0x0,0x0,0x0,0x0}, //  
};

byte mode = 0;
bool clientConnecte = false, horlogeSynchronisee = false, enMarche = false;
unsigned long baseHeure = 0, millisHeure = 0, dernierTickMode = 0, derniereImage = 0;
unsigned long debutAnimation = 0;
unsigned long dureeMs = 0, tempsMs = 0;
bool pausePomodoro = false;
struct Programme { byte heure = 0, minute = 0; bool actif = false; char texte[17] = {}; int jour = -1; };
Programme programmes[2];
struct Goutte { int x, y, longueur; unsigned long dernier, cadence; };
Goutte gouttes[5];
bool pluieInitialisee = false;
char texteHorloge[8] = {};
unsigned long debutHorloge = 0;

void pixel(int x, int y) {
  if (x >= 0 && x < 32 && y >= 0 && y < 8) image[y][x / 8] |= (0x80 >> (x % 8));
}

void texteCourt(const char *texte) {
  memset(image, 0, sizeof(image));
  int count = strlen(texte);
  int left = max(0, (32 - (count * 6 - 1)) / 2);
  for (int n = 0; n < count; n++) {
    const char *pos = strchr(fontChars, texte[n]);
    if (!pos) continue;
    int glyph = pos - fontChars;
    for (int x = 0; x < 5; x++) for (int y = 0; y < 7; y++)
      if (fontColumns[glyph][x] & (1 << (6 - y))) pixel(left + n * 6 + x, y);
  }
}

void lancerMode(byte nouveau, unsigned long duree) {
  if (nouveau < 1 || nouveau > 9) return;
  mode = nouveau; defilement = false;
  if (nouveau == 7) pluieInitialisee = false;
  if (nouveau == 1) texteHorloge[0] = 0;
  dureeMs = duree * 1000UL; tempsMs = 0;
  pausePomodoro = false; enMarche = true;
  dernierTickMode = millis(); derniereImage = 0; debutAnimation = dernierTickMode;
}

void controlerMode(byte action) {
  if (mode < 2 || mode > 4) return;
  if (action == 0) enMarche = false;
  if (action == 1) { enMarche = true; dernierTickMode = millis(); }
  if (action == 2) { tempsMs = 0; pausePomodoro = false; enMarche = false; if (mode == 4) dureeMs = 1500000UL; }
}

void animation(byte laquelle, unsigned long maintenant) {
  memset(image, 0, sizeof(image));
  if (laquelle == 5) {
    const byte petit[] = {0x00,0x28,0x7c,0x7c,0x38,0x10,0x00,0x00};
    const byte grand[] = {0x66,0xff,0xff,0x7e,0x3c,0x18,0x00,0x00};
    unsigned long phase = maintenant % 1400;
    bool battement = phase < 180 || (phase >= 320 && phase < 500);
    const byte *coeur = battement ? grand : petit;
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++)
      if (coeur[y] & (0x80 >> x)) pixel(x + 12, y);
    if (battement) {
      pixel(9, 2); pixel(10, 3); pixel(9, 4);
      pixel(22, 2); pixel(21, 3); pixel(22, 4);
    }
  } else if (laquelle == 6) {
    const byte xs[] = {3, 9, 15, 22, 28};
    const byte ys[] = {2, 5, 1, 5, 2};
    for (int i = 0; i < 5; i++) {
      int phase = (maintenant / 250 + i * 2) % 8;
      pixel(xs[i], ys[i]);
      if (phase >= 2 && phase <= 5) {
        pixel(xs[i] - 1, ys[i]); pixel(xs[i] + 1, ys[i]);
        pixel(xs[i], ys[i] - 1); pixel(xs[i], ys[i] + 1);
      }
      if (phase == 3 || phase == 4) {
        pixel(xs[i] - 2, ys[i]); pixel(xs[i] + 2, ys[i]);
      }
    }
  } else if (laquelle == 7) {
    if (!pluieInitialisee) {
      for (int i = 0; i < 5; i++) {
        gouttes[i] = {1 + i * 6 + (int)(esp_random() % 4), (int)(esp_random() % 8),
                      2 + (int)(esp_random() % 2), maintenant, 100 + esp_random() % 100};
      }
      pluieInitialisee = true;
    }
    for (int i = 0; i < 5; i++) {
      Goutte &g = gouttes[i];
      if (maintenant - g.dernier >= g.cadence) {
        g.y += (maintenant - g.dernier) / g.cadence;
        g.dernier = maintenant;
        if (g.y - g.longueur >= 8) {
          g.x = 1 + i * 6 + esp_random() % 4;
          g.y = -(int)(esp_random() % 3);
          g.longueur = 2 + esp_random() % 2;
          g.cadence = 100 + esp_random() % 100;
        }
      }
      for (int j = 0; j < g.longueur; j++) pixel(g.x, g.y - j);
    }
  } else if (laquelle == 8) {
    int debut = (maintenant / 100) % 32;
    for (int x = 0; x <= 13; x++) pixel((debut + x) % 32, 3);
    for (int i = 1; i <= 3; i++) {
      pixel((debut + 13 - i) % 32, 3 - i);
      pixel((debut + 13 - i) % 32, 3 + i);
    }
    if ((maintenant / 300) % 2 == 0) {
      pixel((debut + 1) % 32, 2); pixel((debut + 1) % 32, 4);
    }
  } else {
    const char *surprises[] = {"SALUT", "BRAVO", "MERCI", "HELLO", "SUPER"};
    texteCourt(surprises[(maintenant / 5000) % 5]);
    int phase = maintenant % 5000;
    int visibles = phase < 800 ? phase * 32 / 800 :
                   phase >= 4200 ? (5000 - phase) * 32 / 800 : 32;
    for (int y = 0; y < 8; y++) for (int x = visibles; x < 32; x++)
      image[y][x / 8] &= ~(0x80 >> (x % 8));
  }
}

void demarrerProgramme(const char *texte) {
  int count = strlen(texte); int index = 0;
  for (int n=0;n<count;n++) {
    const char *p = strchr(fontChars, texte[n]);
    if (!p) continue;
    int glyph = p-fontChars;
    for (int x=0;x<5;x++) colonnes[index++] = fontColumns[glyph][x] << 1;
    if (n+1<count) colonnes[index++] = 0;
  }
  if (index == 0) return;
  if (audioActif) { audioEnPause = true; debutPauseAudio = millis(); }
  if (rpmActif) { rpmEnPause = true; debutPauseRPM = millis(); }
  largeur = index; intervalle = 50; position = 0; dernierPas = millis();
  defilement = true; mode = 0; modePages = false;
  revelationFixe = false; revelationDefilante = true;
  debutRevelation = millis(); dernierRenduRevelation = debutRevelation;
}

void avancerFonctions(unsigned long maintenant) {
  if (horlogeSynchronisee && clientConnecte) {
    time_t date = baseHeure + (unsigned long)(maintenant - millisHeure) / 1000;
    struct tm t; gmtime_r(&date, &t);
    for (int i=0;i<2;i++) if (programmes[i].actif && programmes[i].heure == t.tm_hour && programmes[i].minute == t.tm_min && programmes[i].jour != t.tm_yday) {
      programmes[i].jour = t.tm_yday; demarrerProgramme(programmes[i].texte);
    }
  }
  if (!mode) return;
  unsigned long elapsed = maintenant - dernierTickMode;
  dernierTickMode = maintenant;
  if (enMarche && mode >= 2 && mode <= 4) {
    if (mode == 2) tempsMs = min(tempsMs + elapsed, 5999000UL);
    else if (tempsMs + elapsed >= dureeMs) {
      if (mode == 4) { pausePomodoro = !pausePomodoro; dureeMs = pausePomodoro ? 300000UL : 1500000UL; tempsMs = 0; }
      else { tempsMs = dureeMs; enMarche = false; }
    } else tempsMs += elapsed;
  }
  if (maintenant - derniereImage < 100) return;
  derniereImage = maintenant;
  char texte[8];
  if (mode == 1) {
    if (!horlogeSynchronisee) { snprintf(texte, sizeof(texte), "HEURE"); }
    else {
      time_t date = baseHeure + (unsigned long)(maintenant - millisHeure) / 1000;
      struct tm t; gmtime_r(&date, &t);
      if ((maintenant / 5000) % 3 == 0) snprintf(texte,sizeof(texte),"%02d/%02d",t.tm_mday,t.tm_mon+1);
      else if ((maintenant / 5000) % 3 == 1) { const char *jours[] = {"DIM","LUN","MAR","MER","JEU","VEN","SAM"}; snprintf(texte,sizeof(texte),"%s",jours[t.tm_wday]); }
      else snprintf(texte,sizeof(texte),"%02d:%02d",t.tm_hour,t.tm_min);
    }
    if (strcmp(texteHorloge, texte) != 0) {
      strncpy(texteHorloge, texte, sizeof(texteHorloge));
      texteHorloge[sizeof(texteHorloge) - 1] = 0;
      debutHorloge = maintenant;
    }
    texteCourt(texte);
    masquerRevelation(image, maintenant - debutHorloge);
  } else if (mode <= 4) {
    unsigned long secondes = mode == 2 ? tempsMs / 1000 : (dureeMs - tempsMs + 999) / 1000;
    if (secondes > 5999) secondes = 5999;
    snprintf(texte,sizeof(texte),"%02lu:%02lu",secondes / 60,secondes % 60);
    texteCourt(texte);
    if (mode == 3 || mode == 4) {
      int bar = dureeMs ? 32 * (uint64_t)tempsMs / dureeMs : 0;
      for (int x=0;x<bar;x++) pixel(x,7);
      if (mode == 3 && tempsMs == dureeMs && (maintenant / 350) % 2 == 0) memset(image, 0, sizeof(image));
    }
  } else animation(mode, maintenant - debutAnimation);
  afficherImage();
}

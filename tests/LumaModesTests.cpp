#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
using std::min;
using std::max;
using byte = uint8_t;

byte image[8][4] = {};
byte colonnes[359] = {};
int largeur = 0, intervalle = 0, position = 0;
unsigned long fakeTime = 0, dernierPas = 0;
bool defilement = false, revelationFixe = false, revelationDefilante = false, modePages = false;
bool audioActif = false, audioEnPause = false, rpmActif = false, rpmEnPause = false;
unsigned long debutPauseRPM = 0;
unsigned long debutPauseAudio = 0;
unsigned long debutRevelation = 0, dernierRenduRevelation = 0;
uint32_t esp_random() { static uint32_t state = 35791; state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; }
unsigned long millis() { return fakeTime; }
void afficherImage() {}
#include "../LumaESP32/LumaTransition.h"
#include "../LumaESP32/LumaAudio.h"
#include "../LumaESP32/LumaRPM.h"
#include "../LumaESP32/LumaModes.h"

int main() {
  assert(composerRegime(image, 0, 6000));
  for (const auto &row : image) for (byte cell : row) assert(cell == 0);
  assert(composerRegime(image, 3000, 6000));
  for (const auto &row : image) assert(row[0] == 0xff && row[1] == 0xff && row[2] == 0 && row[3] == 0);
  assert(composerRegime(image, 7000, 6000));
  for (const auto &row : image) for (byte cell : row) assert(cell == 0xff);
  assert(!composerRegime(image, 1000, 0));
  assert(!composerRegime(image, 16384, 6000));
  for (const auto &row : image) for (byte cell : row) assert(cell == 0xff);

  byte spectre[16] = {};
  spectre[0] = 0x80; spectre[15] = 0x08;
  assert(composerSpectre(image, spectre));
  for (int y = 0; y < 8; y++) assert((image[y][0] & 0x80) != 0 && (image[y][3] & 0x01) != 0);
  assert((image[7][0] & 0x40) == 0);
  spectre[0] = 0x90;
  assert(!composerSpectre(image, spectre));

  byte first[8][4];
  for (byte effect = 5; effect <= 9; effect++) {
    animation(effect, 1000);
    std::memcpy(first, image, sizeof(image));
    bool any = false;
    for (const auto &row : image) for (byte cell : row) any |= cell != 0;
    assert(any);
    animation(effect, effect == 9 ? 6000 : 1400);
    assert(std::memcmp(first, image, sizeof(image)) != 0);
  }
  animation(5, 200);
  int small = 0; for (const auto &row : image) for (byte cell : row) small += __builtin_popcount(cell);
  animation(5, 400);
  int large = 0; for (const auto &row : image) for (byte cell : row) large += __builtin_popcount(cell);
  assert(small > 0 && large > small);
  pluieInitialisee = false;
  int premiersX[5] = {}; bool positionsChangees = false;
  for (unsigned long t = 0; t < 10000; t += 100) {
    animation(7, t);
    for (int i = 0; i < 5; i++) {
      if (t == 0) premiersX[i] = gouttes[i].x;
      else positionsChangees |= gouttes[i].x != premiersX[i];
      assert(gouttes[i].x >= 1 + i * 6 && gouttes[i].x <= 4 + i * 6);
      assert(gouttes[i].longueur >= 2 && gouttes[i].longueur <= 3);
      if (i > 0) assert(gouttes[i].x - gouttes[i-1].x >= 3);
    }
    for (int y = 0; y < 8; y++) {
      uint32_t row = 0;
      for (int x = 0; x < 32; x++) if (image[y][x/8] & (0x80 >> (x%8))) row |= uint32_t(1) << x;
      assert((row & (row << 1)) == 0);
    }
  }
  assert(positionsChangees);
  byte colonnesPage[32] = {};
  colonnesPage[0] = 0x80; colonnesPage[15] = 0x40; colonnesPage[31] = 0x01;
  composerPage(image, colonnesPage);
  assert(image[0][0] == 0x80 && image[1][1] == 0x01 && image[7][3] == 0x01);
  assert(image[0][1] == 0 && image[7][0] == 0);
  std::memset(image, 0xff, sizeof(image));
  masquerRevelation(image, 400);
  for (const auto &row : image) assert(row[0] == 0xff && row[1] == 0xff && row[2] == 0 && row[3] == 0);
  std::memset(image, 0xff, sizeof(image));
  masquerRevelation(image, 800);
  for (const auto &row : image) for (byte cell : row) assert(cell == 0xff);

  fakeTime = 20000; lancerMode(1, 0);
  avancerFonctions(fakeTime);
  for (const auto &row : image) for (byte cell : row) assert(cell == 0);
  fakeTime = 20400; avancerFonctions(fakeTime);
  int partial = 0; for (const auto &row : image) for (byte cell : row) partial += __builtin_popcount(cell);
  fakeTime = 20800; avancerFonctions(fakeTime);
  int full = 0; for (const auto &row : image) for (byte cell : row) full += __builtin_popcount(cell);
  assert(partial > 0 && full > partial);

  fakeTime = 11000; lancerMode(5, 0);
  animation(5, 200); std::memcpy(first, image, sizeof(image));
  fakeTime = 11200; avancerFonctions(fakeTime);
  assert(std::memcmp(first, image, sizeof(image)) == 0);
  fakeTime = 0;

  lancerMode(3, 60);
  assert(mode == 3 && dureeMs == 60000 && enMarche);
  fakeTime = 1200; avancerFonctions(fakeTime);
  assert(tempsMs == 1200);
  controlerMode(0); fakeTime = 5200; avancerFonctions(fakeTime);
  assert(tempsMs == 1200);
  controlerMode(1); fakeTime = 5300; avancerFonctions(fakeTime);
  assert(tempsMs == 1300);
  fakeTime = 65000; avancerFonctions(fakeTime);
  assert(tempsMs == 60000 && !enMarche);

  lancerMode(4, 1500);
  fakeTime += 1500000; avancerFonctions(fakeTime);
  assert(pausePomodoro && dureeMs == 300000);
  controlerMode(2);
  assert(!pausePomodoro && dureeMs == 1500000 && !enMarche);

  baseHeure = 1767254400UL; millisHeure = fakeTime; horlogeSynchronisee = true;
  programmes[0].actif = true; programmes[0].heure = 8; programmes[0].minute = 0;
  std::strcpy(programmes[0].texte, "BONJOUR");
  clientConnecte = false; avancerFonctions(fakeTime);
  assert(mode == 4);
  clientConnecte = true; avancerFonctions(fakeTime);
  assert(mode == 0 && defilement && largeur == 41 && programmes[0].jour >= 0);
}

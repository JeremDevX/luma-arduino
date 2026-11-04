#include <Arduino.h>
#include <esp_system.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

constexpr int DIN = 4;
constexpr int CS = 5;
constexpr int CLK = 6;
constexpr int MATRICES = 4;
constexpr int MAX_COLONNES = 960;

#define SERVICE_UUID "7A2F0007-CB86-4B6D-8F78-38D24B9E1940"
#define WRITE_UUID   "7A2F0002-CB86-4B6D-8F78-38D24B9E1940"

byte image[8][MATRICES];
byte imageRecue[8][MATRICES];
byte imageCible[8][MATRICES];
byte colonnes[MAX_COLONNES];
byte reception[MAX_COLONNES];
int largeur = 0, largeurAttendue = 0, recues = 0;
int intervalle = 100, intervalleRecu = 100, position = 0;
unsigned long dernierPas = 0;
bool defilement = false, premierePartieRecue = false;
bool modePages = false;
bool audioActif = false, audioEfface = true, audioEnPause = false;
bool rpmActif = false, rpmEfface = true, rpmEnPause = false;
unsigned long dernierRPM = 0, debutPauseRPM = 0;
unsigned long dernierAudio = 0, debutPauseAudio = 0;
int pagesAttendues = 0, nombrePages = 0;
unsigned long debutPages = 0, dernierRenduPages = 0;
bool prochaineRevelation = false, revelationFixe = false, revelationDefilante = false;
unsigned long debutRevelation = 0, dernierRenduRevelation = 0;
SemaphoreHandle_t verrou;

void envoyerAToutes(byte adresse, byte valeur) {
  digitalWrite(CS, LOW);
  for (int i = 0; i < MATRICES; i++) {
    shiftOut(DIN, CLK, MSBFIRST, adresse);
    shiftOut(DIN, CLK, MSBFIRST, valeur);
  }
  digitalWrite(CS, HIGH);
}

void afficherImage() {
  for (int ligne = 0; ligne < 8; ligne++) {
    digitalWrite(CS, LOW);
    for (int matrice = 0; matrice < MATRICES; matrice++) {
      shiftOut(DIN, CLK, MSBFIRST, ligne + 1);
      shiftOut(DIN, CLK, MSBFIRST, image[ligne][matrice]);
    }
    digitalWrite(CS, HIGH);
  }
}

#include "LumaTransition.h"
#include "LumaAudio.h"
#include "LumaRPM.h"

void afficherPages(unsigned long maintenant) {
  unsigned long duree = intervalle >= 100 ? 4000 : intervalle <= 20 ? 2400 : 3000;
  unsigned long ecoule = maintenant - debutPages;
  int page = (ecoule / duree) % nombrePages;
  unsigned long phase = ecoule % duree;
  composerPage(image, colonnes + page * 32);
  if (phase < 800) masquerRevelation(image, phase);
  else if (phase >= duree - 800) masquerRevelation(image, duree - phase);
  afficherImage();
}

void afficherDefilement() {
  int gauche = revelationDefilante ? 0 : 32 - position;
  for (int ligne = 0; ligne < 8; ligne++) {
    for (int matrice = 0; matrice < MATRICES; matrice++) {
      byte valeur = 0;
      for (int colonne = 0; colonne < 8; colonne++) {
        int source = matrice * 8 + colonne - gauche;
        if (source >= 0 && source < largeur && (colonnes[source] & (0x80 >> ligne))) {
          valeur |= 0x80 >> colonne;
        }
      }
      image[ligne][matrice] = valeur;
    }
  }
  if (revelationDefilante) {
    unsigned long ecoule = millis() - debutRevelation;
    masquerRevelation(image, ecoule);
    if (ecoule >= 800) { revelationDefilante = false; position = 32; }
  }
  afficherImage();
}

#include "LumaModes.h"

class Commandes : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String data = characteristic->getValue();
    if (data.length() == 0) return;
    xSemaphoreTake(verrou, portMAX_DELAY);
    byte commande = (byte)data[0];
    if (data.length() == 17 && (commande == 0x20 || commande == 0x21)) {
      int partie = commande - 0x20;
      if (partie == 0 || premierePartieRecue) {
        for (int i = 0; i < 16; i++) imageRecue[partie * 4 + i / MATRICES][i % MATRICES] = (byte)data[i + 1];
        if (partie == 0) premierePartieRecue = true;
        else {
          premierePartieRecue = false;
          defilement = false;
          audioActif = false;
          rpmActif = false; rpmEnPause = false;
          modePages = false;
          pagesAttendues = 0;
          mode = 0;
          revelationDefilante = false;
          memcpy(imageCible, imageRecue, sizeof(imageCible));
          memcpy(image, imageCible, sizeof(image));
          revelationFixe = prochaineRevelation;
          prochaineRevelation = false;
          if (revelationFixe) {
            debutRevelation = millis();
            dernierRenduRevelation = debutRevelation;
            masquerRevelation(image, 0);
          }
          afficherImage();
        }
      }
    } else if (data.length() == 2 && commande == 0x0A && (byte)data[1] <= 15) {
      envoyerAToutes(0x0A, (byte)data[1]);
      envoyerAToutes(0x0C, (byte)data[1] == 0 ? 0 : 1);
    } else if (data.length() == 4 && commande == 0x30) {
      int taille = (byte)data[1] | ((byte)data[2] << 8);
      largeurAttendue = taille > 0 && taille <= MAX_COLONNES && (byte)data[3] > 0 ? taille : 0;
      intervalleRecu = (byte)data[3];
      recues = 0;
    } else if (data.length() >= 4 && data.length() <= 20 && commande == 0x31) {
      int debut = (byte)data[1] | ((byte)data[2] << 8);
      int nombre = data.length() - 3;
      if (largeurAttendue && debut == recues && recues + nombre <= largeurAttendue) {
        for (int i = 0; i < nombre; i++) reception[recues++] = (byte)data[i + 3];
      } else largeurAttendue = 0;
    } else if (data.length() == 1 && commande == 0x32 && largeurAttendue && recues == largeurAttendue) {
      if (!pagesAttendues || largeurAttendue == pagesAttendues * 32) {
        memcpy(colonnes, reception, largeurAttendue);
        largeur = largeurAttendue;
        intervalle = intervalleRecu;
        position = 0;
        dernierPas = millis();
        mode = 0;
        audioActif = false;
        rpmActif = false; rpmEnPause = false;
        revelationFixe = false;
        if (pagesAttendues) {
          nombrePages = pagesAttendues;
          modePages = true;
          defilement = false;
          revelationDefilante = false;
          debutPages = millis();
          dernierRenduPages = debutPages;
          afficherPages(debutPages);
        } else {
          modePages = false;
          defilement = true;
          revelationDefilante = prochaineRevelation;
          debutRevelation = millis();
          dernierRenduRevelation = debutRevelation;
          afficherDefilement();
        }
      }
      largeurAttendue = 0;
      pagesAttendues = 0;
      prochaineRevelation = false;
    }
    else if (commande == 0x61 && data.length() == 2 && (byte)data[1] <= 1) {
      if ((byte)data[1] == 1) {
        rpmActif = true; rpmEnPause = false; rpmEfface = true; dernierRPM = millis();
        audioActif = false; audioEnPause = false;
        defilement = false; modePages = false; mode = 0;
        revelationFixe = false; revelationDefilante = false;
        memset(image, 0, sizeof(image)); afficherImage();
      } else if (rpmActif) {
        rpmActif = false;
        if (!rpmEnPause) { memset(image, 0, sizeof(image)); afficherImage(); }
        rpmEnPause = false;
      }
    } else if (commande == 0x60 && data.length() == 5 && rpmActif) {
      unsigned long maintenant = millis();
      if (!rpmEnPause || maintenant - debutPauseRPM >= 10000) {
        unsigned int regime = (byte)data[1] | ((unsigned int)(byte)data[2] << 8);
        unsigned int maximum = (byte)data[3] | ((unsigned int)(byte)data[4] << 8);
        byte prochaineImage[8][4];
        if (composerRegime(prochaineImage, regime, maximum)) {
          if (rpmEnPause) { rpmEnPause = false; defilement = false; modePages = false; mode = 0; }
          memcpy(image, prochaineImage, sizeof(image));
          rpmEfface = false; dernierRPM = maintenant;
          afficherImage();
        }
      }
    }
    else if (commande == 0x51 && data.length() == 2 && (byte)data[1] <= 1) {
      if ((byte)data[1] == 1) {
        audioActif = true; audioEnPause = false; audioEfface = true; dernierAudio = millis();
        rpmActif = false; rpmEnPause = false;
        defilement = false; modePages = false; mode = 0;
        revelationFixe = false; revelationDefilante = false;
        memset(image, 0, sizeof(image)); afficherImage();
      } else if (audioActif) {
        audioActif = false;
        if (!audioEnPause) { memset(image, 0, sizeof(image)); afficherImage(); }
        audioEnPause = false;
      }
    } else if (commande == 0x50 && data.length() == 17 && audioActif) {
      unsigned long maintenant = millis();
      if (!audioEnPause || maintenant - debutPauseAudio >= 10000) {
        byte prochaineImage[8][4];
        if (composerSpectre(prochaineImage, (const byte *)data.c_str() + 1)) {
          if (audioEnPause) { audioEnPause = false; defilement = false; modePages = false; mode = 0; }
          memcpy(image, prochaineImage, sizeof(image));
          audioEfface = false; dernierAudio = maintenant;
          afficherImage();
        }
      }
    }
    else if (commande == 0x46 && data.length() == 2 && (byte)data[1] <= 1) {
      prochaineRevelation = (byte)data[1] == 1;
    } else if (commande == 0x47 && data.length() == 2 && (byte)data[1] <= 30) {
      pagesAttendues = (byte)data[1];
    } else if (commande == 0x40 && data.length() == 6) {
      unsigned long duree = (byte)data[2] | ((unsigned long)(byte)data[3] << 8) |
                            ((unsigned long)(byte)data[4] << 16) | ((unsigned long)(byte)data[5] << 24);
      if ((byte)data[1] >= 1 && (byte)data[1] <= 9 && duree <= 5999 && (((byte)data[1] != 3 && (byte)data[1] != 4) || duree > 0)) {
        revelationFixe = false; revelationDefilante = false; prochaineRevelation = false;
        modePages = false; pagesAttendues = 0; audioActif = false; audioEnPause = false;
        rpmActif = false; rpmEnPause = false;
        lancerMode((byte)data[1], duree);
      }
    } else if (commande == 0x41 && data.length() == 5) {
      baseHeure = (byte)data[1] | ((unsigned long)(byte)data[2] << 8) |
                  ((unsigned long)(byte)data[3] << 16) | ((unsigned long)(byte)data[4] << 24);
      millisHeure = millis(); horlogeSynchronisee = true;
    } else if (commande == 0x42 && data.length() == 2 && (byte)data[1] <= 2) {
      controlerMode((byte)data[1]);
    } else if (commande == 0x44 && data.length() >= 4 && data.length() <= 20 && (byte)data[1] < 2 && (byte)data[2] < 24 && (byte)data[3] < 60) {
      int index = (byte)data[1]; bool valide = true;
      for (int i=4;i<data.length();i++) if (data[i] == 0 || !strchr(fontChars, data[i])) valide = false;
      if (valide) {
        programmes[index].heure = (byte)data[2]; programmes[index].minute = (byte)data[3];
        int longueur = data.length() - 4;
        for (int i=0;i<longueur;i++) programmes[index].texte[i] = data[i+4];
        programmes[index].texte[longueur] = 0;
        programmes[index].actif = longueur > 0; programmes[index].jour = -1;
      }
    }
    xSemaphoreGive(verrou);
  }
};

class Connexion : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    xSemaphoreTake(verrou, portMAX_DELAY);
    clientConnecte = true;
    xSemaphoreGive(verrou);
  }
  void onDisconnect(BLEServer *server) override {
    xSemaphoreTake(verrou, portMAX_DELAY);
    premierePartieRecue = false;
    largeurAttendue = 0;
    clientConnecte = false;
    if ((audioActif && !audioEnPause) || (rpmActif && !rpmEnPause)) { memset(image, 0, sizeof(image)); afficherImage(); }
    audioActif = false; audioEnPause = false; rpmActif = false; rpmEnPause = false;
    prochaineRevelation = false;
    pagesAttendues = 0;
    xSemaphoreGive(verrou);
    server->startAdvertising();
  }
};

void setup() {
  pinMode(DIN, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  digitalWrite(CLK, LOW);
  envoyerAToutes(0x0F, 0);
  envoyerAToutes(0x0C, 1);
  envoyerAToutes(0x09, 0);
  envoyerAToutes(0x0B, 7);
  envoyerAToutes(0x0A, 3);
  for (byte ligne = 1; ligne <= 8; ligne++) envoyerAToutes(ligne, 0);

  verrou = xSemaphoreCreateMutex();
  if (!verrou) return;
  BLEDevice::init("Luma ESP32-C3");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new Connexion());
  BLEService *service = server->createService(SERVICE_UUID);
  BLECharacteristic *commandes = service->createCharacteristic(WRITE_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  commandes->setCallbacks(new Commandes());
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();
}

void loop() {
  if (verrou) {
    xSemaphoreTake(verrou, portMAX_DELAY);
    unsigned long maintenant = millis();
    avancerFonctions(maintenant);
    if (rpmActif && !rpmEnPause && !rpmEfface && maintenant - dernierRPM >= 1500) {
      memset(image, 0, sizeof(image)); afficherImage(); rpmEfface = true;
    }
    if (audioActif && !audioEnPause && !audioEfface && maintenant - dernierAudio >= 1000) {
      memset(image, 0, sizeof(image)); afficherImage(); audioEfface = true;
    }
    if (modePages && maintenant - dernierRenduPages >= 40) {
      dernierRenduPages = maintenant;
      afficherPages(maintenant);
    }
    if (revelationFixe && maintenant - dernierRenduRevelation >= 40) {
      dernierRenduRevelation = maintenant;
      unsigned long ecoule = maintenant - debutRevelation;
      memcpy(image, imageCible, sizeof(image));
      masquerRevelation(image, ecoule);
      if (ecoule >= 800) revelationFixe = false;
      afficherImage();
    }
    if (defilement && revelationDefilante && maintenant - dernierRenduRevelation >= 40) {
      dernierRenduRevelation = maintenant;
      dernierPas = maintenant;
      afficherDefilement();
    } else if (defilement && !revelationDefilante && maintenant - dernierPas >= (unsigned long)intervalle) {
      dernierPas = maintenant;
      position = (position + 1) % (32 + largeur + 6);
      afficherDefilement();
    }
    xSemaphoreGive(verrou);
  }
  delay(1);
}

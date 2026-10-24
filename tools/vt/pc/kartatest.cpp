// SPDX-License-Identifier: GPL-3.0-or-later
// Provar kartans färg (v36): simulerad körning upp och ner över ett fält, ritar helskärmskartan.
#include <Arduino.h>
#include <Arduino_GFX.h>
#include <U8g2lib.h>
#include <map>
#include <string>
#include <vector>
#include <sys/stat.h>
struct SerialC { void printf(const char*f,...){va_list a;va_start(a,f);vprintf(f,a);va_end(a);} } Serial;
static bool sdOk=true; static void sdLock(){} static void sdUnlock(){}
#define MALLOC_CAP_SPIRAM 0
static void* heap_caps_malloc(size_t n,int){return malloc(n);}
static void* heap_caps_calloc(size_t n,size_t m,int){return calloc(n,m);}
static void heap_caps_free(void*p){free(p);}
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"
std::string SDROOT="sdk";
struct File { FILE*f=nullptr; operator bool() const {return f;} size_t read(uint8_t*b,size_t n){return fread(b,1,n,f);}
  size_t write(const uint8_t*b,size_t n){return fwrite(b,1,n,f);} void close(){if(f)fclose(f);f=nullptr;}
  int printf(const char*fm,...){va_list a;va_start(a,fm);int r=vfprintf(f,fm,a);va_end(a);return r;} };
struct SDC { bool exists(const char*p){struct stat st;return stat((SDROOT+p).c_str(),&st)==0;} void mkdir(const char*p){::mkdir((SDROOT+p).c_str(),0755);}
  void remove(const char*p){::remove((SDROOT+p).c_str());}
  File open(const char*p,const char*m="r"){File f; f.f=fopen((SDROOT+p).c_str(), m[0]=='w'?"wb":(m[0]=='a'?"ab":"rb")); return f;} } SD;
std::map<std::string,double> nvs;
struct Preferences { void begin(const char*,bool){} 
  uint8_t getUChar(const char*k,uint8_t d){auto i=nvs.find(k);return i==nvs.end()?d:i->second;} void putUChar(const char*k,uint8_t v){nvs[k]=v;}
  bool getBool(const char*k,bool d){auto i=nvs.find(k);return i==nvs.end()?d:i->second;} void putBool(const char*k,bool v){nvs[k]=v;}
  float getFloat(const char*k,float d){auto i=nvs.find(k);return i==nvs.end()?d:i->second;} void putFloat(const char*k,float v){nvs[k]=v;}
  double getDouble(const char*k,double d){auto i=nvs.find(k);return i==nvs.end()?d:i->second;} void putDouble(const char*k,double v){nvs[k]=v;}
  size_t getBytes(const char*,void*,size_t){return 0;} size_t putBytes(const char*,const void*,size_t n){return n;} };
static uint32_t T=0; static uint32_t millis_(){return T;}
#define millis millis_
static bool demoOn=false;
static char kartaStatus[200];
#define FW_VERSION 1
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x)
#define portEXIT_CRITICAL(x)
#include "ui.h"
#include "traktor.h"
#include "karta_pc.h"
class PcGfx : public Arduino_GFX { public: uint16_t fb[800*480]; PcGfx():Arduino_GFX(800,480){}
  bool begin(int32_t){return true;} void writePixelPreclipped(int16_t x,int16_t y,uint16_t c){fb[y*800+x]=c;} };
// Fältet: 18 drag à 300 m, 6 m bredd, norrut = uppför (mer bränsle), söderut nedför.
// En tung fläck mitt i fältet (lera): mer dragkraft och bränsle.
int main(int argc,char**argv){
  uiLang = getenv("EN") ? 1 : 0;
  ::mkdir("sdk",0755); ::mkdir("sdk/KARTA",0755); ::remove("sdk/KARTA/SPAR.BIN"); ::remove("sdk/KARTA/SPAR.ORG");
  bool omstart = argc>2;           // läs spåret från "SD" i stället (som efter omstart)
  if (omstart) { system("cp sdk_spar/* sdk/KARTA/"); }
  kartaBegin();
  uiTema(getenv("NATT")!=nullptr);
  double lat0=59.8160, lon0=17.6560, my=kMperDegLat(lat0), mx=kMperDegLon(lat0);
  if (!omstart) {
    kcfg.trig=KT_MANUELL; kcfg.autoW=false; kcfg.widthM=6;
    kartaStartLog();
    for (int drag=0; drag<18; drag++) {
      bool upp = drag%2==0; double x = drag*6.0;
      for (double s=0; s<=300; s+=0.6) {              // 8 km/h = 2,2 m/s, 0,6 m per ~0,27 s
        T += 270;
        double y = upp ? s : 300-s;
        bool lera = (x-50)*(x-50)+(y-150)*(y-150) < 40*40;
        float fuel = (upp ? 26 : 16) + (lera ? 6 : 0);
        float kn = (upp ? 30 : 24) + (lera ? 12 : 0);
        float sl = (upp ? 15 : 10) + (lera ? 12 : 0);   /* v41: slirning */
        trSet(trd.bransle, fuel, T); trSet(trd.kmhMark, 8, T); trSet(trd.dragKN, kn, T); trSet(trd.kmhHjul, 8 / (1 - sl / 100), T); trSlir(T);
        trd.riktning=1; trd.lat = lat0 + (y + (upp?3:-3))/my; trd.lon = lon0 + x/mx; trd.posMs = T;
        kartaLoop(T, NAN, -1);
      }
    }
    kartaStopLog();
    system("mkdir -p sdk_spar && cp sdk/KARTA/SPAR.* sdk_spar/");
  } else {
    for (int i=0;i<2000 && kReplaying;i++) { T+=10; kartaLoop(T, NAN, -1); }
    trd.lat = lat0+150/my; trd.lon=lon0+50/mx; trd.posMs=T;
  }
  printf("areal %.2f ha, rutor bränsle %u (snitt %.2f l/ha), drag %u (snitt %.1f kN), slir %u (snitt %.1f %%), block %d\n",
    kCells/10000.0, kfCnt[0], kfSnitt(0), kfCnt[1], kfSnitt(1), kfCnt[2], kfSnitt(2), kfNB);
  kartaFarg(getenv("FARG")?atoi(getenv("FARG")):0);   // FARG=1 dragkraft, FARG=2 slirning (v41)
  kartaBakgrund(getenv("BG")?1:0);
  UiState ui = {}; ui.page = PG_KARTA; ui.kartaMpp = 0.8f; ui.natt = getenv("NATT")!=nullptr; ui.busOk = true; ui.kartaGardin = getenv("GARDIN")!=nullptr;
  kartaFill(ui, T);
  trd.posMs = T; kFollow = true;
  uiKartaRender = kartaRender;
  PcGfx *g = new PcGfx(); g->setUTF8Print(true);
  uiDrawKarta(g, ui);
  FILE*f=fopen(argv[1],"wb"); fwrite(g->fb,2,800*480,f); fclose(f);
}

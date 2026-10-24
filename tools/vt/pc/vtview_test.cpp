// SPDX-License-Identifier: GPL-3.0-or-later
#include <Arduino.h>
#include <Arduino_GFX.h>
#include <U8g2lib.h>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
// ---- stubbar
struct SerialC { void printf(const char*f,...){va_list a;va_start(a,f);vprintf(f,a);va_end(a);} void println(const char*s){puts(s);} } Serial;
static bool sdOk=true; static void sdLock(){} static void sdUnlock(){}
#define MALLOC_CAP_SPIRAM 0
static void* heap_caps_malloc(size_t n,int){return malloc(n);}
static void* heap_caps_realloc(void*p,size_t n,int){return realloc(p,n);}
static uint32_t esp_rom_crc32_le(uint32_t c,const uint8_t*b,uint32_t n){c=~c;for(uint32_t i=0;i<n;i++){c^=b[i];for(int k=0;k<8;k++)c=(c>>1)^(0xEDB88320&-(c&1));}return ~c;}
#define FILE_READ "r"
#define FILE_WRITE "w"
std::string SDROOT="sd";
struct File { FILE*f=nullptr; std::string nm; bool dir=false; std::vector<std::string> ents; size_t ei=0;
  operator bool() const {return f||dir;} size_t size(){long p=ftell(f);fseek(f,0,SEEK_END);long s=ftell(f);fseek(f,p,SEEK_SET);return s;}
  size_t read(uint8_t*b,size_t n){return fread(b,1,n,f);} size_t write(const uint8_t*b,size_t n){return fwrite(b,1,n,f);}
  void close(){if(f)fclose(f);f=nullptr;} const char*name(){return nm.c_str();} bool isDirectory(){return dir;}
  File openNextFile(){File r; if(ei<ents.size()){r.nm=ents[ei++]; r.f=fopen((SDROOT+"/VTPOOL/"+r.nm).c_str(),"rb");} return r;} };
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>
struct SDC { bool exists(const char*p){struct stat st;return stat((SDROOT+p).c_str(),&st)==0;} void mkdir(const char*p){::mkdir((SDROOT+p).c_str(),0755);}
  File open(const char*p,const char*m="r"){File f; std::string fp=SDROOT+p; DIR*d=opendir(fp.c_str());
    if(d){f.dir=true; while(auto e=readdir(d)){ if(e->d_name[0]!='.') f.ents.push_back(e->d_name);} closedir(d); return f;}
    f.f=fopen(fp.c_str(), m[0]=='w'?"wb":"rb"); f.nm=p; return f;} } SD;
std::map<std::string,uint32_t> nvs;
struct Preferences { void begin(const char*,bool){} void end(){} uint32_t getUInt(const char*k,uint32_t d){auto i=nvs.find(k);return i==nvs.end()?d:i->second;} void putUInt(const char*k,uint32_t v){nvs[k]=v;} void remove(const char*k){nvs.erase(k);} };
static uint32_t T=0; static uint32_t millis_(){return T;}
#define millis millis_
#include "version.h"
struct BusDev{uint64_t name;uint32_t tcMs,vtMs;uint8_t known,flags;}; static BusDev busDev[256];
static int nmTillv(uint64_t n){return (n>>21)&0x7FF;} static int nmKlass(uint64_t n){return (n>>49)&0x7F;} static int nmFunk(uint64_t n){return (n>>40)&0xFF;}
struct {bool ok; char namn[33],ver[33];} ddop; static uint64_t ddopRawName=0;
struct RedskapInfo{int addr; bool nameKnown; uint64_t name;}; static RedskapInfo rdInfo={-1,false,0};
static void netMsg(const char*f,...){}
#include "vtpool.h"
#include "vtview.h"
#define TR_UI_ROWS 12
#include "ui.h"
#include "analys.h"
// ---- bildbuffert
class PcGfx : public Arduino_GFX { public: uint16_t fb[800*480]; PcGfx():Arduino_GFX(800,480){}
  bool begin(int32_t){return true;} void writePixelPreclipped(int16_t x,int16_t y,uint16_t c){fb[y*800+x]=c;} };
int main(int argc,char**argv){
  // Ritar en sida till en bild (800 x 480 RGB565). Val (miljövariabler):
  //   PAGE=<sidnummer> (se enum PG_* i ui.h), START=1 startbilden, EN=1 engelska, NATT=1 nattläge, DEMO=1, NOBUS=1,
  //   OFFER=<version> uppdateringsfrågan, OTA=1 uppdaterar, DLG=1 nolla kartan, SKARM=<namn>,
  //   INFO=0..1, RAW=1, BG=1, GARDIN=1, HEM=1, NOIMPL=1, EXP=1..3, NUM=1..3, KB2=1, SKIFT=1/k/c, TEMP, KRASCH=1,
  //   MAN=<kapitel> MANS=<sida>, UPPDAV=1, LOGGPA=1, DRAG, LUTF, LUTS, LJUS, FARG, SLIR.
  //   Argument: ut.raw [till_tid logg.LOG ...] – en candump-logg spelas upp för TERMINAL-sidan (valfritt).
  PcGfx *g=new PcGfx(); g->setUTF8Print(true);
  uiLang = getenv("EN") ? 1 : 0;
  vtpoolBegin(); vtviewBegin();
  uint64_t NAME=getenv("NAMN")?strtoull(getenv("NAMN"),0,16):0xA00C000000200001ULL; busDev[0x82].name=NAME; busDev[0x82].known=1;
  rdInfo={0x82,true,NAME};
  double until = argc>2? atof(argv[2]) : 1e9;
  for(int i=3;i<argc;i++){ std::ifstream f(argv[i]); std::string l;
    while(std::getline(f,l)){ double t; char id[16],dat[32]; if(sscanf(l.c_str(),"(%lf) can0 %8[0-9A-F]#%16[0-9A-F]",&t,id,dat)!=3)continue;
      if(t>until) break;
      T=(uint32_t)(t*1000); uint32_t I=strtoul(id,0,16); uint8_t sa=I&0xFF,da=(I>>8)&0xFF,pf=(I>>16)&0xFF; uint32_t pgn=((I>>24)&3)<<16|pf<<8; if(pf>=240)pgn|=da;
      uint8_t d[8]; for(int k=0;k<8;k++){char h[3]={dat[2*k],dat[2*k+1],0}; d[k]=strtoul(h,0,16);}
      vtpoolFrame(pgn,sa,da,d,8,T); vtpoolLoop(T); vtviewLoop(T); } }
  vtpoolLoop(T+4000); vtviewLoop(T+4000);
  static UiState ui; memset(&ui,0,sizeof(ui));
  ui.lang=uiLang;
  ui.busOk=true; ui.vtAvail=vvOk; ui.vtLive=true; strcpy(ui.genTitel,"Demo-spruta"); ui.valSecs=14;
  uiVtRender=[](Arduino_GFX*gg,int x,int y,int sz,int kx,int ky,int kw,int kh){ vtviewRender(gg,((PcGfx*)gg)->fb,x,y,sz,kx,ky,kw,kh); };
  ui.natt=getenv("NATT")!=nullptr; ui.trInfo=getenv("INFO")?atoi(getenv("INFO")):0; uiTema(ui.natt); ui.net.connected=true; const char* page=getenv("PAGE"); ui.page = page? atoi(page) : PG_TRAKTOR;
  for(int i=0;i<CHART_POINTS;i++) ui.trFuelMin[i]= i<8? NAN : 0.20f+0.12f*sinf(i*0.5f)+ (i>20?0.15f:0);
  ui.trv.kmh=8.4f; ui.trv.rpm=1450; ui.trv.fuel=22.6f; ui.trv.hours=4321.5f; ui.trv.liftF=12; ui.trv.liftR=62; ui.trv.workR=1; ui.trv.workF=0;
  ui.trv.ptoF=NAN; ui.trv.ptoR=1000; ui.trv.dir=1; ui.trv.nValve=5; int vn[5]={1,2,3,4,7}; float vf[5]={40,0,-30,0,0};
  for(int i=0;i<5;i++){ ui.trv.valveNo[i]=vn[i]; ui.trv.valveFlow[i]=vf[i]; }
  ui.karta.lPerHa=11.8f; ui.nTrRaw=3; strcpy(ui.trRaw[0],"F022 F0  01 00 FF FF FF FF FF FF"); strcpy(ui.trRaw[1],"FD02 F0  20 4E 00 00 FF FF FF FF"); strcpy(ui.trRaw[2],"FE40 F0  55 05 FF FF FF FF FF FF");
  ui.karta.gps=true; ui.karta.areaHa=3.42f;
  ui.trv.dragKN=getenv("DRAG")?atof(getenv("DRAG")):28; ui.trv.lutFram=getenv("LUTF")?atof(getenv("LUTF")):4; ui.trv.lutSida=getenv("LUTS")?atof(getenv("LUTS")):-6; ui.trv.ljus=getenv("LJUS")?strtoul(getenv("LJUS"),0,0):(LJ_POS|LJ_HALV|LJ_ARB_BAK); ui.trv.ljusOk=true;
  ui.karta.fargLage=getenv("FARG")?atoi(getenv("FARG")):0; ui.karta.fargHar=true; ui.karta.fargSnitt=ui.karta.fargLage==2?16.0f:(ui.karta.fargLage?24.0f:11.8f); ui.karta.fargSpann=0.3f*ui.karta.fargSnitt;
  ui.trv.slir=getenv("SLIR")?atof(getenv("SLIR")):17;
  ui.manKap=getenv("MAN")?atoi(getenv("MAN")):-1; ui.manSida=getenv("MANS")?atoi(getenv("MANS")):0; ui.manBack=PG_SETTINGS;
  if(ui.page==PG_ANALYS){ analysBegin(); srand(3);
    for(int t=0;t<1800;t++){ float k= t<200?0: (t<700?6.0f+ (rand()%10)/10.0f : t<1200? 8.0f+(rand()%10)/10.0f : 10.2f+(rand()%10)/10.0f);
      float rpm = (t%300<150)?1350:1750; float fuel = k<0.5f?6.0f: (k<7? 20.0f: k<9? 23.5f: 29.0f) + (rpm>1500?3.0f:0);
      bool vand = t>=200 && t%150>=130, transp = t>=1000 && t<1090;
      if (vand) { k = 4.0f; fuel = 14.0f; } if (transp) { k = 15.0f; fuel = 25.0f; }
      analysSample(k,fuel,rpm,0,12.0f,k>1 && !vand && !transp); }
    ui.anDiesel=23; ui.anTimKr=800; ui.anLage=getenv("LAGE")?atoi(getenv("LAGE")):1; ui.anPris=getenv("PRIS")!=nullptr;
    analysCompute(ui.an,ui.anDiesel,ui.anTimKr,ui.anLage); }
  ui.kmh=8.4f; ui.widthM=24; ui.widthFromBus=true;
  ui.version=FW_VERSION; strcpy(ui.net.devId,"fl-0a1b2c"); ui.net.sdOk=true; strcpy(ui.net.msg,"OK"); strcpy(ui.net.ssid,"Gardsnat"); ui.net.rssi=-60; ui.net.hasSaved=true;
  ui.net.uppd=getenv("UPPDAV")==nullptr; strcpy(ui.net.fbUrl,"https://github.com/statsminister1-blip/faltloggen/issues");
  ui.net.nScan=4; const char*ss[4]={"Gardsnat","Verkstad","Telefon","Maskinhall"}; for(int i=0;i<4;i++){strcpy(ui.net.scanSsid[i],ss[i]); ui.net.scanRssi[i]=-50-i*9; ui.net.scanSecure[i]=i!=3;}
  ui.brightness=80; ui.manualWidth=6; ui.tcSeen=true; ui.genHasImpl=true; strcpy(ui.genSub,uiLang?"sprayer · manufacturer 0 · address 130 · software 1.0":"spruta · tillverkare 0 · adress 130 · program 1.0");
  strcpy(ui.genWidth,uiLang?"24.00":"24,00"); strcpy(ui.genWork,L("PÅ","ON")); ui.genDdop=true;
  ui.nGen=12; for(int i=0;i<12;i++){ snprintf(ui.gen[i].namn,64,uiLang?"Value number %d from the element":"Värde nummer %d från elementet",i+1); snprintf(ui.gen[i].val,20,uiLang?"%d.%d":"%d,%d",i*7,i); strcpy(ui.gen[i].enh,i%2?"l/ha":"mm"); ui.gen[i].farsk=i%5!=4; }
  ui.loggAv=getenv("LOGGPA")==nullptr;
  strcpy(ui.kbSsid,"Verkstad"); strcpy(ui.kbText,"hemligt1"); ui.nRows=6; for(int i=0;i<6;i++){ ui.rows[i]={0x82,0xF7,(uint16_t)(i%4),(uint16_t)(i==0?2:141+i),3,(int32_t)(20000+i),(uint32_t)(100+i),0}; } ui.nowMs=2000; ui.frames=123456; ui.tcAddr=0xF7;
  strcpy(ui.karta.trigTxt,"ISOBUS auto"); strcpy(ui.karta.trigKort,"ISOBUS"); strcpy(ui.karta.startTxt,L("arbetar","working")); strcpy(ui.karta.stopTxt,L("slutar","stops")); ui.karta.trig=12; ui.karta.cfgAutoW=true; ui.karta.widthFromBus=true; ui.karta.widthM=24; ui.karta.cfgWidthM=6; ui.karta.logging=true; ui.karta.fuelL=48.2f; ui.karta.distKm=4.6f; strcpy(ui.karta.tileTxt,L("Satellitbild: alla 169 rutor finns","Satellite image: all 169 tiles present"));
  ui.kartaMpp=K_MPP_100M; ui.kartaConfirm=getenv("DLG")!=nullptr; ui.net.otaActive=getenv("OTA")!=nullptr; ui.net.otaPct=42; ui.net.latestVersion=getenv("OFFER")?atoi(getenv("OFFER")):FW_VERSION; ui.net.otaOffer=getenv("OFFER")?atoi(getenv("OFFER")):0; if(getenv("SKARM")) strncpy(ui.net.namn,getenv("SKARM"),32); ui.demo=getenv("DEMO")!=nullptr; if(getenv("NOBUS")) ui.busOk=false;
  ui.kartaGardin=getenv("GARDIN")!=nullptr; ui.karta.bakgrund=getenv("BG")?1:0; ui.valHem=getenv("HEM")!=nullptr; if(getenv("NOIMPL")){ui.genHasImpl=false; ui.vtAvail=false;} ui.trRawView=getenv("RAW")!=nullptr;
  if(getenv("EXP")){ ui.exp.status=atoi(getenv("EXP")); strcpy(ui.exp.namn,L("Norra åkern","North field")); strcpy(ui.exp.fil,"2026-10-22_2135_norra_akern.pdf"); strcpy(ui.exp.ap,"Faltloggen"); strcpy(ui.exp.fel,L("SD-kortet saknas.","No SD card.")); const char*R[]={L("3,29 ha bearbetat","3.29 ha worked"),L("Bränsle 14,8 l, varav 14,7 l i arbete","Fuel 14.8 l, of which 14.7 l working"),L("4,5 l/ha  ·  20,4 l/h i arbete","4.5 l/ha  ·  20.4 l/h working"),L("Tid på fältet 44 min, varav 43 min i arbete","Time in field 44 min, of which 43 min working"),L("Körsträcka 5,3 km  ·  snitthastighet 8,0 km/h","Distance 5.3 km  ·  average speed 8.0 km/h"),L("4,54 ha/h  ·  279 kr/ha","4.54 ha/h  ·  279 kr/ha")}; for(int i=0;i<6;i++) strcpy(ui.exp.rad[i],R[i]); ui.exp.nRad=6; ui.exp.stationer=1; ui.exp.hamtade=0; }
  if(getenv("SKIFT")){ ui.karta.skiftHa=12.5f; ui.karta.skiftKvarHa=3.2f; ui.karta.skiftMin=getenv("SKIFT")[0]=='k'?-1:95; strcpy(ui.karta.skiftKlass,"15:42"); ui.karta.skiftKlart=getenv("SKIFT")[0]=='c'; ui.karta.areaHa=9.3f; } else { ui.karta.skiftHa=0; }
  if(getenv("NUM")){ ui.numFor=atoi(getenv("NUM")); strcpy(ui.numText, getenv("NUM")[0]=='3' ? "35.65" : "1050"); } if(getenv("KB2")){ ui.kbFor=2; strcpy(ui.kbText,"Norra åkern"); }
  { EnhView &e=ui.enh; e.tempC=getenv("TEMP")?atof(getenv("TEMP")):48.6f; e.tempMin=31.2f; e.tempMax=e.tempC+2.3f; e.psFreeKB=4310; e.psTotKB=8192; e.psBigKB=3968; e.inFreeKB=118; e.inTotKB=352; e.inMinKB=74; e.inBigKB=60; e.sdKand=true; e.sdTotMB=30436; e.sdFriMB=22780; e.upSek=2*3600+13*60; e.bootNo=219; strcpy(e.omstart,getenv("KRASCH")?L("KRASCH (panic)","CRASH (panic)"):L("ström på","power on")); strcpy(e.part,"ota_1"); e.ramarS=203; e.ramarTot=1584211; e.canState=1; e.canMiss=0; e.canOverrun=0; e.canBusErr=2; e.ritN=1180; e.ritSnitt=96; e.ritMax=212; strcpy(ui.net.ip,"192.168.1.47"); }
  ui.dubbel=getenv("DUBBEL")!=nullptr; ui.dubbelFraga=getenv("DUBBEL") && getenv("DUBBEL")[0]=='2';
  if(getenv("START")) uiDrawSplash(g); else uiDraw(g,ui);
  FILE*o=fopen(argv[1],"wb"); fwrite(g->fb,2,800*480,o); fclose(o);
}
static const char *kBildText() { return L("Satellitbild: Sentinel-2 cloudless 2016 av EOX IT Services GmbH (CC BY 4.0)", "Satellite image: Sentinel-2 cloudless 2016 by EOX IT Services GmbH (CC BY 4.0)"); }

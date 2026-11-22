#include "Arduino.h"
Ser Serial; uint32_t g_ms=0; std::map<int,int> g_pin; std::string g_relayLog; std::string g_lcd[2];
int (*g_digitalRead)(int); uint32_t (*g_adc)();
#include "sketch.ino"

// scripted scenario
static int btn=1; // 1 = released (INPUT_PULLUP)
static uint32_t mv=300;
int rd(int p){ return p==32 ? btn : 0; }
uint32_t adc(){ return mv; }
static void run(uint32_t until){ while(g_ms<until){ g_ms+=5; loop(); } }
static void press(uint32_t holdMs){ btn=0; run(g_ms+holdMs); btn=1; run(g_ms+60); }
static int fails=0;
#define CHECK(c,msg) do{ if(!(c)){ printf("FAIL: %s\n",msg); fails++; } else printf("ok:   %s\n",msg);}while(0)
static bool unlocked(){ return g_pin[33]==1; }
int main(){
  g_digitalRead=rd; g_adc=adc;
  setup();
  CHECK(!unlocked(),"TC-01 relay locked at boot");
  CHECK(state==ST_WARMUP,"TC-01 starts in WARMUP");
  run(3000); press(100);
  CHECK(state==ST_WARMUP,"TC-02 button ignored during WARMUP");
  run(20500);
  CHECK(state==ST_READY && r0>0,"TC-03 calibrated and READY");
  printf("       R0=%.0f ohm (clean air 300 mV)\n", r0);
  // TC-04 clean air test
  press(100); CHECK(state==ST_SAMPLING,"press in READY starts SAMPLING");
  run(g_ms+5300);
  CHECK(state==ST_RESULT_SAFE && unlocked(),"TC-04 clean air -> SAFE, relay unlocked");
  printf("       LCD: [%s] [%s]\n", g_lcd[0].c_str(), g_lcd[1].c_str());
  run(g_ms+60500);
  CHECK(state==ST_READY && !unlocked(),"SAFE window ends -> READY, relay re-locked");
  // TC-05 low vapour: pot to 700 mV (AI ~2.7)
  press(100); mv=700; run(g_ms+5300); mv=300;
  CHECK(state==ST_RESULT_WARNING && !unlocked(),"TC-05 low vapour -> WARNING, relay locked");
  printf("       LCD: [%s] [%s] peak=%.2f\n", g_lcd[0].c_str(), g_lcd[1].c_str(), peakAI);
  press(100); CHECK(state==ST_RESULT_WARNING,"button ignored during cool-down");
  run(g_ms+30500); CHECK(state==ST_READY,"30 s cool-down -> READY");
  // TC-06 high vapour: 1100 mV (AI ~5)
  press(100); mv=1100; run(g_ms+5300); mv=300;
  CHECK(state==ST_RESULT_DANGER && !unlocked(),"TC-06 high vapour -> DANGER, relay locked");
  printf("       LCD: [%s] [%s] peak=%.2f\n", g_lcd[0].c_str(), g_lcd[1].c_str(), peakAI);
  CHECK(g_pin[25]==1 && g_pin[27]==1,"DANGER: red LED steady + continuous buzzer");
  run(g_ms+60500); CHECK(state==ST_READY,"60 s cool-down -> READY");
  // hysteresis: AI between 0.8*WARNING and WARNING after a WARNING stays WARNING
  Level l=classify(1.7f,LV_WARNING); CHECK(l==LV_WARNING,"hysteresis holds WARNING at AI 1.7");
  l=classify(1.5f,LV_WARNING); CHECK(l==LV_SAFE,"hysteresis releases WARNING at AI 1.5");
  l=classify(1.7f,LV_SAFE); CHECK(l==LV_SAFE,"no hysteresis when coming from SAFE");
  // long press -> recalibrate
  press(3200); CHECK(state==ST_CALIBRATING,"long press -> recalibrate");
  run(g_ms+10500); CHECK(state==ST_READY,"recalibration completes");
  // TC-07 fault: pin at rail
  mv=3300; run(g_ms+1300);
  CHECK(state==ST_FAULT && !unlocked(),"TC-07 pin at rail -> FAULT E1, relay locked");
  printf("       LCD: [%s] [%s]\n", g_lcd[0].c_str(), g_lcd[1].c_str());
  mv=300; run(g_ms+3000); press(100);
  CHECK(state==ST_FAULT,"FAULT latched until reset");
  printf("\nRelay transitions: %s\n", g_relayLog.c_str());
  printf("\nSerial excerpt:\n%.900s\n", Serial.out.c_str());
  printf("\n%s (%d failures)\n", fails? "SOME TESTS FAILED":"ALL TESTS PASSED", fails);
  return fails;
}

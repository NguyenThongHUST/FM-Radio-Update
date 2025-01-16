/*FM Radio : Lib: ESP8266Audio by Earle F. Philhower, III Lib: ArduinoTEA5767 by Simon Monk Lib: TaskScheduler by Anatoli Arkhipenko 3.8.5
// https://github.com/mathertel/Radio/tree/master
RDA5807
//https://github.com/pu2clr/RDA5807/tree/master

button wake up hw : button vagy a timer felebreszti a kartyat,
uj csatorna frekvenciajat es elinditja a radiot.
Utana minden 10 perc utan a timer felebeszti a kartyat,
// https://electronics.stackexchange.com/questions/404458/button-which-wakes-up-the-esp8266-from-deep-sleep-but-can-be-used-as-common-but


Touch valtja a FM csatornat:
ESP8266-Capacitive-Touch:
https://github.com/Manasmw01/ESP8266-Capacitive-Touch/blob/main/README.md
Problemak:
1.) touch button analog out-ot olvasni kell folyamatosan a loop() function-ben, tehat a programnak mindig futnia kell,
igy NEM teheto deepSleep-be a chip.
ESP.deepSleep(...)  funcio utan all minden, nem hivja meg loop() function-ont, setup()-ban beallit mindent es lemegy deep sleep-be.
ESP.deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR); // all a program.
A pelda kodban is latszik, RESET-re wake up, a timer wake up ki van kommnezve.Ketto egyutt nem megy.
Timer wake up:
 //ESP.deepSleep(30e6);
 Kivesszuk a comment-et //, ott megallna cihp es NEM FUTNA RA A reset wake up-ra
 ESP.deepSleep(0);
2.) buton csak ugy mukodhet, ha felebreszti a chip-et, de ahhoz plusy elektronika kell.
timer or button hw reset.
https://electronics.stackexchange.com/questions/404458/button-which-wakes-up-the-esp8266-from-deep-sleep-but-can-be-used-as-common-but
*/
/*
Meg tudnad ilyenre csinálni:
define egy FRPCS mint "FM frekvencia darabszám", amely 1 2 3 4 lehet,
es annyi FM frekvecia van define-ban megadva amiket sorban vált. Ez lehet egy vektor is, elválasztó jellel. Pl pontosvesszô lehet a frekvanciák között: 88.0;100.2;94.2;96.4;

Bekapcsoláskor mindig az 1-essel kezdi.

Ha a darabszám = 1, akkor nem használja az érintkezôt, így egyáltalán nem müködteti a frekvencia váltó interruptot, csak a 10 perceset.
ESP32 External Wake Up from Deep Sleep
https://randomnerdtutorials.com/esp32-external-wake-up-deep-sleep/
https://github.com/pcbreflux/espressif/blob/master/esp32/app/ESP32_deep_sleep_gpio_timer/main/deep_sleep_main.c
*/
/*
RTC Memory ESP8266 12-E read an write
https://github.com/esp8266/Arduino/blob/master/libraries/esp8266/examples/RTCUserMemory/RTCUserMemory.ino
*/

#include <string.h>
#include <Wire.h>
#include <Arduino.h>

#include <TEA5767Radio.h>

#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2SNoDAC.h"

#include "AudioFileSourceLittleFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
//#include "AudioOutputI2SNoDAC.h"

#include "memory.h"
#include "utils.h"

#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>

// Duy's define start
#include <AudioFileSourceHTTPStream.h>

#define WDT_RESET_PIN 2 // GPIO pin to toggle for debugging
#define WIFI_SSID "TMOBILE-xxxx"
#define WIFI_PASSWORD "pw"
String serverUrl = "http://104.238.164.12/FMfrequencies.txt";

// Timer interrupt function
void ICACHE_RAM_ATTR timer1ISR() {
  // Check if the system is stuck, and if so, trigger a reset
  if (!digitalRead(WDT_RESET_PIN)) {
    Serial.println("System stuck, resetting...");
    ESP.restart(); // Reset the ESP8266
  }
}
AudioFileSourceHTTPStream *fileHttp;
void fileOpearation();
void startWifiRadio();
void startMP3Streaming();
// Duy's define end

extern "C"
{
#include <user_interface.h>
}

#define POWER_RESET 1 
#define DEEP_SLEEP_RESET 2
#define EXTERNAL_RESET 3
#define EXCEPTION_RESET 4
#define NONE_RESET 5

///***************  DEEP_SLEEP_TIME constante begin
#define DEEP_SLEEP_TIME_DEFAULT 1000000*60*10  // 10 min
#define DEEP_SLEEP_TIME1 1000000*5    // 5 sec.
#define DEEP_SLEEP_TIME2 1000000*10   // 10 sec.
///***************  DEEP_SLEEP_TIME constante end


#define TIMER_INTERVAL_5sec (1000 * 1000) // 1.0 sec
#define uS_TO_S_FACTOR 1000000LL          /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 20                  /* Time ESP32 will go to sleep (in seconds), 20, later 10*60 secunds = 10 mins */
// #define TIME_TO_SLEEP 600      /* Time ESP32 will go to sleep (in seconds), 600 = 10 min */
#define MAX_DELAY_RDS 80 // 40ms - polling method
#define MAX_DELAY_STATUS 3000
#define EEPROM_SIZE 1 // Size needed to store our station index

bool FMenabled = true;
bool WIFIenabled = false;

String prodVersion = "";
//RADIO_FREQ* pRadios_array = NULL;
std::vector<FMRADIO> radios_vector;

int radios_array_length = 0;
int stationIndex = 0;
int stationStartIndex = 0;
int resetReason = POWER_RESET;

unsigned long status_elapsed = millis();

TEA5767Radio* radio;
//char bufferAux[512];
char timeBuffer[48];

WFILESTREAM* wfilestream;

const int BUTTONEXT_PIN = 12; // PIN6 read
const int BUTTONPOW_PIN = 14; // PIN 5 read
int powinput = 0;

int inputVal = 0;
bool buttonExtState = 0;

AudioGeneratorWAV *wav;
AudioFileSourcePROGMEM *file;
AudioOutputI2SNoDAC *out;

AudioGeneratorMP3 *mp3;
AudioFileSourceLittleFS *filemp3;
AudioFileSourceID3 *id3;

// Battery
ADC_MODE(ADC_VCC);
// Constants for voltage measurement
//const float batteryMinVoltage = 2.6f ;//5.0f; //2.6f ;     // Minimum operational voltage for your system.
uint64_t currentDeepTime = DEEP_SLEEP_TIME2; // 10 sec
float batteryMinVoltageHight =  2.8f; // bvolt_1; 
int deepTimeHight = 2; //deepTime_1, min; 
float batteryMinVoltageMidle =  2.6f; // bvolt_2;
int deepTimeMidle = 2; //deepTime_2. min
float batteryMinVoltageLow =  2.5f; // bvolt_3;
int deepTimeLow = 2; //deepTime_3. min

/* KESOBB
WFILESTREAM* pWifiStation_array = NULL;
int wifi_array_length = 0;
int wifistationIndex = 0;
int wifistationStartIndex = 0;
KESOBB */

static void setFrequency(float radioFreq);
static void playAudio(const uint8_t *audioData, unsigned int audioSize);
//static void startPlayback(const char *trackName);
static void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);

//--------------- setup ----------------
void setup()
{
    pinMode(BUTTONEXT_PIN, INPUT_PULLUP); // GPIO12 Button read
    //pinMode(BUTTONPOW_PIN, INPUT_PULLUP); // GPIO14 Button read
    pinMode(BUTTONPOW_PIN, INPUT); // GPIO14 External pull-up resistor needed
  
   
  // SzL FROM HERE innen 
    powinput = digitalRead(BUTTONPOW_PIN);

    int _count = 0;
    int _plus = 0;
    int _minus = 0;
    currentDeepTime = DEEP_SLEEP_TIME2;
    //pRadios_array = NULL;
    /* JESOBB
    pWifiStation_array = NULL;
    */
    radios_vector.clear();
    
    while (_count < 12)
    {
        if (digitalRead(BUTTONEXT_PIN) > 0)
        {
            _plus++;
        }
        else
        {
            _minus++;
        }
        _count++;
        delay(5); //(50);
    }
    buttonExtState = (_plus >= _minus) ? 1 : 0;

    Serial.begin(115200);
    delay(100);
    while (!Serial)
        delay(20);
    ;
    delay(2); //200 SzL
    Serial.println("\n---");
    delay(100); //1000 SzL
    //Serial.begin(115200);
    //delay(300);
    
    Serial.printf("\nPOWER input PIN state: "); // GPIO14 PIN 5
    Serial.println(powinput); // Print the button state (1 or 0)
//return;
    if (powinput == 0)
     {
        Serial.println("Power on");
        resetReason = POWER_RESET;
     }
    if (powinput == 1)
     {
        Serial.println("Wake from Sleep");  
        resetReason = DEEP_SLEEP_RESET;
     }    
     if (buttonExtState == 0)
     {
       Serial.println("Button pressed");
        resetReason = EXTERNAL_RESET;
     } 

    delay(100);
    // SzL END eddig

    
    // SzL form here Dec 04 2024
    Serial.printf("RED Button state: ");
    Serial.println(buttonExtState); // Print the button state (1 or 0)
    //SzL to here Dec 04 2024
//This is line 227 Duy's part beginning here

  static int wcount = 10;
  if(resetReason == POWER_RESET) {
    while (wcount >= 0)
    {
      if(digitalRead(BUTTONEXT_PIN) > 0) 
      {
        // run Wifi Radio  
        startWifiRadio();
        wcount = -1;
        break;
      } 
      else 
      {
        wcount--;
        if(wcount < 0) {
          wcount = 10;
          // Attempt to connect to WiFi
          // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

          // int attempts = 0;
          // const int maxAttempts = 8; // Maximum number of attempts before triggering watchdog reset

          // while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
          //   delay(2000);
          //   Serial.print("Attempting to connect to WiFi, attempt ");
          //   Serial.println(attempts + 1);
          //   attempts++;
          // }

          // if (WiFi.status() == WL_CONNECTED) {
          //   Serial.println("WiFi connected!");
          // } else {
          //   Serial.println("Failed to connect to WiFi");
          // }
          break;
        }
      }
      delay(100);
    }
    
    while (wcount >= 0)
    {
      if(digitalRead(BUTTONEXT_PIN) > 0) 
      {
        // run connect wifi  
        wcount = 0;
        if(wcount < 0) {
          wcount = 10;
          // Attempt to connect to WiFi
          WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

          int attempts = 0;
          const int maxAttempts = 8; // Maximum number of attempts before triggering watchdog reset

          while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
            delay(2000);
            Serial.print("Attempting to connect to WiFi, attempt ");
            Serial.println(attempts + 1);
            attempts++;
          }

          if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WiFi connected!");
            startMP3Streaming();
          } else {
            Serial.println("Failed to connect to WiFi");
          }
          break;
        }
        break;
      } 
      else 
      {
        wcount--;
        if(wcount < 0){
          // play file operation
          fileOpearation();
          break;
        }
      }
      delay(100);
    }
    
  }


//Duy's part ended here

    delay(50);
    pinMode(4, INPUT_PULLUP); // SDA
    pinMode(5, INPUT_PULLUP); // SCL
    radio = new TEA5767Radio();

    /*Serial.begin(115200);
    delay(100);
    while (!Serial)
        delay(20);
    ;
    delay(2); //200 SzL
    Serial.println("\n---");
    delay(100); //1000 SzL
    */

    if (!parserProdnumTxt("/Prodnum.txt", &prodVersion)){
        Serial.println("FATAL: parserProdnumTxt");          
        return;
    }
    Serial.printf("Product VERSION: %s\n", prodVersion.c_str());   
    delay(100);


    if (!parserFMFrequenciesTxtVector("/FMfrequencies.txt",  &radios_vector, &FMenabled, &WIFIenabled, &stationStartIndex))
    {
        Serial.println("FATAL: parserFMFrequenciesTxt");
        startPlayback("/Hiba.mp3"); 
        return;
    }    
    if (FMenabled)
    {
      radios_array_length = radios_vector.size();
      Serial.printf("radios_array_length: %d\n", radios_array_length); 
      Serial.println("FM Radio Active !!!!!!");
    }    
    if (WIFIenabled)
    {
      radios_array_length = 0;
      Serial.println("Wifi Stream Active !!!!!");
    }

    WiFi.mode(WIFI_OFF);

    // LittleFS Start
  /*  if (!LittleFS.begin())
    {
        Serial.println("FATAL:error opening LittleFS!");
        LittleFS.end();
        //startPlayback("/Hiba.mp3");
        return;
    }   
    listDir("/");
    LittleFS.end();  
    */  
    // LittleFS End
      // Initialize the Radio
  

// Read battery voltage
    if (!parserParametersTxt("/Parameters.txt", &batteryMinVoltageHight, &deepTimeHight, &batteryMinVoltageMidle, &deepTimeMidle, &batteryMinVoltageLow, &deepTimeLow)) {
        Serial.println("FATAL: parserParametersTxt");          
        return;
    }
    /*
    Serial.printf("batteryMinVoltageHight %f V, deepTimeHight %d\n",batteryMinVoltageHight, deepTimeHight);
    Serial.printf("batteryMinVoltageMidle %f V, deepTimeMidle %d\n",batteryMinVoltageMidle, deepTimeMidle);
    Serial.printf("batteryMinVoltageLow %f V, deepTimeLow %d\n",batteryMinVoltageLow, deepTimeLow);
    */
    float _batteryVoltage = ESP.getVcc() / 1024.0; // Get Vcc in volts
    int _deepTime = 0; // min
    if (_batteryVoltage > batteryMinVoltageHight) {
       currentDeepTime = 1000000*60*deepTimeHight;
      _deepTime=deepTimeHight;      
    }
    else
    if ((_batteryVoltage < batteryMinVoltageHight) && (_batteryVoltage > batteryMinVoltageMidle)) {
      currentDeepTime = 1000000*60*deepTimeMidle;
      _deepTime=deepTimeMidle;
      Serial.println("Battery voltage is low. Consider recharging.");        
      startPlayback("/Elemcsere.mp3");
    }    
    else
    if (_batteryVoltage < batteryMinVoltageLow) {    
        currentDeepTime = 1000000*60*deepTimeLow;
        _deepTime=deepTimeLow;
        Serial.println("Battery voltage is low. Consider recharging.");        
        startPlayback("/Elemcsere.mp3"); // LittleFS-bol
    }
    Serial.printf("Current voltage: %fV, current deep time %d min.\n",_batteryVoltage, _deepTime); //delay(300); // SzL Dec 14 2024

  if (FMenabled)
  {          
      if (resetReason == POWER_RESET)
      {        
        for (int ii=0;ii<radios_array_length;ii++)
        {
            Serial.printf(" radios_vector: index=%d, freq=%d, \n", ii,  radios_vector[ii].freq); 
            //Serial.printf( "RADIO CSATORNA: stationIndex= %d, Radio Number= %d, ii, Mp3= %s, Megjegyzes= %s \n", stationIndex, radios_vector[ii].freq, radios_vector[ii].nameMp3.c_str(), radios_vector[ii].comment.c_str());       
        }
        Serial.printf("stationStartIndex=%d\n", stationStartIndex);
      } 
      stationStartIndex--;

      if  ((resetReason == POWER_RESET) || (resetReason == EXTERNAL_RESET))
      {
      // Set FM Options for Europe
        /*radio->setup(RADIO_FMSPACING, RADIO_FMSPACING_100);  // for EUROPE
        radio->setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50); // for EUROPE     
        radio->AudioOutputHighZ(true);
        radio->setMono(true);
        radio->setMute(false);
        radio->setVolume(14); */
      } 
  }
  if (WIFIenabled)
  {
  
  }       
  
  if  (resetReason == POWER_RESET)
  { // Power
      Serial.println("Power On"); //SzL           
      startPlayback("/Bekapcsolas.mp3"); 
      clearMemory();
      setdefaultMemory(); 
     
      if (FMenabled)
      {
        if ((stationStartIndex>=0) && (stationStartIndex<=radios_array_length))
        {
            stationIndex = stationStartIndex;
        }
        else
        {
          stationIndex = 0;
          stationStartIndex=0;
        }
        setvaluetMemory((stationStartIndex), POINTER);
        printMemory();
      }
      if (WIFIenabled)
      {
      }
  }
  else if (resetReason == DEEP_SLEEP_RESET)
  { 
    // Deep-Sleep Wake
      Serial.println("Deep-Sleep Wake");
      // getMemory();
      // printMemory();
      if (FMenabled)
      {        
        byte temp = getvalueMemory(POINTER);
      //  printMemory();
        if (temp == 0xFF)
        {
            Serial.println("FATAL:Deep-Sleep Wake");
            startPlayback("/Hiba.mp3");
            return;
        }
        stationIndex = (int)temp;
        if ((stationIndex >= (int)radios_array_length))
        {
          stationIndex = 0; 
          setvaluetMemory((byte)(stationIndex), POINTER);
         // printMemory();
        }  
        return;    // nincs radio frekvenciseting, a setup() KILEP.
      } 
      if (WIFIenabled)
      {
      }
  }
  else if (resetReason == EXTERNAL_RESET)
  {
      // External System
      Serial.println("External System Reset");     
      if (FMenabled)
      { 
        byte temp = getvalueMemory(POINTER);
        if (temp == 0xFF)
        {
            Serial.println("FATAL1:getvalueMemory(POINTER)");
            startPlayback("/Hiba.mp3");            
            return;
        }       
        int _stationTempIndex = (int)temp;                    
        if (_stationTempIndex >= radios_array_length)
        {
        _stationTempIndex = 0;
          Serial.println("FATAL1: if (_stationTempIndex >= radios_array_length)");
          startPlayback("/Hiba.mp3");
        }
        else 
        {
          _stationTempIndex = (int)temp;
        }
        _stationTempIndex++;
        stationIndex = (_stationTempIndex >= (int)radios_array_length) ? 0 : _stationTempIndex;       
        setvaluetMemory((byte)(_stationTempIndex), POINTER);
        printMemory();
      }
      if (WIFIenabled)
      {
      }
  } 

  if (FMenabled)
  { 
    if (radios_array_length > 0)
    {       
      if (radios_vector[stationIndex].freq > 0) {
        Serial.printf( "RADIO CSATORNA: stationIndex= %d, Radio Number= %d, Mp3= %s, \n", stationIndex, radios_vector[stationIndex].freq, radios_vector[stationIndex].nameMp3.c_str());       
        startPlayback( radios_vector[stationIndex].nameMp3.c_str());
        if (radios_vector[stationIndex].comment.length() > 0) {
          Serial.printf( "RADIO CSATORNA:  Megjegyzes= %s\n", radios_vector[stationIndex].comment.c_str());    
        }   
        setFrequency( radios_vector[stationIndex].freq);
        if (resetReason == POWER_RESET)
        {
          // radio POWER ON -kor var 1 masodpercet (hogy a bekapcsolo bombot fel tudjak erositeni)
          delay(1000);
          startPlayback("/FM_radio.mp3");
          delay(300);
          startPlayback(radios_vector[stationIndex].nameMp3.c_str());
        }
      }
    }
  }
  if (WIFIenabled)
  {
  }
}

//--------------- loop ----------------
void loop()
{   
    //return;
    delay(500);

    Serial.printf("Going into deepsleep. time: %s\n", millisecToHoursMinutesSecuntums(timeBuffer, millis()));
    delay(500); // SzL delay(1000);
    Serial.end();
    //return;
    if (FMenabled)
    { 
      if (out != NULL)
      {
          delete out;
          out = NULL;
      }

      radios_vector.clear();
      /* KESOBB
      if (pWifiStation_array != NULL)
      {
        
        if (pWifiStation_array->nameMp3!= NULL)
        {
          delete pWifiStation_array->nameMp3;
        }
        if (pWifiStation_array->hostName!= NULL)
        {
          delete pWifiStation_array->hostName;
        }
        delete pWifiStation_array;
      } 
      KESOBB */
    }
    if (WIFIenabled)
    {
    }
    /*
    while(1)
    {
      startPlayback("/Bekapcsolas.mp3");
      delay(3000);
      startPlayback("/Hiba.mp3");
      delay(3000);
      startPlayback("/Elemcsere.mp3");
      delay(3000);
    }*/
    //ESP.deepSleep(DEEP_SLEEP_TIME1); //  5  sec
    ESP.deepSleep(DEEP_SLEEP_TIME2); //  10  sec
    // ESP.deepSleep(currentDeepTime); //  10  min   
}

//---------------------------------
void setFrequency(float radioFreq)
{
    Wire.begin();
    delay(200);
    radio->setFrequency(radioFreq);    
   // radio->setBandFrequency(RADIO_BAND_FM, radioFreq);     
    delay(100);
    radio->setFrequency(radioFreq);
}

// audio.png
// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
    (void)cbData;
    //Serial.printf("ID3 callback for: type %s\n", type);

    if (isUnicode)
    {
        string += 2;
    }

    while (*string)
    {
        char a = *(string++);
        if (isUnicode)
        {
            string++;
        }
        Serial.printf("%c", a);
    }
    Serial.printf("'\n");
    Serial.flush();
}

void startPlayback(const char *trackName)
{
  if (! existFile(trackName)) {    
    //Serial.printf("FATAL: Failed to open file: %s\n", trackName);
    return;
  }  
    Serial.printf("Playing file: %s\n", trackName);

    //radio->AudioOutputHighZ(true);
    filemp3 = new AudioFileSourceLittleFS(trackName);
    id3 = new AudioFileSourceID3(filemp3);
    id3->RegisterMetadataCB(MDCallback, (void *)"ID3TAG");
    out = new AudioOutputI2SNoDAC();
    mp3 = new AudioGeneratorMP3();
    mp3->begin(id3, out);

    while (1)
    {
        if (mp3->isRunning())
        {
            if (!mp3->loop())
                mp3->stop();
        }
        else
        {
            Serial.printf("MP3 done\n");
            delay(1000);
          if (filemp3 != NULL)
            {
                delete filemp3;
                filemp3 = NULL;
            }
            if (mp3 != NULL)
            {
                delete mp3;
                mp3 = NULL;
            }
            if (out != NULL)
            {
                delete out;
                out = NULL;              
            }     
            if (id3 != NULL)
            {
                delete id3;
                id3 = NULL;              
            }     
            pinMode(3, INPUT);                
            delay(10);                           
            break;            
        }
    }
  //  radio->AudioOutputHighZ(false);
    delay(100);
}
/*
FMfrequencies.txt
Parameters.txt
Parameters.txt
Prodnum.txt fajlok feldolgozva, parser-ezve.
line 568-570
    //ESP.deepSleep(DEEP_SLEEP_TIME1); //  5  sec
    ESP.deepSleep(DEEP_SLEEP_TIME2); //  10  sec
    // ESP.deepSleep(currentDeepTime); //  10  min  

Amikor élesben működik, akkor 570. sor a currentDeepTime változóval működik, ami a Parameters.txt fájból kapjuk.
Tesztelni az 10 sec-es sorral kell, most úgy fog működni. Nincs idő 20 percig várni.
FMfrequencies.txt fajl átalakult, a '// commentek' sorokban letírtam, amik nem hatnak a műk0désre.
Sokat változott a FM_V10.ino
Pár darab hang mp3 fájt készítettem, másképpen nem lenne teljes a tesztelés
*/


// Duy's start code
void fileOpearation() {
  Serial.println("------------- Uploading/replacing LittleFS mode ----------\n");
  static int attempts = 0;
  const int maxAttempts = 5; // Maximum number of attempts before triggering watchdog reset
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(2000);
    Serial.print("Attempting to connect to WiFi, attempt ");
    Serial.println(attempts + 1);
    attempts++;
  }
  
  if(WiFi.status()== WL_CONNECTED){
      attempts = 0;
      WiFiClient client;
      HTTPClient http;
      
      http.begin(client, serverUrl);
        
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        writeFile("/FMfrequencies.txt", payload.c_str());
      }
      else {
        Serial.print("FATAL_ERROR Error code: ");
        Serial.println(httpResponseCode);
        return ;
      }
      // Free resources
      http.end();
    } else {
      Serial.println("WiFi Disconnected");
    }
}


void startWifiRadio() {
  // Setup Timer1
  timer1_attachInterrupt(timer1ISR); // Attach the interrupt service routine
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
  timer1_write(30000000); // Set timer for 30 seconds (30,000,000 microseconds)

  // Attempt to connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  const int maxAttempts = 8; // Maximum number of attempts before triggering watchdog reset

  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(2000);
    Serial.print("Attempting to connect to WiFi, attempt ");
    Serial.println(attempts + 1);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    startMP3Streaming();
  } else {
    Serial.println("Failed to connect to WiFi");
  }

  while(true){
    if (mp3->isRunning()) {
    if (!mp3->loop()) {
      Serial.println("MP3 playback stopped unexpectedly");
      mp3->stop();
      break;
    }
    // Toggle the pin to indicate the system is not stuck
    digitalWrite(WDT_RESET_PIN, !digitalRead(WDT_RESET_PIN));
    // Reset Timer1
    timer1_write(30000000); // Reset timer for another 30 seconds
    } else {
      Serial.println("MP3 playback stopped");
      delay(1000); // Delay before attempting to restart
      startMP3Streaming();
    }
  }
   
}

void startMP3Streaming() {
  // Initialize I2SNoDAC for audio output
  out = new AudioOutputI2SNoDAC();
  out->begin();

  // Initialize MP3 player
  fileHttp = new AudioFileSourceHTTPStream("http://mariaradio.hu:8000/teszt");
  mp3 = new AudioGeneratorMP3();
  if (mp3->begin(fileHttp, out)) {
    Serial.println("MP3 player setup complete");
  } else {
    Serial.println("Failed to start MP3 playback");
  }
}
// Duy's end code
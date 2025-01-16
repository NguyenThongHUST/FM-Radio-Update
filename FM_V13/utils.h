#ifndef UTIL_H
#define UTIL_H
#pragma once

//typedef uint16_t RADIO_FREQ;

struct _FMRADIO
{
   // RADIO_FREQ freq;
    float freq;
    String nameMp3;
    String comment;
};

typedef struct  _FMRADIO  FMRADIO;

struct _WFILESTREAM
{
  char* nameMp3;
  char* hostName;
};
 
 typedef struct _WFILESTREAM WFILESTREAM; 

extern void listDir(const char *dirname);
extern bool existFile(const char* fileName); 
extern char *millisecToHoursMinutesSecuntums(char *timeBuffer, unsigned long currentMillis);
extern bool parserProdnumTxt(const char* fileName, String* prodVersion);
extern bool parserParametersTxt(const char* fileName, float* bvolt_1, int* deepTime_1, float* bvolt_2, int* deepTime_2, float* bvolt_3, int* deepTime_3);
//extern RADIO_FREQ* getFMLines(char* pBuffer, int* pLinesNumber, int* pStationStartIndex);
extern bool parserFMFrequenciesTxtVector(const char* fileName, std::vector<FMRADIO>* radios_vector, bool* FMenabled, bool* WIFIenabled, int* stationStartIndex);
//extern bool parserFMFrequenciesTxt(char* buffer, RADIO_FREQ* pRadios_array, int radios_array_length);
//extern bool parserFMFrequenciesTxt_uj(const char* fileName, RADIO_FREQ* pRadios_array, int radios_array_length);
extern WFILESTREAM* getWSLines(char* pBuffer, int* pLinesNumber, int* pStationStartIndex);
extern bool parserWifistreamingsTxt(char* buffer,  WFILESTREAM* pWifiStation_array, int wifi_array_length);
bool LittleFSOpen(const char* fileName, char* pBuffer);
extern int test(void);

/* DUY's code START */
extern void readFile(const char *path);
extern void writeFile(const char *path, const char *message);
extern void appendFile(const char *path, const char *message);
extern void renameFile(const char *path1, const char *path2);
extern void deleteFile(const char *path);

/*DUY's code end*/

#endif // UTIL_H
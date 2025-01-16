#include "FS.h"
#include <stdio.h>
#include <string.h>
#include <cctype>
#include <locale>
//#include "radio.h"
#include <LittleFS.h>
#include "utils.h"

//using namespace std;

char *millisecToHoursMinutesSecuntums(char *timeBuffer, unsigned long currentMillis)
{
    unsigned long seconds = currentMillis / 1000;               // unsigned long
    unsigned long remainder = currentMillis - (seconds * 1000); // added 11/27/2021
    unsigned long minutes = seconds / 60;                       // unsigned long
    unsigned long hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    sprintf(timeBuffer, "h:%lu m:%lu s:%lu ms:%lu", hours, minutes, seconds, remainder);
    return timeBuffer;
}

void listDir(const char *dirname)
{
    Serial.printf("Listing directory: %s\n", dirname);

    Dir root = LittleFS.openDir(dirname);

    while (root.next())
    {
        File file = root.openFile("r");
        Serial.print("\n  FILE: ");
        Serial.print(root.fileName());
        Serial.print("  SIZE: ");
        Serial.print(file.size());
        //    time_t cr = file.getCreationTime();
        //    time_t lw = file.getLastWrite();
        file.close();
    }
}

bool existFile(const char* fileName) 
{
  if (!LittleFS.begin()) {
    Serial.println("FATAL:mounting SPIFFS error");
    return false;    
  }
    File _file = LittleFS.open(fileName, "r");
  if (!_file) {    
    Serial.printf("FATAL: Failed to open file: %s\n", fileName);
    return false;
  }  
  _file.close();
  LittleFS.end();
  return true;
}

std::vector<String> Lines(const char* fileName)
{
  std::vector<String> _linesVector;

  if (!LittleFS.begin()) {
    Serial.println("FATAL:mounting SPIFFS error");
    return std::vector<String>();    
  }
  File _file = LittleFS.open(fileName, "r");
  if (!_file) {
    Serial.println("FATAL: Failed to open file");
    return std::vector<String>();
  }  
  while (_file.available()) {
    String _line=_file.readStringUntil('\n');
    _linesVector.push_back(_line);
  }
  _file.close();
  LittleFS.end();
  if ( _linesVector.size() <= 0) {
    Serial.println("FATAL: not founded lines");
    return std::vector<String>();
  }
  return  _linesVector;
}

bool parserProdnumTxt(const char* fileName, String* prodVersion)
{
  std::vector<String> _linesVector = Lines(fileName);
  if (_linesVector.empty()) {
    Serial.println("FATAL: 'parserProdnum.txt' is empty");
    return false;
  }
  String _item = "";
  bool _see = false;
  //int _voltNumber = 0;
  for (String _vline : _linesVector) {
    _item.clear();       
    char* _token = strtok(_vline.begin(), " ");
    while (_token != NULL) {       
      _item += _token;
      _token = strtok(NULL, " ");
    } 

    _token = strtok(_item.begin(), "\n");
    if (_token != NULL) {
      if ((_token [0] == 'S') && (_token [1] == 'E') && (_token[2] == 'E')) {      
        // SEE
        _see = true;  
        //_voltNumber = 0;      
        //Serial.printf("start SEE\n");
        continue;
      }
      else 
      if ((_token [0] == 'S') && (_token [1] == 'E') && (_token[2] == 'V')) {      
        // SEV
         //Serial.printf("end SEV\n");
          _see = false;
          //_voltNumber = 0; 
          Serial.println("'Prodnum.txt' parser READY");
          return true;
      }
    }
    if (_see) {
      char* _token = strtok(_item.begin(), "\n");
      if (_token != NULL) {
        *prodVersion = String(_token);
      }
    }
  }
  return false;
}

bool parserParametersTxt(const char* fileName, float* bvolt_1, int* deepTime_1, float* bvolt_2, int* deepTime_2, float* bvolt_3, int* deepTime_3)
{
  std::vector<String> _linesVector = Lines(fileName);
  if (_linesVector.empty()) {
    Serial.println("FATAL: 'parserParameters.txt' is empty");
    return false;
  }
  String _item = "";
  bool _bae = false;
  int _voltNumber = 0;
  for (String _vline : _linesVector) {
    _item.clear();       
    char* _token = strtok(_vline.begin(), " ");
    while (_token != NULL) {       
      _item += _token;
      _token = strtok(NULL, " ");
    } 
   
    _token = strtok(_item.begin(), "\n");
    if (_token != NULL) {
      if ((_token [0] == 'B') && (_token [1] == 'A') && (_token[2] == 'E')) {      
        // BAE
        _bae = true;  
        _voltNumber = 0;      
      //  Serial.printf("start BAE\n");
        continue;
      }
      else 
      if ((_token [0] == 'B') && (_token [1] == 'A') && (_token[2] == 'V')) {      
        // BAV
       // Serial.printf("end BAV\n");
          _bae = false;
          _voltNumber = 0; 
          Serial.println("'Parameters.txt' parser READY");
          return true;
      }
    }
    if (_bae) {
      char* _token = strtok(_item.begin(), ",");
      int _count = 0;
      _voltNumber++;       
      while (_token != NULL) {  
        if (_count == 0) {
        // bvolt_x
       //   Serial.printf("bvolt_x: %s, %d\n",_token, _voltNumber);
          float _volttemp = 0;            
          sscanf(_token, "%f", &_volttemp);          
          if (_voltNumber == 1) {            
            *bvolt_1 = _volttemp;  
          } else  if (_voltNumber == 2) {
            *bvolt_2 = _volttemp;
          } else  if (_voltNumber == 3) {
            *bvolt_3 = _volttemp;
          }            
        }                  
        else
        if (_count == 1) {
        // deepTime_x      
         // Serial.printf("deepTime: %s, %d\n",_token, _voltNumber);               
          int _time = 0;       
          sscanf(_token, "%d", &_time);          
          if (_voltNumber == 1) {            
            *deepTime_1 = _time;  
          } else  if (_voltNumber == 2) {
            *deepTime_2 = _time;
          } else  if (_voltNumber == 3) {
            *deepTime_3 = _time;
          }                        
        }
        _count++;        
        _token = strtok(NULL, ","); 
      }
       continue;                       
    }
  }
  return false;
}

//bool parserFMFrequenciesTxt_uj(const char* fileName, RADIO_FREQ* pRadios_array, int radios_array_length)
bool parserFMFrequenciesTxtVector(const char* fileName, std::vector<FMRADIO>* radios_vector, bool* FMenabled, bool* WIFIenabled, int* stationStartIndex)
{
  if (radios_vector == NULL) {
    Serial.println("FATAL:radios_vector=NULL");
    return false;
  }
  radios_vector->clear();

  std::vector<String> _linesVector = Lines(fileName);
  if (_linesVector.empty()) {
    Serial.println("FATAL:linesVector is empty");
    return false;
  }
  String _item = "";
  bool _fme = false;
  for (String _vline : _linesVector) {
    _item.clear();
   // Serial.printf("* %s\n",_vline.c_str());
    char* _token = strtok(_vline.begin(), " ");
    while (_token != NULL) {       
      _item += _token;
      _token = strtok(NULL, " ");
    }
    //Serial.printf("%s\n",_item.c_str());

      //_token = strtok(_vline.begin(), "=");
      _token = strtok(_item.begin(), "=");
      if (_token != NULL) {      
        if ((_token [0] == 'M') && (_token [1] == 'o') && (_token[3] == 'e')) {
          // Mode
          _token = strtok(NULL, ",");                       
          if (_token != NULL) {     
            *FMenabled = true;
            *WIFIenabled = false;           
            //Serial.printf("%s\n",_token, strlen(_token));
            if ((_token [0] == 'W') && (_token [1] == 'I') && (_token [2] == 'F') && (_token [3] == 'I')) {
              *FMenabled = false;
              *WIFIenabled = true;  
            }
          }
          continue;
        }
        else 
        if ((_token [0] == 'd') && (_token [6] == 't') && (_token[11] == 'o')) {
         // defaultRadio
            _token = strtok(NULL, ",");                       
            if (_token != NULL) {                
             //Serial.printf("%s\n",_token);                
              uint16_t _rtnumber = 0;
              sscanf(_token, "%d", &_rtnumber);
              *stationStartIndex = _rtnumber;                                   
          }
          continue;
        }
      }
      _token = strtok(_item.begin(), "\n");
      //_token = strtok(_vline.begin(), "\n");
      if (_token != NULL) {
        if ((_token [0] == 'F') && (_token [1] == 'M') && (_token[2] == 'E')) {      
          // FME
          _fme = true;
          //Serial.printf("start FME\n");
          continue;
        }
        else 
        if ((_token [0] == 'F') && (_token [1] == 'M') && (_token[2] == 'V')) {      
          // FMV          
          //Serial.printf("end FM\n");
          Serial.println("'FMFrequencies.txt' parser READY");
           _fme = false;
           return true;
        }
      }
      if (_fme) {       
        int _count = 0;
        bool _flag = false;
        FMRADIO _fmRadio;
         //char* _token = strtok(_vline.begin(), ",");
        char* _token = strtok(_item.begin(), ",");
         while (_token != NULL) {  
          if (_count == 0) {
          // number.
           // Serial.printf("rnumber: %s\n",_token);
            float _frtemp = 0;
            uint16_t _fr = 0;
            if (sscanf(_token, "%f", &_frtemp) != 0) {
             // _fr = (_frtemp*10)*10; 
              _flag = true;                         
            } else {
              //_fr = 0;
              _frtemp  = 0;
              _flag = false; 
            }   
            //_fmRadio.freq = (RADIO_FREQ)_fr;  
            _fmRadio.freq = _frtemp;                            
          }
          else
          if (_count == 1) {
          // mp3?
            //Serial.printf("mp3: %s\n",_token);
            if (_token != NULL) {
              _fmRadio.nameMp3  = String(_token); 
            } else {
              _fmRadio.nameMp3  = String(""); 
            }                         
          }         
          else
          if (_count == 2) {
          // commment3?
            //Serial.printf("comment: %s\n",_token);
            if (_token != NULL) {
              _fmRadio.comment = String(_token); 
            } else {
              _fmRadio.comment = String(""); 
            }         
          }  
          _count++;         
          _token = strtok(NULL, ",");
        }
        if (_flag) {
        //  Serial.printf("feq : %d, mp3: %s, comment: %s \n",_fmRadio.freq, _fmRadio.nameMp3.c_str(), _fmRadio.comment.c_str());
          radios_vector->push_back(_fmRadio);
          _flag = false;
        }
        continue;        
      }     
  }
  return false;
}

/// Wifi streaming
WFILESTREAM* getWSLines(char* pBuffer, int* pLinesNumber, int* pStationStartIndex)
{
    Serial.println("getWSLines"); 
    size_t _len = strlen(pBuffer);
    if ((_len <= 0) || (_len > 1024))
    {
      Serial.println("WS lines bad"); 
      return NULL;
    }
    char *_temp=new char[_len+1];
    strncpy(_temp, pBuffer, _len);
    char* _token = strtok(_temp, "\n");
    int  _line_index = 0; 
    *pLinesNumber=0;
    *pStationStartIndex=0;
     
    while (_token != NULL) 
    {                                    
        if (_token[0]=='W' && _token[1]=='S')
        {
          if (_token+2 == NULL)
          {
            return NULL;
          }
         if (_token[0]=='W' && _token[1]=='S' && _token[2]=='E' )
          {
           // Serial.println("WSE lines end");         
             break;
          }          
          else
          {            
            uint16_t _itemp = 0; 
            char number[] = {'1', '\0'};  
            number[0] = _token[2];     
            _itemp = atoi(number) ;  
            // sscanf(_token[2], "%d", &_itemp);
            if ((_itemp>= 1) && (_itemp<= 9))
            {
              *pStationStartIndex=_itemp;
              //Serial.println("WSx lines begin");             
            }                              
          }
        }
        else 
        {
          _line_index++;
       //   Serial.printf("WS _line_index: %d\n",_line_index);
        }
        _token = strtok(NULL, "\n");         
      }    
      delete (_temp);
      WFILESTREAM* _result = NULL;
     
      if ((_line_index >= 0) && (_line_index <= 20))
      {
        //Serial.printf("WS _line_index 0: %d\n",_line_index);
        _line_index /= 2;  
        //Serial.printf("WS _line_index 1: %d\n",_line_index);    
        _result = new WFILESTREAM[_line_index];
        *pLinesNumber = _line_index;
        for (int ii=0;ii<*pLinesNumber;ii++)
        { 
          _result[ii].nameMp3=NULL;
          _result[ii].hostName=NULL;
        }        
      }
      else
      {
        *pLinesNumber =  0;
        return NULL;
      }  
  return _result;
}

bool parserWifistreamingsTxt(char* buffer,  WFILESTREAM* pWifiStation_array, int wifi_array_length)
{
    int line_index = 0;     
    char *p;        
    char _printbuffer[64];
   int _fmindex = wifi_array_length;
    Serial.printf("parserWifistreamingsTxt(): %d\n", _fmindex);      
    char* _token = strtok(buffer, "\n");
    int _fmIndexCurrent=0;
    bool _fm = false;

    while (_token != NULL) {     
      if (!_fm)
      {          
            if (_token[0]=='W' && _token[1]=='S')
            {
              if (_token+2 == NULL)
              {                
                return false;
              }
              if (_token[0]=='W' && _token[1]=='S' && _token[2]=='E' )
              {
                Serial.println("WSE lines end"); 
                return true;                        
              }               
              else
              {
                  Serial.println("WSx lines  begin");
                  _fmIndexCurrent=0;
              }          
            }
            else 
            {  
              if (_fmIndexCurrent>=(_fmindex*2))
              {
                Serial.println("ERROR:if (_fmIndexCurrent>=_fmindex)");
                return false;
              }             
              size_t _len = strlen(_token);
              if (_len <= 3 ) 
              {
                Serial.println("_len <= 3");                
              }  
              else
              if(_fmIndexCurrent >= 0)
              {  
                if ((_fmIndexCurrent % 2) == 0)
                {  // event
                    pWifiStation_array[_fmIndexCurrent/2].nameMp3 = new char[_len+1];
                    strlcpy(pWifiStation_array[_fmIndexCurrent/2].nameMp3,_token,_len);
                }
                else
                {  // odd
                    pWifiStation_array[_fmIndexCurrent/2].hostName = new char[_len+1];
                    strlcpy(pWifiStation_array[_fmIndexCurrent/2].hostName,_token,_len);
                }
                //Serial.printf("_fmIndexCurrent % 2: %d, %d\n",_fmIndexCurrent, _fmIndexCurrent % 2);
                  
               // pWifiStation_array[_fmIndexCurrent]=_ft;
              }
                                                         
              _fmIndexCurrent++;
            }            
      }
      _token = strtok(NULL, "\n");         
    }    
  return true;
}


// LittleFS part.
bool LittleFSOpen(const char* fileName, char* pBuffer)
{
  bool _result = true;
  if (!LittleFS.begin())
  {
      Serial.println("Error opening LittleFS!");
      return false;
  }
  File file = LittleFS.open(fileName, "r");
  if (!file)
  {
      // Serial.println("Error opening file!");
      // startPlayback("/Hiba.mp3"); // LittleFS-bol
      _result = false;
      
  }
  else
  {
    while (file.available())
    {
      if (file.read((uint8_t*)pBuffer, file.size()+1)==0)
      {
          //Serial.println("FATAL = file rewd!");
          //startPlayback("/Hiba.mp3");
          _result = false;
          break;
      }
    }
  }
  file.close();
  LittleFS.end(); 
 // Serial.println("---");   
  return _result;
}
/*
      Serial.printf("\n\nSdk version: %s\n", ESP.getSdkVersion());
       Serial.printf("Core Version: %s\n", ESP.getCoreVersion().c_str());
       Serial.printf("Boot Version: %u\n", ESP.getBootVersion());
       Serial.printf("Boot Mode: %u\n", ESP.getBootMode());
       Serial.printf("CPU Frequency: %u MHz\n", ESP.getCpuFreqMHz());

*/

/*DUY's code START*/

void readFile(const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) { Serial.write(file.read()); }
  file.close();
}

void writeFile(const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  delay(2000);  // Make sure the CREATE and LASTWRITE times are different
  file.close();
}

void appendFile(const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = LittleFS.open(path, "a");
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (LittleFS.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (LittleFS.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

/*DUY's code end*/
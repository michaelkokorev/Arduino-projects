#include <Wire.h>
#include "RTClib.h"

#include <EEPROM.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define BUFFER_SIZE 100
#define VERSION     "3.0"

#define  ALARM_DATA_LENGHT 4
#define  MAX_ALARM_NUMBER 99
#define  NUMBER_DEVICE  8

#define  MINUTES_IN_DAY  1440

// RTC object
RTC_DS1307 RTC;

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 
RF24 radio(9,10);
//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Buffer for incoming data
char serial_buffer[BUFFER_SIZE];
int buffer_position;

enum eDayOfWeek {eSunday = 0, eMonday, eTuesday, eWednesday, eThursday, eFriday, eSaturday};

struct ALARM_DATA {
    unsigned short AlarmLen        : 5;  // 00000000 00000000 00000000 000?????
    unsigned short AlarmMinute     : 6;  // 00000000 00000000 00000??? ???00000
    unsigned short AlarmHour       : 5;  // 00000000 00000000 ?????000 00000000
    unsigned short AlarmSunday     : 1;  // 00000000 0000000? 00000000 00000000
    unsigned short AlarmMonday     : 1;  // 00000000 000000?0 00000000 00000000
    unsigned short AlarmTuesday    : 1;  // 00000000 00000?00 00000000 00000000
    unsigned short AlarmWednesday  : 1;  // 00000000 0000?000 00000000 00000000
    unsigned short AlarmThursday   : 1;  // 00000000 000?0000 00000000 00000000
    unsigned short AlarmFriday     : 1;  // 00000000 00?00000 00000000 00000000
    unsigned short AlarmSaturday   : 1;  // 00000000 0?000000 00000000 00000000
    unsigned short AlarmActive     : 1;  // 00000000 ?0000000 00000000 00000000
    unsigned short AlarmDevice1    : 1;  // 0000000? 00000000 00000000 00000000
    unsigned short AlarmDevice2    : 1;  // 000000?0 00000000 00000000 00000000
    unsigned short AlarmDevice3    : 1;  // 00000?00 00000000 00000000 00000000
    unsigned short AlarmDevice4    : 1;  // 0000?000 00000000 00000000 00000000
    unsigned short AlarmDevice5    : 1;  // 000?0000 00000000 00000000 00000000
    unsigned short AlarmDevice6    : 1;  // 00?00000 00000000 00000000 00000000
    unsigned short AlarmDevice7    : 1;  // 0?000000 00000000 00000000 00000000
    unsigned short AlarmDevice8    : 1;  // ?0000000 00000000 00000000 00000000
};

union AlarmType{
  unsigned long  lValue;
  byte           bValue[4];
  ALARM_DATA     aValue;
};

AlarmType alarmArray[MAX_ALARM_NUMBER];
int       alarmNumber; 

AlarmType atNextAlarm;
bool bAlarmWasSet = false;
int iAlarmDayOfWeek = -1;

unsigned int uiDeviceStatus[NUMBER_DEVICE];

void setup() {
  
  Serial.begin(57600);
  Wire.begin();
  RTC.begin();

  //
  // Setup and configure rf radio
  //

  radio.begin();
//  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.setRetries(15,15);

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.startListening();
  radio.printDetails();
  
  resetBuffer();

  for(int i = 0; i < NUMBER_DEVICE; i++){
    uiDeviceStatus[i] = 0;
  }
  
  bAlarmWasSet = GetAlarmList();
}

void loop() {
  
  // Wait for incoming data on serial port
  if (Serial.available() > 0) {
    
    // Read the incoming character
    char incoming_char = Serial.read();
    
    // End of line?
    if(incoming_char == '\n') {
      // Parse the command
      ParseIncomingComand();
    }
    // Carriage return, do nothing
    else if(incoming_char == '\r');
    // Normal character
    else {
      // Buffer full, we need to reset it
      if(buffer_position == BUFFER_SIZE - 1) buffer_position = 0;

      // Store the character in the buffer and move the index
      serial_buffer[buffer_position] = incoming_char;
      buffer_position++;      
    }
  }
  else{
    DateTime now = RTC.now();
    
    if (bAlarmWasSet == true){
    
      if((now.hour() == atNextAlarm.aValue.AlarmHour) && (now.minute() == atNextAlarm.aValue.AlarmMinute)){
        int iMinutesRemain = atNextAlarm.aValue.AlarmLen * 5 + now.hour() * 60 + now.minute();

        if(atNextAlarm.aValue.AlarmDevice1 == 1){
          SendNRFCommand((unsigned short)11);
          if(uiDeviceStatus[0] < iMinutesRemain) uiDeviceStatus[0] = iMinutesRemain;
        }
        if(atNextAlarm.aValue.AlarmDevice2 == 1){
          SendNRFCommand((unsigned short)21);
          if(uiDeviceStatus[1] < iMinutesRemain) uiDeviceStatus[1] = iMinutesRemain;
        }
        if(atNextAlarm.aValue.AlarmDevice3 == 1){
          SendNRFCommand((unsigned short)31);
          if(uiDeviceStatus[2] < iMinutesRemain) uiDeviceStatus[2] = iMinutesRemain;
        }
        if(atNextAlarm.aValue.AlarmDevice4 == 1){
          SendNRFCommand((unsigned short)41);
          if(uiDeviceStatus[3] < iMinutesRemain) uiDeviceStatus[3] = iMinutesRemain;
        }
        if(atNextAlarm.aValue.AlarmDevice5 == 1){
          SendNRFCommand((unsigned short)51);
          if(uiDeviceStatus[4] < iMinutesRemain) uiDeviceStatus[4] = iMinutesRemain;
        }
        if(atNextAlarm.aValue.AlarmDevice6 == 1){
          SendNRFCommand((unsigned short)61);
          if(uiDeviceStatus[5] < iMinutesRemain) uiDeviceStatus[5] = iMinutesRemain;
        }
        if(atNextAlarm.aValue.AlarmDevice7 == 1){
          SendNRFCommand((unsigned short)71);
          if(uiDeviceStatus[6] < iMinutesRemain) uiDeviceStatus[6] = iMinutesRemain;
        }
        if(atNextAlarm.aValue.AlarmDevice8 == 1){
          SendNRFCommand((unsigned short)81);
          if(uiDeviceStatus[7] < iMinutesRemain) uiDeviceStatus[7] = iMinutesRemain;
        }
        
        bAlarmWasSet = GetNextAlarm();
      }
    }
    else if(now.dayOfWeek()!= iAlarmDayOfWeek){
        bAlarmWasSet = GetNextAlarm();
        iAlarmDayOfWeek = now.dayOfWeek();
    }
    
    if(IsAllDevicesOff() == false){
        int iMinutesNow = now.hour() * 60 + now.minute();

        for(int i = 0; i < NUMBER_DEVICE; i++){
          if((uiDeviceStatus[i] > 0) && ((uiDeviceStatus[i] <= iMinutesNow) || ((uiDeviceStatus[i] >= MINUTES_IN_DAY) && ((uiDeviceStatus[i] - MINUTES_IN_DAY) <= iMinutesNow)))){
            SendNRFCommand((unsigned short)(i + 1) * 10);
            uiDeviceStatus[i] = 0;
          }
        }
    }
  }
//    delay(1000);    
}

bool IsAllDevicesOff(){
bool bRet = true;

  for(int i = 0; i < NUMBER_DEVICE; i++){
    if(uiDeviceStatus[i] > 0){
      bRet = false;
      break;
    }
  }
  
  return bRet;
}

void ParseIncomingComand(){
AlarmType at;
String strRet = "Error: ";

      switch(serial_buffer[0]){
        case '#':                                                // '##' - Test conektion
          if(serial_buffer[1] == '#') strRet = "!!";
          break;
        case '?':{                                               // '?' Get information
            switch(serial_buffer[1]){
              case 'V':                                          // '?V' Get Number Version
                strRet = VERSION;
                break;
              case 'T':{                                         // '?T' Get RTC Time
                  DateTime now = RTC.now();
                  char chStr[20];
                  sprintf(chStr, "%02d.%02d.%d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
                  strRet = chStr;
                }
                break;
                case 'W':{                                       // '?Woo' Send command to nRF24 module get status remote pin.
                  String message_string = String(serial_buffer);
                  
                  unsigned short id = message_string.substring(2, 4).toInt();
                  unsigned short message = id * 10 + 2;
                  
                  strRet = (SendNRFCommand(message) == 1) ? "On" : "Off";
                }
                break;
              case 'A':{                                         // '?A  ' Get Alarm data 
                  if(serial_buffer[2] == 'N'){                   // '?AN ' Get Alarm Number
                      byte value = EEPROM.read(0);
                      char chStr[2];
                      sprintf(chStr, "%02d", value);
                      strRet = chStr;
                  }
                  else if(serial_buffer[2] == 'C') {            // '?AC ' Get Nearest Alarm value
                    if(bAlarmWasSet == true){
                      char chStr[9];
                      sprintf(chStr, "%02d:%02d %03d", atNextAlarm.aValue.AlarmHour, atNextAlarm.aValue.AlarmMinute, atNextAlarm.aValue.AlarmLen * 5);
                      strRet = chStr;
                     
                      if((atNextAlarm.bValue[2] == 0) || (at.bValue[2] == 0x80))
                        strRet += " Nothing";
                      else{
                        if(atNextAlarm.aValue.AlarmMonday == 1) strRet += " Mon.";
                        if(atNextAlarm.aValue.AlarmTuesday == 1) strRet += " Tue.";
                        if(atNextAlarm.aValue.AlarmWednesday == 1) strRet += " Wed.";
                        if(atNextAlarm.aValue.AlarmThursday == 1) strRet += " Thu.";
                        if(atNextAlarm.aValue.AlarmFriday == 1) strRet += " Fri.";
                        if(atNextAlarm.aValue.AlarmSaturday == 1) strRet += " Sat.";
                        if(atNextAlarm.aValue.AlarmSunday == 1) strRet += " Sun.";
                      }  
                      
                      if(atNextAlarm.bValue[3] == 0)
                        strRet += "No Devices";
                      else{
                        if(atNextAlarm.aValue.AlarmDevice1 == 1) strRet += " Dev1.";
                        if(atNextAlarm.aValue.AlarmDevice2 == 1) strRet += " Dev2.";
                        if(atNextAlarm.aValue.AlarmDevice3 == 1) strRet += " Dev3.";
                        if(atNextAlarm.aValue.AlarmDevice4 == 1) strRet += " Dev4.";
                        if(atNextAlarm.aValue.AlarmDevice5 == 1) strRet += " Dev5.";
                        if(atNextAlarm.aValue.AlarmDevice6 == 1) strRet += " Dev6.";
                        if(atNextAlarm.aValue.AlarmDevice7 == 1) strRet += " Dev7.";
                        if(atNextAlarm.aValue.AlarmDevice8 == 1) strRet += " Dev8.";
                      }
                      
                      strRet += (atNextAlarm.aValue.AlarmActive == 1) ? " Active" : " No Active";
                    }
                    else
                      strRet = " Not Active";
                  }
                  else{                                          // '?Aoo' Get Alarm data value
                      int address = atoi((serial_buffer + 2));
                      
                      at.bValue[3] = EEPROM.read(1 + address * ALARM_DATA_LENGHT);
                      at.bValue[2] = EEPROM.read(1 + address * ALARM_DATA_LENGHT + 1);
                      at.bValue[1] = EEPROM.read(1 + address * ALARM_DATA_LENGHT + 2);
                      at.bValue[0] = EEPROM.read(1 + address * ALARM_DATA_LENGHT + 3);
                      
                      char chStr[9];
                      sprintf(chStr, "%02d:%02d %03d", at.aValue.AlarmHour, at.aValue.AlarmMinute, at.aValue.AlarmLen * 5);
                      strRet = chStr;
                     
                      if((at.bValue[2] == 0) || (at.bValue[2] == 0x80))
                        strRet += " Nothing";
                      else{
                        if(at.aValue.AlarmMonday == 1) strRet += " Mon.";
                        if(at.aValue.AlarmTuesday == 1) strRet += " Tue.";
                        if(at.aValue.AlarmWednesday == 1) strRet += " Wed.";
                        if(at.aValue.AlarmThursday == 1) strRet += " Thu.";
                        if(at.aValue.AlarmFriday == 1) strRet += " Fri.";
                        if(at.aValue.AlarmSaturday == 1) strRet += " Sat.";
                        if(at.aValue.AlarmSunday == 1) strRet += " Sun.";
                      }  
                      
                      if(at.bValue[3] == 0)
                        strRet += "No Devices";
                      else{
                        if(at.aValue.AlarmDevice1 == 1) strRet += " Dev1.";
                        if(at.aValue.AlarmDevice2 == 1) strRet += " Dev2.";
                        if(at.aValue.AlarmDevice3 == 1) strRet += " Dev3.";
                        if(at.aValue.AlarmDevice4 == 1) strRet += " Dev4.";
                        if(at.aValue.AlarmDevice5 == 1) strRet += " Dev5.";
                        if(at.aValue.AlarmDevice6 == 1) strRet += " Dev6.";
                        if(at.aValue.AlarmDevice7 == 1) strRet += " Dev7.";
                        if(at.aValue.AlarmDevice8 == 1) strRet += " Dev8.";
                      }
                      
                      strRet += (at.aValue.AlarmActive == 1) ? " Active" : " No Active";
                  }
                }
                break;
            }
          }
          break;
        case '!':{                                             // '! ' Set data
            switch(serial_buffer[1]){
              case 'W':{                                       // '!Woo' Send command to nRF24 module
                String message_string = String(serial_buffer);
                
                unsigned short id = message_string.substring(2, 4).toInt();
                unsigned short message = id * 10;
                
                if (message_string.indexOf("On", 4) >= 0) message += 1;
                
                if(SendNRFCommand(message) != 3)
                  strRet = "OK";
                else
                  strRet+= "4";
              }
              break;
              case 'T':{                                       // '!T' Set Current Time
                String time_string = String(serial_buffer);
                int day = time_string.substring(2, 4).toInt();
                int month = time_string.substring(4, 6).toInt();        
                int year = time_string.substring(6, 10).toInt();
                int hour = time_string.substring(10, 12).toInt();
                int minute = time_string.substring(12, 14).toInt();
                int second = time_string.substring(14, 16).toInt();
                DateTime set_time = DateTime(year, month, day, hour, minute, second);
                RTC.adjust(set_time);
                strRet = "OK";
              }
              break;
              case 'A':{                                       // '!A '  Set Alarm data
                String alarm_string = String(serial_buffer);
                int CurrentAlarm;
                byte AlarmNumber;        
                
                switch(serial_buffer[2]){
                  case 'C':{                                   // '!AC'  Clear All Alarm List
                    EEPROM.write(0, 0);
                    alarmNumber = 0;
                    delay(100);
                    strRet = "OK";
                  }
                  break;
                  case 'D':{                                   // '!ADoo' Delete Alarm data
                    CurrentAlarm = alarm_string.substring(3, 5).toInt();
                    AlarmNumber = EEPROM.read(0);
                    AlarmNumber --;          
           
                    if(AlarmNumber >= CurrentAlarm){
                      for(int i = CurrentAlarm; i < AlarmNumber; i++){
                        delay(100);
                        EEPROM.write(1 + i * ALARM_DATA_LENGHT, EEPROM.read(1 + (i + 1) * ALARM_DATA_LENGHT));
                        delay(100);
                        EEPROM.write(1 + i * ALARM_DATA_LENGHT + 1, EEPROM.read(1 + (i + 1) * ALARM_DATA_LENGHT + 1));
                        delay(100);
                        EEPROM.write(1 + i * ALARM_DATA_LENGHT + 2, EEPROM.read(1 + (i + 1) * ALARM_DATA_LENGHT + 2));
                        delay(100);
                        EEPROM.write(1 + i * ALARM_DATA_LENGHT + 3, EEPROM.read(1 + (i + 1) * ALARM_DATA_LENGHT + 3));
                      }
                      
                      EEPROM.write(0, AlarmNumber);
                      
                      bAlarmWasSet = GetAlarmList();
                      
                      strRet = "OK";
                    }
                    else
                      strRet+= "3";
                  }
                  break;
                  case 'A':{                                   // '!AA'  Add Alarm data
                    CurrentAlarm = EEPROM.read(0);
                    AlarmNumber = CurrentAlarm + 1;
                  
                    if(AlarmNumber < MAX_ALARM_NUMBER){
                      EEPROM.write(0, AlarmNumber);
                      if (AddAlarmData(CurrentAlarm, alarm_string))
                        strRet = "OK";
                      else
                        strRet+= "2";              
                    }              
                    else
                      strRet+= "1";              
                  }
                  break;
                  default:{                                  // '!Aoo' Add Alarm data
                    CurrentAlarm = alarm_string.substring(2, 4).toInt();
                    
                    if (AddAlarmData(CurrentAlarm, alarm_string))
                      strRet = "OK";
                    else
                      strRet+= "2";              
                  }
                  break;
              }
              break;
            }
          }
          break;
      }
      break;
    }      
    
    Serial.println(strRet);
      
    resetBuffer();
}

void resetBuffer(){
  // Reset the buffer
  for(int i = 0; i < BUFFER_SIZE; i++){
    serial_buffer[i] = 0;
  }
  
  buffer_position = 0;
}

bool AddAlarmData(int CurrentAlarm, String alarm_string){
bool bRet = false;
AlarmType at;

    if(CurrentAlarm < MAX_ALARM_NUMBER){
      at.bValue[0] = 0;
      at.bValue[1] = 0;
      at.bValue[2] = 0;
      at.bValue[3] = 0;
      
      at.aValue.AlarmHour = alarm_string.substring(4, 6).toInt();
      at.aValue.AlarmMinute = alarm_string.substring(6, 8).toInt();
      at.aValue.AlarmLen = (byte)(alarm_string.substring(8, 11).toInt() / 5);

      if(alarm_string.indexOf("Sun.", 10) >= 0) at.aValue.AlarmSunday = 1;
      if(alarm_string.indexOf("Mon.", 10) >= 0) at.aValue.AlarmMonday = 1;
      if(alarm_string.indexOf("Tue.", 10) >= 0) at.aValue.AlarmTuesday = 1;
      if(alarm_string.indexOf("Wed.", 10) >= 0) at.aValue.AlarmWednesday = 1;
      if(alarm_string.indexOf("Thu.", 10) >= 0) at.aValue.AlarmThursday = 1;
      if(alarm_string.indexOf("Fri.", 10) >= 0) at.aValue.AlarmFriday = 1;
      if(alarm_string.indexOf("Sat.", 10) >= 0) at.aValue.AlarmSaturday = 1;
      
      at.aValue.AlarmActive = (alarm_string.indexOf("No", 10) >= 0) ? 0 : 1;

      if(alarm_string.indexOf("Dev1.", 10) >= 0) at.aValue.AlarmDevice1 = 1;
      if(alarm_string.indexOf("Dev2.", 10) >= 0) at.aValue.AlarmDevice2 = 1;
      if(alarm_string.indexOf("Dev3.", 10) >= 0) at.aValue.AlarmDevice3 = 1;
      if(alarm_string.indexOf("Dev4.", 10) >= 0) at.aValue.AlarmDevice4 = 1;
      if(alarm_string.indexOf("Dev5.", 10) >= 0) at.aValue.AlarmDevice5 = 1;
      if(alarm_string.indexOf("Dev6.", 10) >= 0) at.aValue.AlarmDevice6 = 1;
      if(alarm_string.indexOf("Dev7.", 10) >= 0) at.aValue.AlarmDevice7 = 1;
      if(alarm_string.indexOf("Dev8.", 10) >= 0) at.aValue.AlarmDevice8 = 1;
      
      delay(100);
      EEPROM.write(1 + CurrentAlarm * ALARM_DATA_LENGHT, at.bValue[3]);
      delay(100);
      EEPROM.write(1 + CurrentAlarm * ALARM_DATA_LENGHT + 1, at.bValue[2]);
      delay(100);
      EEPROM.write(1 + CurrentAlarm * ALARM_DATA_LENGHT + 2, at.bValue[1]);
      delay(100);
      EEPROM.write(1 + CurrentAlarm * ALARM_DATA_LENGHT + 3, at.bValue[0]);
     
     bAlarmWasSet = GetAlarmList();
     bRet = true;
  }

  return bRet;
}

unsigned short SendNRFCommand(unsigned short message){
unsigned long got_message = 3;
bool bRet = false;

    // First, stop listening so we can talk.
    radio.stopListening();

    // Take the time, and send it.  This will block until complete
    bRet = radio.write( &message, sizeof(unsigned short) );
    
    // Now, continue listening
    radio.startListening();
    
    if (bRet == true){
      // Wait here until we get a response, or timeout (250ms)
      unsigned long started_waiting_at = millis();
      bool timeout = false;
      while (!radio.available() && !timeout )
        if (millis() - started_waiting_at > 250 )
          timeout = true;

      // Describe the results
      if ( timeout == false )
      {
        // Grab the response, compare, and send to debugging spew
        radio.read( &got_message, sizeof(unsigned long) );
      }
    }
    
    return got_message;
}

bool GetNextAlarm(){
int iMinutesNow;
int iDeltaAlarm;
int iMinDeltaAlarm;
bool bFound;
AlarmType atOldAlarm;

  iMinDeltaAlarm = -1;
  atNextAlarm.lValue = 0;

  if(alarmNumber > 0){
    DateTime now = RTC.now();
    iMinutesNow = now.hour() * 60 + now.minute();
    iAlarmDayOfWeek = now.dayOfWeek();
    atOldAlarm.lValue = atNextAlarm.lValue;

    for(int i = 0; i < alarmNumber; i++){
      if(atOldAlarm.lValue == alarmArray[i].lValue) continue;
      
      switch((eDayOfWeek)now.dayOfWeek()){
        case eSunday:
          bFound = (alarmArray[i].aValue.AlarmSunday == 1);
          break;
        case eMonday:
          bFound = (alarmArray[i].aValue.AlarmMonday == 1);
          break;
        case eTuesday:
          bFound = (alarmArray[i].aValue.AlarmTuesday == 1);
          break;
        case eWednesday:
          bFound = (alarmArray[i].aValue.AlarmWednesday == 1);
          break;
        case eThursday:
          bFound = (alarmArray[i].aValue.AlarmThursday == 1);
          break;
        case eFriday:
          bFound = (alarmArray[i].aValue.AlarmFriday == 1);
          break;
        case eSaturday:
          bFound = (alarmArray[i].aValue.AlarmSaturday == 1);
          break;
      }
      
      if(bFound == true){
        iDeltaAlarm = alarmArray[i].aValue.AlarmHour * 60 + alarmArray[i].aValue.AlarmMinute - iMinutesNow;
        
        if((iDeltaAlarm > 0) && ((iMinDeltaAlarm > iDeltaAlarm) || (iMinDeltaAlarm == -1))){
            iMinDeltaAlarm = iDeltaAlarm;
            atNextAlarm.lValue = alarmArray[i].lValue;
        }
      }
     }
  }
  
  return (iMinDeltaAlarm > 0);
}

bool GetAlarmList(){
AlarmType at;
unsigned short uNumAlarms = 0;

  uNumAlarms = EEPROM.read(0);
  alarmNumber = 0;

  if(uNumAlarms > 0){
    for(int address = 0; address < uNumAlarms; address++){
      at.bValue[3] = EEPROM.read(1 + address * ALARM_DATA_LENGHT);
      at.bValue[2] = EEPROM.read(1 + address * ALARM_DATA_LENGHT + 1);
      at.bValue[1] = EEPROM.read(1 + address * ALARM_DATA_LENGHT + 2);
      at.bValue[0] = EEPROM.read(1 + address * ALARM_DATA_LENGHT + 3);
      
      if((at.aValue.AlarmActive == 1) && (!((at.bValue[2] == 0) || (at.bValue[2] == 0x80))) && (at.bValue[3] != 0)){
        alarmArray[alarmNumber].lValue = at.lValue;
        alarmNumber ++; 
      }
    }
  }

  return GetNextAlarm();
}

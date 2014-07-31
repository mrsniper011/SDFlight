/*----------------------------------------------------------------------------------
|Version History Table
|-----------------------------------------------------------------------------------
|  V# | R Date | Changes                      
|-----------------------------------------------------------------------------------
| 2014
|-----------------------------------------------------------------------------------
| 0.1 | May 11 | Initial build, KML proof of concept
| 0.2 | Jun 20 | Filtering for false data and outliers
| 0.3 | Jul 07 | Auto file split between flights
|
*/

//external libraries
#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
//#include <EEPROM.h>

//gps serial port
SoftwareSerial mySerial(2, 3);
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false
/* set to true to only log to SD when GPS has a fix, for debugging, keep it false */
#define LOG_FIXONLY false//to be pulled from sd card/eeprom

// Set the pins used
#define chipSelect 4
#define ledPin 7
#define btnPin 5
#define loPin 9
#define loActive true//when true program returns at checkpoints to prevent data loss

#define averagePoints 20
#define averagePoints2 10

File logfile;
// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// blink out an error code
void error(uint8_t errno) {
/*
  if (SD.errorCode()) {
    putstring("SD error: ");
    //Serial.print(card.errorCode(), HEX);
    //Serial.print(',');
    //Serial.println(card.errorData(), HEX);
  }
  */
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}


//When given two coordinates this function returns the great-circle distance between them.
float HaverSine(float lat1, float lon1, float lat2, float lon2)
{
  float ToRad = PI / 180.0;
  float R = 6371;   // radius earth in Km
  
  float dLat = (lat2-lat1) * ToRad;
  float dLon = (lon2-lon1) * ToRad; 
  
  float a = sin(dLat/2) * sin(dLat/2) +
        cos(lat1 * ToRad) * cos(lat2 * ToRad) * 
        sin(dLon/2) * sin(dLon/2); 
        
  float c = 2 * atan2(sqrt(a), sqrt(1-a)); 
  
  float d = R * c;
  return d;
}



//char filename[15];
char filename2[15];
/*This is spaghetti code to save on program memory. this is what we are printing to the sd card:
<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
<Document>
<name>SDFlight Path</name>
<description></description>
<Style id="yellowLineGreenPoly">
<LineStyle>
<color>7FFF6421</color>
<width>5</width>
</LineStyle>
<PolyStyle>
<color>7f8A886D</color>
</PolyStyle>
</Style>
<Placemark>
<name>SDFlight Path</name>
<description></description>
<styleUrl>#yellowLineGreenPoly</styleUrl>
<LineString>
<extrude>1</extrude>
<tessellate>1</tessellate>
<altitudeMode>absolute</altitudeMode>
<coordinates></coordinates>
</LineString>
</Placemark>
</Document>
</kml>
*/
prog_char string_0[] PROGMEM = "<?xml version=\"1.0";
prog_char string_1[] PROGMEM = "\" encoding=\"UTF-8";
prog_char string_2[] PROGMEM = "\"?>\n";
prog_char string_3[] PROGMEM = "<kml xmlns=\"http://";   
prog_char string_4[] PROGMEM = "www.opengis.net/kml/";
prog_char string_5[] PROGMEM = "2.2\">\n";
prog_char string_6[] PROGMEM = "<Document>\n";
prog_char string_7[] PROGMEM = "<name>SDFlight Path<";
prog_char string_8[] PROGMEM = "/name>\n";
prog_char string_9[] PROGMEM = "<description></descr";
prog_char string_10[] PROGMEM = "iption>\n";
prog_char string_11[] PROGMEM = "<Style id=\"yellowL";
prog_char string_12[] PROGMEM = "ineGreenPoly\">\n";
prog_char string_13[] PROGMEM = "<LineStyle>\n";
prog_char string_14[] PROGMEM = "<color>7FFF6421</co";
prog_char string_15[] PROGMEM = "lor>\n";
prog_char string_16[] PROGMEM = "<width>5</width>\n";
prog_char string_17[] PROGMEM = "</LineStyle>\n";
prog_char string_18[] PROGMEM = "<PolyStyle>\n";
prog_char string_19[] PROGMEM = "<color>7f8A886D</c";
prog_char string_20[] PROGMEM = "olor>\n";
prog_char string_21[] PROGMEM = "</PolyStyle>\n";
prog_char string_22[] PROGMEM = "</Style>\n";
prog_char string_23[] PROGMEM = "<Placemark>\n";
prog_char string_24[] PROGMEM = "<name>SDFlight Pat";
prog_char string_25[] PROGMEM = "h</name>\n";
prog_char string_26[] PROGMEM = "<description>";
prog_char string_27[] PROGMEM = "</description>\n";
prog_char string_28[] PROGMEM = "<styleUrl>#yellowLi";
prog_char string_29[] PROGMEM = "neGreenPoly</styleU";
prog_char string_30[] PROGMEM = "rl>\n";
prog_char string_31[] PROGMEM = "<LineString>\n<ext";
prog_char string_32[] PROGMEM = "rude>1</extrude>\n";
prog_char string_33[] PROGMEM = "<tessellate>1";
prog_char string_34[] PROGMEM = "</tessellate>\n";
prog_char string_35[] PROGMEM = "<altitudeMode>absol";
prog_char string_36[] PROGMEM = "ute</altitudeMode>";
prog_char string_37[] PROGMEM = "\n<coordinates>";
prog_char string_38[] PROGMEM = "</coordinates>\n";
prog_char string_39[] PROGMEM = "</LineString>\n";
prog_char string_40[] PROGMEM = "</Placemark>\n";
prog_char string_41[] PROGMEM = "</Document>\n";
prog_char string_42[] PROGMEM = "</kml>";


PROGMEM const char *string_table[] = 	   // change "string_table" name to suit
{   
  string_0,
  string_1,
  string_2,
  string_3,
  string_4,
  string_5,
  string_6,
  string_7,
  string_8,
  string_9,
  string_10,
  string_11,
  string_12,
  string_13,
  string_14,
  string_15,
  string_16,
  string_17,
  string_18,
  string_19,
  string_20,
  string_21,
  string_22,
  string_23,
  string_24,
  string_25,
  string_26,
  string_27,
  string_28,
  string_29,
  string_30,
  string_31,
  string_32,
  string_33,
  string_34,
  string_35,
  string_36,
  string_37,
  string_38,
  string_39,
  string_40,
  string_41,
  string_42 };

int16_t altHistory[averagePoints];
int32_t latHistory[averagePoints2];
int32_t lonHistory[averagePoints2];

//this function will start of a phresh new KML file to write to.
void newKML() {
    strcpy(filename2, "KMLLOG00.KML");
  for (uint8_t i = 0; i < 100; i++) {
    filename2[6] = '0' + i/10;
    filename2[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename2)) {
      break;
    }
  }
  
  logfile = SD.open(filename2, FILE_WRITE);
  if( ! logfile ) {
    //Serial.print("Couldnt create "); //Serial.println(filename2);
    error(3);
  }
  //Serial.println("Writing to "); //Serial.println(filename2);
  
  char buffer[20];
  for (int i = 0; i < 43; i++)
  {
    strcpy_P(buffer, (char*)pgm_read_word(&(string_table[i]))); // Necessary casts and dereferencing, just copy. 
    logfile.print( buffer );
  }
  logfile.close();
}

void setup() {
  pinMode(loPin, INPUT);
  pinMode(ledPin, OUTPUT);
  // for Leonardos, if you want to debug SD issues, uncomment this line
  // to see serial output
  //while (!Serial);
  
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  //Serial.begin(115200);
  //Serial.println("\r\nUltimate GPSlogger Shield");
  

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  //if (!SD.begin(chipSelect, 11, 12, 13)) {
  if (!SD.begin(chipSelect)) {      // if you're using an UNO, you can use this line instead
    //Serial.println("Card init. failed!");
    error(2);
  }
  /*---------------------------------------------------------------------<indev code>
  strcpy(filename, "CONFIG.TXT");
  if(SD.exists(filename)){
    logfile = SD.open(filename, FILE_READ);
    EEPROM.write(0,logfile.read());
    logfile.close();
    SD.remove("CONFIG.TXT");
    error(4);//not an error, just a confirmation;
  }*/
  //---------------------------------------------------------------------</indev code>
  
  
  //for dumping raw nmea sentances (not useful if using KML files)
  /*strcpy(filename, "RAWLOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }*/
  
  newKML();
  
  // connect to the GPS at the desired rate
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For logging data, we don't suggest using anything but either RMC only or RMC+GGA
  // to keep the log files at a reasonable size
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 or 5 Hz update rate

  // Turn off updates on antenna status, if the firmware permits it
  GPS.sendCommand(PGCMD_NOANTENNA);
  
  //Serial.println("Ready!");
}

void loop() {
  if(!digitalRead(loPin)){//if we detect the power switch has been turned off, abandon the program to avoid lost data.
      return;
    }
  
  char c = GPS.read();

  //if (GPSECHO)
     //if (c) Serial.print(c);  
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    ////Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
        
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
    
    
    // Sentence parsed!
    //Serial.println("OK");
    if (LOG_FIXONLY && !GPS.fix) {
        //Serial.print("No Fix");
        return;
    }
    
    //write raw nmea sentances (not useful if using KML files)
    //Serial.println("raw");
    /*
    logfile = SD.open(filename, FILE_WRITE);
    if( ! logfile ) {
      //Serial.print("Couldnt create "); //Serial.println(filename);
      error(3);
    }
    //Serial.print("Writing to "); //Serial.println(filename);
   
    if(!digitalRead(loPin)){//if we detect the power switch has been turned off, abandon the program to avoid lost data.
      return;
    }
    
    char *stringptr = GPS.lastNMEA();
    uint8_t stringsize = strlen(stringptr);
    if (stringsize != logfile.write((uint8_t *)stringptr, stringsize))    //write the string to the SD file
      error(4);
    if (strstr(stringptr, "RMC"))   logfile.flush();
    //Serial.println();
    logfile.close();
    */
    
    double dLon = GPS.longitude;
    int iDeg = int(dLon/100.0);
    dLon = dLon-(iDeg*100.0);
    dLon *= 0.016666666667;
    dLon += iDeg;
    //if(GPS.lon == 'W')
      dLon *= -1.0;
    if(dLon > 180 || dLon < -180)//abort if the point can't exist.
      return;
      
    double dLat = GPS.latitude;
    iDeg = int(dLat/100.0);
    dLat = dLat-(iDeg*100.0);
    dLat *= 0.016666666667;
    dLat += iDeg;
    if(GPS.lat == 'S')
      dLat *= -1.0;
    if(dLat > 90.0 || dLat < -90.0)//abort if the point can't exist.
      return;
    
    double dAlt = GPS.altitude;
    
    double dSpeed = GPS.speed;
    
    //this section checks if there is an outlier in the altitude data
    long lAv = 0;
    long lAv2 = 0;
    for(int i = 0; i < averagePoints; i++){
      lAv += altHistory[i];
    }
    lAv /= averagePoints;
    
    for(int i = 0; i < (averagePoints-1); i++){
      altHistory[i] = altHistory[i+1];
    }
    altHistory[averagePoints-1] = dAlt*10;
    
    
    if((dAlt*10.0) > (lAv+584) || (dAlt*10) < (lAv-584)){//if current datapoint indicates an instantaneous climb/descent faster than 11,500 fpm (ie. outlier datapoint), discard the data.
      return;
    }
    
    lAv = 0;
    lAv2 = 0;
    
    //this section checks if there is an outlier in the position data
    for(int i = 0; i < averagePoints2; i++){
      lAv += latHistory[i];
    }
    lAv /= averagePoints2;
    
    for(int i = 0; i < averagePoints2; i++){
      lAv2 += lonHistory[i];
    }
    lAv2 /= averagePoints2;
    
      
    for(int i = 0; i < (averagePoints2-1); i++){
      latHistory[i] = latHistory[i+1];
    }
    latHistory[averagePoints2-1] = dLat*1000000;
    
    for(int i = 0; i < (averagePoints2-1); i++){
      lonHistory[i] = lonHistory[i+1];
    }
    lonHistory[averagePoints2-1] = dLon*1000000;
    
    //if((dAlt*10.0) > (lAv+584) || (dAlt*10) < (lAv-584))//if current datapoint indicates an instantaneous speed above 300 kt, discard the data
    //to use haversine function
   
    float fAv = lAv / 1000000.0;
    float fAv2 = lAv2 / 1000000.0;
    
    if(0.8 < HaverSine(dLat, dLon, fAv, fAv2))//if current datapoint indicates an instantaneous speed above 300 kt (ie. outlier datapoint), discard the data
      return;

    
    
    if(!digitalRead(loPin)){//if we detect the power switch has been turned off, abandon the program to avoid lost data.
      return;
    }
    digitalWrite(ledPin, HIGH);
    //Serial.println("parsed");
    logfile = SD.open(filename2, FILE_WRITE);
    if( ! logfile ) {
      //Serial.print("Couldnt create "); //Serial.println(filename2);
      error(3);
    }
    //Serial.println("Writing to "); //Serial.println(filename2);
    
    if(dLat == 0.0 && dLon == 0.0){
      
    }else{
dLon = -77.141273;
dLat = 43.989624;
dAlt = 300.00;
      logfile.seek(logfile.position()-60);
  
      logfile.print(dLon,6);
      logfile.print(',');
      logfile.print(dLat,6);
      logfile.print(',');
      logfile.println(dAlt);
    
      char buffer[20];
      for (int i = 38; i < 43; i++)
      {
        strcpy_P(buffer, (char*)pgm_read_word(&(string_table[i]))); // Necessary casts and dereferencing, just copy. 
        logfile.print( buffer );
      }
    
    }
    
    /*logfile.print(int(GPS.hour));
    logfile.print(':');
    logfile.print(int(GPS.minute));
    logfile.print(':');
    logfile.print(int(GPS.seconds));
    logfile.print('.');
    logfile.print(int(GPS.milliseconds));
    
    logfile.println();*/
    logfile.flush();
    logfile.close();
    delay(5);
    digitalWrite(ledPin, LOW);
  }
}


/* End code */

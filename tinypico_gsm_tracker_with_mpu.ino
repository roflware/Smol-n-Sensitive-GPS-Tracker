#include <Arduino.h>
#include <Wire.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>

/*
MotionDetector Section
=====================================
*NOTE: The registers used in this have put my mpu6050 to a permanant interrupt
* sleep I cannot recover it from.  It will still work in interrupt mode but nothing else.

Wiring
======
MPU6050    GY-521(MPU6050)
3.3V       VCC ( GY-521 has onboard regulator 5v tolerant)
GND        GND
21         SDA
22         SCL
15         INT - Note below on allowed RTC IO Pins
*/


/*
Current Usage
=====================================
// 5.37mA with GY-521 LED attached
// 3.96mA with Led Broken/removed, Temp and Gyro still active, high speed parsing
// 0.71mA with Broken LED, Gyro's disabled
// 0.65mA with Broken LED, Gyro's disabled, Temp sensor disabled
*/

/*
Deep Sleep with External Wake Up
=====================================
NOTE:
======
Only RTC IO can be used as a source for external wake
source. They are pins: 0,2,4,12-15,25-27,32-39.
*/

// MPU registers
#define SIGNAL_PATH_RESET  0x68
#define CONFIG        0x1A
#define GYRO_CONFIG        0x1B
#define ACCEL_CONFIG       0x1C
#define MOT_THR            0x1F  // Motion detection threshold bits [7:0]
#define MOT_DUR            0x20  // This seems wrong // Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
#define MOT_DETECT_CTRL    0x69
#define INT_PIN_CFG        0x37
#define INT_ENABLE         0x38
#define INT_STATUS         0x3A
#define PWR_MGMT_1         0x6B
#define PWR_MGMT_2         0x6C
#define INT_STATUS 0x3A
#define MPU6050_ADDRESS 0x68 //AD0 is 0
#define WHO_AM_I           0x75

// End MPU deep sleep section

// GPS Section

TinyGPS gps;
SoftwareSerial ss(32, 33);

// End GPS Section

// SIM800 Section

#define SIM800L_RX     27
#define SIM800L_TX     26
#define SIM800L_PWRKEY 4
#define SIM800L_RST    5
#define SIM800L_POWER  23
#define LED_GPIO       13
#define LED_ON         HIGH
#define LED_OFF        LOW

String apn = "";               //APN
String apn_u = "";                     //APN-Username
String apn_p = "";                     //APN-Password
String url = "https://URL_HERE";  //URL of Server

// GSM location variables
String incoming = "";
String response = "";
String cellID = "";
String lac = "";
int gsmCheckRan = 0;
int gsmCENGRan = 0;

// End SIM800 Section

const int transistor = 25;


/*    Example for using write byte
      Configure the accelerometer for self-test
      writeByte(MPU6050_ADDRESS, ACCEL_CONFIG, 0xF0); // Enable self test on all three axes and set accelerometer range to +/- 8 g */
void writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
  Wire.begin(21, 22);
  Wire.beginTransmission(address);  // Initialize the Tx buffer
  Wire.write(subAddress);           // Put slave register address in Tx buffer
  Wire.write(data);                 // Put data in Tx buffer
  Wire.endTransmission();           // Send the Tx buffer
}

//example showing using readbytev   ----    readByte(MPU6050_ADDRESS, GYRO_CONFIG);
uint8_t readByte(uint8_t address, uint8_t subAddress)
{
  uint8_t data;                            // `data` will store the register data
  Wire.beginTransmission(address);         // Initialize the Tx buffer
  Wire.write(subAddress);                  // Put slave register address in Tx buffer
  Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
  Wire.requestFrom(address, (uint8_t) 1);  // Read one byte from slave register address
  data = Wire.read();                      // Fill Rx buffer with result
  return data;                             // Return data read from slave register
}

void setupSleep() {
  Serial.println("Writing bytes to MPU...");
  writeByte( MPU6050_ADDRESS, PWR_MGMT_1, 0b00001000); // Cycle & disable TEMP SENSOR
  writeByte( MPU6050_ADDRESS, PWR_MGMT_2, 0b11000111); // Disable Gyros, 40MHz sample rate

  //writeByte( MPU6050_ADDRESS, CONFIG, 0b00000000 ); // disable filtering for max sensitivity
  writeByte( MPU6050_ADDRESS, ACCEL_CONFIG, 0b00000000 );
  writeByte( MPU6050_ADDRESS, ACCEL_CONFIG, 0b00000100 );
  writeByte( MPU6050_ADDRESS, MOT_THR, 20);  //Write the desired Motion threshold to register 0x1F (For example, write decimal 20).
  writeByte( MPU6050_ADDRESS, MOT_DUR, 1);  //Set motion detect duration to 1  ms; LSB is 1 ms @ 1 kHz rate
  writeByte( MPU6050_ADDRESS, MOT_DETECT_CTRL, 0b00000000); //to register 0x69, write the motion detection decrement and a few other settings (for example write 0x15 to set both free-fall and motion decrements to 1 and accelerometer start-up delay to 5ms total by adding 1ms. )
  writeByte( MPU6050_ADDRESS, INT_PIN_CFG, 0b00100000 ); // now INT pin is active high, cleared on read
  writeByte( MPU6050_ADDRESS, INT_ENABLE, 0b01000000 ); //write register 0x38, bit 6 (0x40), to enable motion detection interrupt.
  readByte(MPU6050_ADDRESS, INT_STATUS);

  Serial.println("Going to sleep!");

  esp_sleep_enable_ext1_wakeup((uint64_t)(1<<15),ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();
  
  //unless the deep sleep code above is commented this won't run

  Serial.printf("0x%02X: %02X\r\n", WHO_AM_I, readByte(MPU6050_ADDRESS, WHO_AM_I));
  Serial.printf("0x%02X: %02X\r\n", INT_ENABLE, readByte(MPU6050_ADDRESS, INT_ENABLE));
  Serial.printf("0x%02X: %02X\r\n", INT_PIN_CFG, readByte(MPU6050_ADDRESS, INT_PIN_CFG));
  Serial.printf("0x%02X: %02X\r\n", MOT_THR, readByte(MPU6050_ADDRESS, MOT_THR));
  Serial.printf("0x%02X: %02X\r\n", MOT_DUR, readByte(MPU6050_ADDRESS, MOT_DUR));
  Serial.printf("0x%02X: %02X\r\n", 0x6B, readByte(MPU6050_ADDRESS, 0x6B));
  Serial.printf("0x%02X: %02X\r\n", 0x6C, readByte(MPU6050_ADDRESS, 0x6C));
  Serial.printf("0x%02X: %02X\r\n", INT_STATUS, readByte(MPU6050_ADDRESS, INT_STATUS));

  // End the setting up the sleep section for the MPU and entering a deep sleep with the ESP32
}

void setup(){

  // Initiate sleep at the start, and let the MPU movement be a trigger to start up the SIM800 and GPS modules
  // This is only run after first detecting movement, then goes to sleep
  pinMode(2, OUTPUT);
  pinMode(0, INPUT);

  // Setup transistor to turn the GPS on when the device wakes up from sleep
  pinMode (transistor, OUTPUT);
  digitalWrite (transistor, HIGH);
  digitalWrite (34, HIGH);
  digitalWrite (35, HIGH);

  // Setup SIM800 pins
  pinMode(SIM800L_POWER, OUTPUT);
  digitalWrite(SIM800L_POWER, HIGH);

  Serial.begin(9600);
  delay(50);
  
  digitalWrite(2, digitalRead(15));

  // This starts everything
  if (digitalRead(15)) {
    ss.begin(9600);
    Serial.println("ESP32+SIM800L AT CMD Test");
    Serial2.begin(9600, SERIAL_8N1, SIM800L_TX, SIM800L_RX);
    
    delay(15000);
    while (Serial2.available()) {
      //Serial.println("Hi, how are you?"); // This is where we will start the SIM800 and GPS
      Serial.write(Serial2.read());
    }
    delay(2000);

    gsm_config_gprs();

    // Not sure why the pins are reading HIGH when there is no device connected to them...
    if (digitalRead(32) || digitalRead(33)) {
      // Begin GPS portion
      Serial.println("Letting the GPS warm up for 60 seconds...");
      delay(60000); // revert back to 60000
      // GPS Start
      Serial.println("Starting GPS... now!");
      displayInfo();
    } else {
      Serial.println("Starting SIM800L location check... now!");
      gsmGetLocation();
    }
  }
  setupSleep();
}

void prepare_message()
{
  //Sample Output: +CENG: 0,"0685,50,00,310,260,00,8c53,10,03,cbac,255"
  
  // May need to add something here that says to only listen if starting with +CENG
  
  int first_comma = response.indexOf(','); //Find the position of 1st comma
  int second_comma = response.indexOf(',', first_comma+22);
  int third_comma = response.indexOf(',', second_comma+4);

  int whatever = response.indexOf(',', second_comma+10);
  int fourth_comma = response.indexOf(',', third_comma+10);

  /*for(int i=first_comma+1; i<second_comma; i++) //Values from 1st comma to 2nd comma is Longitude 
    Longitude = Longitude + response.charAt(i);*/

  for(int i=second_comma+1; i<third_comma; i++)
    cellID = cellID + response.charAt(i);

  for(int i=whatever+1; i<fourth_comma; i++)
    lac = lac + response.charAt(i);

  Serial.printf("CellID: %s\n", cellID);
  Serial.printf("LAC: %s\n", lac);

  //String apiKey = "pk.7fd374bfa8c8b09e436d21bb9aa9a008";

  // End the engineering session so it doesn't keep repeating CENG
  gsm_send_serial("AT+CENG=0,0");
  gsm_http_post("key=jakndsCSADY8743njusdfJFAS&cellid=" + String(cellID) + "&lac=" + String(lac));
}

void gsmGetLocation() {

  if(gsmCENGRan == 0) {
    gsm_send_serial("AT+SAPBR=1,1");
    gsm_send_serial("AT+SAPBR=2,1");
    gsm_send_serial("AT+COPS?");
    gsm_send_serial("AT+CENG=2,0");
    gsm_send_serial("AT+CREG?");
    gsmCENGRan = 1;
  }

    delay(3000);

    if(gsmCheckRan == 0) {
         response = "";
         cellID="";
         lac="";
         
          while (Serial2.available()) 
          {
           char letter = Serial2.read();
           response = response + String(letter); //Store the location information in string response 
          }

         prepare_message();

         gsmCheckRan = 1;
         gsmCENGRan = 1;
    }
}

void loop(){
  
  digitalWrite(2, digitalRead(15));

  // Constantly checking, waiitng for a motion interrupt on pin 15
  if (!digitalRead(0)) {
    readByte(MPU6050_ADDRESS, INT_STATUS); // Check the status of the MPU's interrupt pin -- do stuff in setup if so
  }
}

void gsm_http_post(String postdata) {
  Serial.println(" --- Start GPRS & HTTP --- ");
  gsm_send_serial("AT+SAPBR=1,1");
  gsm_send_serial("AT+SAPBR=2,1");
  gsm_send_serial("AT+HTTPINIT");
  gsm_send_serial("AT+HTTPSSL=1"); // Enable HTTPS support
  gsm_send_serial("AT+HTTPPARA=CID,1");
  gsm_send_serial("AT+HTTPPARA=URL," + url);
  gsm_send_serial("AT+HTTPPARA=CONTENT,application/x-www-form-urlencoded");
  gsm_send_serial("AT+HTTPDATA=192,5000");
  gsm_send_serial(postdata);
  gsm_send_serial("AT+HTTPACTION=1");
  gsm_send_serial("AT+HTTPREAD");
  gsm_send_serial("AT+HTTPTERM");
  gsm_send_serial("AT+SAPBR=0,1");

  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, LED_OFF);

  // Put the SIM800L to sleep
  Serial.println("Sleeping the SIM800L...");
  gsm_send_serial("AT+CPOWD=1");

  // Turn off the GPS via the transistor when the device is going to sleep
  digitalWrite (transistor, LOW);
  digitalWrite (34, LOW);
  digitalWrite (35, LOW);
  
  delay(2000);
  Serial.println("Okay, nighty night!");
  // Initialize this on the start
  setupSleep();
  /* Figure out how to tell it to go back to sleep once all done */
}

void gsm_config_gprs() {
  Serial.println(" --- CONFIG GPRS --- ");
  gsm_send_serial("AT+SAPBR=3,1,Contype,GPRS");
  gsm_send_serial("AT+SAPBR=3,1,APN," + apn);
  if (apn_u != "") {
    gsm_send_serial("AT+SAPBR=3,1,USER," + apn_u);
  }
  if (apn_p != "") {
    gsm_send_serial("AT+SAPBR=3,1,PWD," + apn_p);
  }

  // Turn off SIM800L light
  gsm_send_serial("AT+CNETLIGHT=?");
  gsm_send_serial("AT+CNETLIGHT=0;&W"); // or just 0, but we will see

  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, LED_ON);
}

void gsm_send_serial(String command) {
  Serial.println("Send ->: " + command);
  Serial2.println(command);
  long wtimer = millis();
  while (wtimer + 3000 > millis()) {
    while (Serial2.available()) {
      Serial.write(Serial2.read());
    }
  }
  Serial.println();
}

// GPS Start
void displayInfo()
{
  bool newData = false;

  // Add logic to check and see if a lock on the tower was obtained

  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  float flat, flon;
  unsigned long age;

  if (newData)
  {
    gps.f_get_position(&flat, &flon, &age);
    Serial.print("LAT=");
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print(" LON=");
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    Serial.print(" SAT=");
    Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
    Serial.print(" PREC=");
    Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
    Serial.print("\n");
  }

  unsigned int satCount = gps.satellites();

  if(satCount > 0 && satCount < 100) { // 255 satellites means 0, apparently
    Serial.printf("Lock obtained on %u satellites!\n", satCount);
    gsm_http_post("key=KEY_HERE&lat=" + String(flat, 6) + "&lon=" + String(flon, 6));
  } else {
    Serial.printf("Bummer! There are no satellites! Resorting to GSM location request\n");
    // Get location via GSM and send to web server
    gsmGetLocation();
  }

  delay(1000);
}
// GPS End

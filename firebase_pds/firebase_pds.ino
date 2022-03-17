#include <WiFiNINA.h>
#include "arduino_secrets.h"            //contains wifi credentials
#include "Firebase_Arduino_WiFiNINA.h"  //Firebase Arduino based on WiFiNINA-install from manage libraries
#include <SPI.h>
#include <MFRC522.h>

#define LMotorPin1 2
#define LMotorPin2 3

#define RMotorPin1 4
#define RMotorPin2 5

#define FIREBASE_HOST "pds-assignment-2f736-default-rtdb.asia-southeast1.firebasedatabase.app"  //Firebase Realtime database URL
#define FIREBASE_AUTH "aiam0ZKRi5K0h0vRxsfZ22KhPuJ75HcsTB37PYK4" //from Firebase Database secrets

#define RST_PIN 24
#define SS_PIN 11

enum direction{forward, backward, left, right, halt};
String robotId = "robot_0";
String robotState; 
String tableId;
bool foundTable = false;
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status
//Define Firebase data object

FirebaseData firebaseData;
String robot_id = "001";
String path = "/robot" + robot_id; 


uint32_t delayMS;
int timestamp;

void move(direction dir, int duration);
void getStateAndTableId();
void navigateAndDeliverFood();
void dump_byte_array(byte *buffer, byte bufferSize);

void setup() {
  //initialize motor pins 
  pinMode(LMotorPin1, OUTPUT); 
  pinMode(LMotorPin2, OUTPUT); 
  pinMode(RMotorPin1, OUTPUT); 
  pinMode(RMotorPin2, OUTPUT); 

   
  //Initialize serial and wait for port to open:

  Serial.begin(9600);
//  while (!Serial);  // comment this line if not connected to computer and Serial Monitor

  // Set delay between sensor readings based on sensor details.

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(5000);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network!");
  
  Serial.println("----------------------------------------");
  printData();
  Serial.println("----------------------------------------");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH, SECRET_SSID, SECRET_PASS);

  Firebase.reconnectWiFi(true); //Let's say that if the connection is down, try to reconnect automatically
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println(F("Scan a MIFARE Classic PICC to demonstrate read and write."));
    Serial.print(F("Using key (for A and B):"));
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  getStateAndTableId();
}

void loop() {
//  Serial.println(robotState);
  getStateAndTableId();
  if (robotState == "home")
  {
    Serial.println("robot is at home state");
    move(halt, 5000); 
  }
  else if (robotState == "delivery")
  {
    Serial.println("robot is at delivery state");
    navigateAndDeliverFood();       
  }   
}

void printData() {
  
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void move(direction dir, int duration)
{
  int Lpin1Val, Lpin2Val, Rpin1Val, Rpin2Val;
  switch(dir)
  {
    case forward:
        Serial.println("forward");
        digitalWrite(LMotorPin1, HIGH);
        digitalWrite(LMotorPin2, LOW);
        digitalWrite(RMotorPin1, HIGH);
        digitalWrite(RMotorPin2, LOW);
        break;
    case backward: 
        digitalWrite(LMotorPin1, LOW);
        digitalWrite(LMotorPin2, HIGH);
        digitalWrite(RMotorPin1, LOW);
        digitalWrite(RMotorPin2, HIGH);
        break;
    case left: 
        Serial.println("left");
        digitalWrite(LMotorPin1, HIGH);
        digitalWrite(LMotorPin2, LOW);
        digitalWrite(RMotorPin1, LOW);
        digitalWrite(RMotorPin2, HIGH);
        break;
    case right: 
        Serial.println("right"); 
        digitalWrite(LMotorPin1, LOW);
        digitalWrite(LMotorPin2, HIGH);
        digitalWrite(RMotorPin1, HIGH);
        digitalWrite(RMotorPin2, LOW);
        break;  
    case halt: 
        Serial.println("halt"); 
        digitalWrite(LMotorPin1, LOW);
        digitalWrite(LMotorPin2, LOW);
        digitalWrite(RMotorPin1, LOW);
        digitalWrite(RMotorPin2, LOW);
        break;
  }
  delay(duration); 
}

void getStateAndTableId()
{
  Serial.println("getting state and table id");
  if (Firebase.getString(firebaseData, path + "/robotState"))
  {
    if (firebaseData.dataType() == "string")
      Serial.println(firebaseData.stringData()); 
      Serial.println("saving robot state");
      robotState=firebaseData.stringData();
  }
  else
  {
    Serial.println("ERROR: " + firebaseData.errorReason());
    Serial.println();
  }

  if (Firebase.getString(firebaseData, path + "/tableId"))
  {
    if (firebaseData.dataType() == "string")
      Serial.println(firebaseData.stringData()); 
      tableId=firebaseData.stringData();
  }
  else
  {
    Serial.println("ERROR: " + firebaseData.errorReason());
    Serial.println();
  }
}

void navigateAndDeliverFood()
{
  while (!foundTable)
  {
    move(forward, 100); 
    tableCheck(); 
  }
  Serial.println("returning home");
  returnHome();
}

void returnHome()
{
  move(backward, 100);
  if (Firebase.setString(firebaseData, path + "/robotState", "home"))
    {
      
      Serial.println("Insert");
      Serial.println("PATH: " + firebaseData.dataPath());
      Serial.println("TYPE: " + firebaseData.dataType());
      Serial.print("VALUE: ");
      if (firebaseData.dataType() == "string") 
        Serial.println(firebaseData.stringData()); 
    }
    else
    {
      Serial.println("ERROR : " + firebaseData.errorReason());
      Serial.println();
    }
  foundTable = false;
  
}

void tableCheck(){
   Serial.println("checking table");
   if ( ! mfrc522.PICC_IsNewCardPresent()) {
      Serial.println("no new card present");
      return;
   }

   // Select one of the cards
   if ( ! mfrc522.PICC_ReadCardSerial()) {
      Serial.println("no card detected");
      return;
   }

   char detectedTableId[32] = "";
   array_to_string(mfrc522.uid.uidByte, 4, detectedTableId); //Insert (byte array, length, char array for output)
   Serial.println(detectedTableId);
   delay(2000);
   if (tableId == detectedTableId)
   {
      foundTable = true;
   }
   mfrc522.PICC_HaltA();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void array_to_string(byte array[], unsigned int len, char buffer[])
{
   for (unsigned int i = 0; i < len; i++)
   {
      byte nib1 = (array[i] >> 4) & 0x0F;
      byte nib2 = (array[i] >> 0) & 0x0F;
      buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
      buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
   }
   buffer[len*2] = '\0';
}

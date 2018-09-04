#include <SH1106.h>
#include <SSD1306Spi.h>
#include <OLEDDisplayFonts.h>
#include <OLEDDisplay.h>
#include <SSD1306.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>

#include <OneWire.h>
#include <DallasTemperature.h>

//#include "SSD1306.h"
#include "image.h"
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6

// Data wire is plugged into GPIO 15 on the ESP32
#define ONE_WIRE_BUS 15
#define TEMPERATURE_PRECISION 12 // Lower resolution

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors (&oneWire);

int numberOfDevices; // Number of temperature devices found

// A Variable to hold the temperature you retrieve
float tempC;

unsigned int counter = 0;
//unsigned int device = 0;

//address,SDA,SCL
SSD1306 display(0x3c, 21, 22);
String rssi = "RSSI --";
String packSize = "--";
//String packet ;

DeviceAddress tempDeviceAddress = {0x28, 0xFF, 0x2B, 0x45, 0x4C, 0x04, 0x00, 0x10}; // We'll use this variable to store a found device address

void setup(void)
{

  pinMode(16,OUTPUT);
  pinMode(2,OUTPUT);
  
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  
  // start serial port
  Serial.begin(9600);

  while (!Serial);
  Serial.println();
  Serial.println("LoRa Sender Test");
  
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("init ok");
  display.init();
  display.flipScreenVertically();  
  display.setFont(ArialMT_Plain_10);

//  delay(1500);

  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();
  
  // Grab a count of devices on the wire
    //numberOfDevices = sensors.getDeviceCount();
    numberOfDevices = 2;

  
  // locate devices on the bus
  Serial.print("Locating devices...");
  
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
  {
    Serial.print("Found device ");
    Serial.print(i, DEC);
    Serial.print(" with address: ");
    printAddress(tempDeviceAddress);
    Serial.println();
    
    Serial.print("Setting resolution to ");
    Serial.println(TEMPERATURE_PRECISION, DEC);
    
    // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
    sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
    
     Serial.print("Resolution actually set to: ");
    Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
    Serial.println();
  }else{
    Serial.print("Found ghost device at ");
    Serial.print(i, DEC);
    Serial.print(" but could not detect address. Check power and cabling");
  }
  }
  delay(1500);
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.println(tempC);
  Serial.print("Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
  Serial.println(" ");
}



void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // Loop through each device, print out temperature data
  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
  {
    // Output the device ID
    Serial.print("Temperature for device: ");
    Serial.println(i,DEC);
    // It responds almost immediately. Let's print out the data
    printTemperature(tempDeviceAddress); // Use a simple function to print out the data
    sendTemp(tempDeviceAddress, i);//send the device temp to the receiving 
    //delay(1500);
  } 
  //else ghost device! Check your power requirements and cabling
  }
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


void sendTemp(DeviceAddress deviceAddress, int i){

      display.clear();
      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.setFont(ArialMT_Plain_16);
  
      display.drawString(64, 0, "Sending packet: ");
      display.drawString(64, 18, String(counter));

      
      display.setFont(ArialMT_Plain_10);
      display.drawString(64, 40, "C: " + String(tempC)+ "   |   F: " + String(DallasTemperature::toFahrenheit(tempC)));
      display.display();

      // send packet
      LoRa.beginPacket();
      
      LoRa.print("Packet: ");
      LoRa.print(counter);
      LoRa.print("                 ");
      
      LoRa.print("Device: ");
      LoRa.print(i);
      LoRa.print("                       ");

      LoRa.print("Temp: ");
      LoRa.print(tempC);
      LoRa.print(" C, ");
      LoRa.print(DallasTemperature::toFahrenheit(tempC));
      LoRa.print(" F");
      
      LoRa.endPacket();

      counter++;
      digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(1000);                       // wait for a second
      digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
      delay(1000); 
  
}


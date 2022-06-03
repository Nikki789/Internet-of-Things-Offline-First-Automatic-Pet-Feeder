//This library allows the use of pin no. 10, 11 as Rx, Tx. 
#include <SoftwareSerial.h>      

//Sets the Rx and the Tx on the Arduino Uno board (Rx ==> Pin 10; Tx ==> Pin 11) 
SoftwareSerial esp8266(10,11);

/* Makes a constant named "DEBUG" and sets its value to true
It is used later for debugging purposes */
#define DEBUG true  

//Library to communicate with DS3231
#include <Wire.h>

//Real time clock library from https://github.com/rodan/ds3231 to read/write current time
#include <ds3231.h>

struct ts t;

//Servo motor library 
#include <Servo.h>

Servo servo;
int angle = 50;

void setup() {
  //The software and hardware serial communication speed(baud rate) is set to 9600
  Serial.begin(9600);
  esp8266.begin(9600);

  //This function initializes a communication between the ESP8266 and the hotspot 
  InitWifiModule();

  //Code used to initialize the RTC module
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);

  /* Code used to synchronize the RTC submodule 
  It is currently commented as the clock is already syncronized
  
  t.hour=20; 
  t.min=31;
  t.sec=0;
  t.mday=21;
  t.mon=05;
  t.year=2022;
 
  DS3231_set(t); */
}


void loop() {

//Checks if there's any data received and stored in the serial receiver buffer 
   if(esp8266.available()) {    
    //Searches for the "+IPD," string in the incoming data
    if(esp8266.find("+IPD,")) {

     // Delay is added to fill the buffer with the data (1s)      
     delay(1000);

     //Subtracts 48 because the read() function returns the ASCII decimal value and 0 (the first decimal number) starts at 48 
     //Converts from ASCI decimal value to a character value  
     int connectionId = esp8266.read()-48;

     //Advances the cursor to the "pin=" part in the request header to read the incoming bytes  
     esp8266.find("pin="); 

     //Reads the first byte from the Arduino input buffer
     //If the pin is 12 then the 1st number is 1. Then multiplies this number by 10, ending with the final value of the "pinNumber" variable 10.                                                      
     int pinNumber = (esp8266.read()-48)*10;

     //Reads the second byte from the Arduino input buffer, in this case 2 and adds it to the "pinNumber" variable
     pinNumber = pinNumber + (esp8266.read()-48);

     //Reads the third byte from the Arduino input buffer. Saves it inside the "chosenTime" variable
     long int chosenTime =(esp8266.read()-48);

    //Prints the "connectionId, pinNumber and the chosenTime" values on the serial monitor for debugging purposes 
     Serial.println(connectionId);

     Serial.print(pinNumber); 

     Serial.print("      ");

     Serial.println(chosenTime);

     //Closes the TCP/IP connection    
     String closeCommand = "AT+CIPCLOSE="; 

     //Appends the connection id to the string
     closeCommand+=connectionId; 

     //Appends the "\r\n" to the string; it simulates the keyboard enter press     
     closeCommand+="\r\n";    

     //Sends the command to the ESP8266 module to execute it    
     sendData(closeCommand,3000,DEBUG);
    }
  }

  //Prints the time from the Real Time clock submodule 
  DS3231_get(&t);
  Serial.print("Date : ");
  Serial.print(t.mday);
  Serial.print("/");
  Serial.print(t.mon);
  Serial.print("/");
  Serial.print(t.year);
  Serial.print("\t Hour : ");
  Serial.print(t.hour);
  Serial.print(":");
  Serial.print(t.min);
  Serial.print(".");
  Serial.println(t.sec);

 //Delay is added after each print (10sec)
  delay(10000);

  /* This code moves the servo motor at the specified time (for example, 15:30)
  Delays were added to be able to assure it doesn't move too fast and appropriate amount of food is dropped */
  if ((t.hour == 21) && (t.min == 20) && t.sec >= 0 && t.sec <= 10) {
    servo.attach(8);

    for(angle = 120; angle > 80; angle--) {                                
      servo.write(angle);           
      delay(15);       
    } 
    delay(15);
    for(angle = 100; angle < 120; angle++) {                                  
      servo.write(angle);               
      delay(15);                   
    }
  } 
}

//This function regulates how the AT Commands will get sent to the ESP8266 submodule
String sendData(String command, const int timeout, boolean debug) {
    String response = "";  
    //Sends the AT command from ARDUINO to the ESP8266                                                 
    esp8266.print(command);

    //Gets the current time and saves it inside the "time" variable      
    long int time = millis();      

    //Executes within 1 second
    while( (time+timeout) > millis()) {   
      //Checks if there is any response from the ESP8266
      while(esp8266.available()) {
        //Reads the next character from the input buffer and saves it in the "response" String variable  
        char c = esp8266.read();  
        //Appends the next character to the response variable. All will be contained as a string(array of characters) showing the response                                 
        response+=c;                                                  
      }  
    }    
    //Prints the response on the Serial monitor
    if(debug) {
      Serial.print(response);
    }    
    return response;                                                  
}

/* This function gives the commands that we need to send to the sendData() function. 
Delays were added to make sure that the Wi-Fi connection is made */
void InitWifiModule() {
  //Resets the ESP8266 module
  sendData("AT+RST\r\n", 2000, DEBUG);   

  //Connects to the Wi-Fi network                         
  sendData("AT+CWJAP=\"up899244\",\"up899244\"\r\n", 5000, DEBUG);        
  delay (5000);

  //Sets the ESP8266 WiFi mode to station mode
  sendData("AT+CWMODE=1\r\n", 2000, DEBUG);                                             
  delay (1500);

  //Shows the IP address and the MAC address
  sendData("AT+CIFSR\r\n", 2000, DEBUG);                                             
  delay (1500);

  //Multiple connections
  sendData("AT+CIPMUX=1\r\n", 2000, DEBUG);                                             
  delay (1500);

  //Starts the communication at port 80, port 80 used to communicate with the web servers through the http request
  sendData("AT+CIPSERVER=1,80\r\n", 2000, DEBUG);      
}

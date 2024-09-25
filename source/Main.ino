#include<WiFi.h>
#include<WebServer.h>
#include <Keypad_I2C.h>
#include <Keypad.h>        
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include<SPI.h>
#include<MFRC522.h>


#define greenLED 32 ;
#define redLED 25;
#define button 4;
#define lock 14;

#define I2CADDR 0x20   //I2C address

MFRC522 rfid(SS_PIN,RST_PIN);         //RFID instance
#define SS_PIN 5               
#define RST_PIN 27

LiquidCrystal_I2C lcd(0x27, 20, 4);   //LCD instance

String cardID="";   //Read user's cardID

//Messages for output
String msg = "ENTER PW : ";
String msgchange = "\\CHANGING PW\\";
String msgnew = "ENTER NEW PW :";

//Passwords
char *PW1 = "0000";
String PW2 = "1234";

char key,prevkey='N'; // N stands for NO_KEY

//variables to manipulate the LCD's cursor
int i, j, k = 0;

int change = 0;
bool pwchange = false;
String readpw = "";

const byte ROWS = 4;
const byte COLS = 3;

//Keypad Config
char kpad[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {0, 1, 2, 3};
byte colPins[COLS] = {4, 5, 6};

Keypad_I2C kypad( makeKeymap(kpad), rowPins, colPins, ROWS, COLS, I2CADDR); //Keypad instance

//Wifi Config
const char* ssid="TT_62F8";
const char* password="gannjvd19b";

WebServer server(80);
char LEDState[2][10]={"Unlocked","Locked"};
int state=1;
char c;

void Home(){
  String site="<!DOCTYPE html>";
 site+="<html>";
 site+="  <head>";
 site+="          <title> esp32 WS</title>";
       "          <meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8' />";
 site+="  <style>";
 site+="            .button{";
 site+="                     border: none;";
 site+="                     color:white;";
 site+="                     display:inline-block;";
 site+="                     text-align:center;";
 site+="                     font-size:20px;";
 site+="                     padding: 10px 200px;";
 site+="                     cursor:pointer;}";

 site+="          .button1{background-color: #4CAF50;margin: 20px 2px; }";
 site+="          .button2{background-color: #FF0000;margin: 20px 2px; }";
 site+="  </style>";
 site+="  </head>";
 site+="          <body>";
 site+="                <p style='background-color: powderblue;text-align:center;font-size:160%'><br>Door : " ;site+=LEDState[state];
 site+="                <img src='C:\Users\USER\Desktop\unlock_icon.png' height='30' width='30' class='img'><br>";
 site+="                <br><a href='/lock' class='button button2'> Lock </a>";
 site+="                <a href='/unlock' class='button button1'>  Unlock </a>";
 site+="                </p>";
 site+="          </body>";
 site+="</html>";

  server.setContentLength(site.length());
  server.send(200,"text/html",site); }
  
void Lock(){
  state=1;
  digitalWrite(greenLED,LOW);
  server.sendHeader("Location","/");
  server.send(303);}
  
 void Unlock(){  
  state=0;
  digitalWrite(greenLED,HIGH);
  server.sendHeader("Location","/");
  server.send(303); }
  
 void NotFound(){
  server.send(404,"text/plain", "404: Not found"); }

void openDoor(){
  digitawlWrite(lock, HIGH);
  }


  
void setup() {
  
  Serial.begin(115200);
  delay(1000);

  attachInterrupt(digitalPinToInterrupt(button), openDoor, RISING);
  WiFi.begin(ssid,password);
  Serial.print("Tentative de connexion..");
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(100);}
  Serial.println("\nConnexion etablie!");
  Serial.println("Adress IP: ");
  Serial.println(WiFi.localIP());
  
  server.on("/lock", Lock);
  server.on("/unlock", Unlock);
  server.on("/",Home);
  server.onNotFound(NotFound);
  server.begin();
  Serial.println("Serveur web actif");

  kypad.begin();
  lcd.begin();
  delay(500);
  pinMode(greenLED,OUTPUT);
  pinMode(redLED, OUTPUT);
  digitalWrite(redLED, HIGH);
  Wire.begin();

  SPI.begin(); //init SPI bus
  rfid.PCD_Init();
  
}

void loop() {
    
    if(rfid.PICC_IsNewCardPresent()) //Look for new cards
       RFID();
  
      server.handleClient(); //For website's queries 
      switch(change){
        
        case 0 : //getting password
          gettingpw_0();
          break;
       
        case 1:  //changing password
          changepw_1();
          break;
          
        case 2:  //Ancient password verification to save new pw
          ChangePWTest_2();
          break;
        
        case 3:  //Getting new password
         getNewPW_3();
         break;
         
        case 4: //testing pw (Used for both: opening door or ancient password testing) 
         PWTest_4();
         break;
       
        }
}
void gettingpw_0(){
        
         i=0;
         j=0;
         lcd.setCursor(i,j++);
         lcd.print("ENTER PW :");
         //getting pw
         if((key = kypad.getKey())!=0){
          lcd.setCursor(k++,j);
          lcd.print("*");
          digitalWrite(redLED,HIGH);
          delay(100);
          digitalWrite(redLED,LOW);
             
             if((readpw == "*")&&(key=='#')){   //changing pw
                                        
                 k=0;
                 i=0;
                 readpw="";
                 lcd.clear();
                 change=1;
                 }
                
                
              else if((prevkey=='*')&&(key=="*"))  //clear password when getting "**"
              {
              lcd.clear();
              digitalWrite(greenLED,HIGH);
              delay(100);
              digitalWrite(greenLED,LOW);
              delay(100);
              digitalWrite(redLED,HIGH);
              delay(100);
              digitalWrite(redLED,LOW);
              delay(100);
              digitalWrite(greenLED,HIGH);
              delay(100);
              digitalWrite(greenLED,LOW);
              delay(100);
              digitalWrite(redLED,HIGH);
              lcd.setCursor(i=0,j=0);
              readpw="";
              k=0;
                  }
              }
              
              //"#" to confirm
              else if(key=='#'){
                lcd.clear();
                lcd.setCursor(5,2);
                change=4; }
                
              //If "**" neither "#" was detected -> get next key
              else{
                readpw=readpw+key;
                prevkey=key;}
    }}
  
  void changepw_1(){      //getting ancient password
              
                lcd.setCursor(i=4,j=0);
                lcd.print(msgchange);
                lcd.setCursor(i=0,j=1);
                lcd.print(msg);
                

                if((key=kypad.getKey())!=0){
                 if(key=='#'){
                  change =2;
                  }
                  
                 else{
                    lcd.setCursor(k++,j=2);
                     lcd.print("*");
                     readpw= readpw + key;
                       }
                }
  }
  
  void ChangePWTest_2(){     //Testing ancient password

    //If the right ancient password was insert -> preparing to get the new password
    if(readpw==PW2){
                  readpw="";
                  lcd.clear();
                  lcd.setCursor(5,0);
                  lcd.print(msgchange);
                  lcd.setCursor(i=0,j=1);
                  lcd.print(msgnew);
                  j++;
                  k=0;
                  change=3;}

    //If wrong password -> Cancel              
    else{
                  lcd.clear();
                  change=0;}
  }
  
  void getNewPW_3(){        //Getting and saving new password
    
    if((key=kypad.getKey())!=0){
                      
                   lcd.setCursor(k++,j);
                    if(key=='#'){
                      if(readpw !=""){
                        PW2=readpw;
                        pwchange=true;
                        lcd.clear();
                        lcd.setCursor(5,2);
                        change = 4;}
                       else{
                        lcd.clear();
                        change = 0;}
                       }
                   else{
                       lcd.print(key);
                       readpw = readpw + key;
                       }
                     
                    }
  }
  
  void PWTest_4(){  //ACCEPTION or DENIAL of [ changing the password | or opening the door ]
          
    //------------if pw has been changed
          if(pwchange){
            digitalWrite(greenLED,HIGH);
            delay(1500);
            digitalWrite(greenLED,LOW);
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(PW2);
            delay(2000);
            pwchange=false;

            }
    //-----------password verification to open the door
          else {
          if((readpw==PW1)||(readpw==PW2)){
          lcd.println("ACCEPTED");
          digitalWrite(greenLED,HIGH);
          delay(2000);
          digitalWrite(greenLED,LOW); }
          
          else{
          lcd.println("DENIED");
          delay(1000);
          lcd.clear();
          lcd.setCursor(0,0);}
          }
          lcd.clear();
          k=0;
          digitalWrite(redLED,HIGH);
          readpw="";
          change=0;
  }

  void RFID(){
    { if(!rfid.PICC_ReadCardSerial()); //Verify if the new card has been read
      else {
          for(byte i=0; i<4; i++){
          cardID.concat(String(rfid.uid.uidByte[i],HEX));
          }
          Serial.println("New card has been detected ");
          Serial.println("UID:  ");
          cardID.toUpperCase();
          Serial.println(cardID);
          
          rfid.PICC_HaltA();
          rfid.PCD_StopCrypto1();
          
          if(cardID=="C373C415") {  digitalWrite(greenLED,HIGH);  state=0;}
          else if(cardID=="9C75849") {  digitalWrite(greenLED,LOW); state=1;}
          cardID="";
          }
          
      }
}

#include "FS.h"
#include "SD.h"
#include "SPI.h"
//current test
#define SD_SCK 15
#define SD_MOSI 21
#define SD_MISO 22
#define SD_CS 5

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

#include "fabgl.h"

#define ENTER_KEY 13
#define SPACE_KEY 32
#define RIGHT_ARROW '}'
#define LEFT_ARROW '{'
#define UP_ARROW '~'
#define DOWN_ARROW '|'
#define BACKSPACE (char)127

const char* ssid = "espkeyboard";  // Enter SSID here
const char* password = "espkeyboard";  //Enter Password here
WebServer server(80);
bool gotChar = false;
char received_char;

char pos_str[20];

fabgl::VGA16Controller display_controller;
fabgl::Terminal terminal;

String cmd, current_word = "", location = "/", parameter = "";
bool prompted = false;
byte lineCount = 1, lineTaken = 1;
String prev_cmd = "";

//sd card functions
void listDir(fs::FS &filesystem, String filename) 
{
  File root = filesystem.open(filename);
  if (!root) 
  {
    Serial.println("The directory \"" + filename + "\" does not exist.");
    tprintln(terminal, "The directory \"" + filename + "\" does not exist.");
    return;
  }
  else if (!root.isDirectory())
  {
    Serial.println("Cannot list files");
    tprintln(terminal, "Cannot list files");
    return;
  }

  File current = root.openNextFile();
  while (current) 
  {
    if (current.isDirectory()) 
    {
      Serial.println(String(current.name()) + "/");
    tprintln(terminal, String(current.name()) + "/");
    }
    else 
    {
      Serial.println(String(current.name()) + "\t\tsize: " + String(current.size()));
    tprintln(terminal, String(current.name()) + "\t\tsize: " + String(current.size()));
    }

    current = root.openNextFile();
  }
}

void makeDir(fs::FS &filesystem, String dirname) 
{
  if (!filesystem.mkdir(dirname)) 
  {
    Serial.println("Failed to create the directory.");
    tprintln(terminal, "Failed to create the directory.");
  }
}

void remove_(fs::FS &filesystem, String path) 
{
  if (!filesystem.rmdir(path) && !filesystem.remove(path)) 
  {
    Serial.println("Failed to remove the file/directory " + path);
    tprintln(terminal, "Failed to remove the file/directory " + path);
  }
}

void createFile(fs::FS &filesystem, String filename)
{
  File file = filesystem.open(filename, FILE_WRITE);
  if (!file) 
  {
    Serial.println("Could not create the file " + filename);
    tprintln(terminal, "Could not create the file " + filename);
  }
  else    
    file.close();
}

void readFile(fs::FS &filesystem, String filename)
{
  File file = filesystem.open(filename);
  
  if (!file)
  {
    Serial.println("Cannot open " + filename);
    tprintln(terminal, "Cannot open " + filename);
    return;
  }

  char pos_str[20];
  sprintf(pos_str, "\e[%d;1H", lineCount);
  terminal.write(pos_str);
  
  while (file.available()) 
  {
    char e = file.read();
    if (e == '\n')
      lineCount ++;
    Serial.write(e);
    terminal.write(e);
  }
  tprintln(terminal, "");

  file.close();
}

void tprint(fabgl::Terminal &t, String text)
{
  sprintf(pos_str, "\e[%d;%dH", lineCount, lineTaken);
  lineTaken += text.length();
  t.write(pos_str);
  
  for (int it = 0; it < text.length(); it++)
  {
    t.write(text[it]);
  }
}

void tprintln(fabgl::Terminal &t, String text)
{
  sprintf(pos_str, "\e[%d;%dH", lineCount++, lineTaken);
  t.write(pos_str);
  lineTaken = 1;

  for (int it = 0; it < text.length(); it++)
  {
    t.write(text[it]);
  }
}

void handlePost()
{
  String payload = server.arg("plain");

  DynamicJsonDocument jsonDoc(1024);
  deserializeJson(jsonDoc, payload);

  const char* character = jsonDoc["character"];
  
  if (String(character).length() > 1) {
    received_char = (char)(String(character).toInt());
  } else {
    received_char = character[0];
  }
  
  Serial.print(received_char);

  gotChar = true;  
}

SPIClass SD_SPI(HSPI);

void setup() {
  Serial.begin(115200);

  // WiFi.setPins(6, 7, 8);
  WiFi.softAP(ssid, password);
  
  Serial.print("AP IP address ozii : ");
  Serial.println(WiFi.softAPIP());

  server.on("/post", HTTP_POST, handlePost);

  server.begin();
  Serial.println("HTTP server started");

  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if(!SD.begin(SD_CS, SD_SPI))
  {
    Serial.println("Card Mount Failed");
    return;
  } else {
    Serial.println("Card Mount Successful");
  }  

  display_controller.begin(GPIO_NUM_26, /*red 1*/
                           GPIO_NUM_25, /*red 0*/
                           GPIO_NUM_27, /*green 1*/
                           GPIO_NUM_14, /*green 0*/
                           GPIO_NUM_12, /*blue 1*/
                           GPIO_NUM_13, /*blue 0*/
                           GPIO_NUM_33, /*hsync*/
                           GPIO_NUM_32  /*vsync*/);
  display_controller.setResolution(VGA_640x480_60Hz);

  terminal.begin(&display_controller);
  terminal.enableCursor(true);

  terminal.write("\e[40;92m");// background: black, foreground: green
  terminal.write("\e[2J");// clear screen
  terminal.write("\e[1;1H");// move cursor to 1,1
}

void loop() {
  server.handleClient();

  if (!prompted) 
  {
    prompted = true;
    Serial.print(location + "> ");
    tprint(terminal, location + "> ");
  }

  //take input from the user
  if (gotChar)
  {
    Serial.println("got : " + String(received_char));
    gotChar = false;

    if (received_char >= 123 && received_char <= 127) {
      switch (received_char) {
        case RIGHT_ARROW:
          sprintf(pos_str, "\e[%d;%dH", lineCount, (lineTaken < terminal.getColumns()) ? ++lineTaken : lineTaken);
          terminal.write(pos_str);
          Serial.println("/Right/");
          break;
        case LEFT_ARROW: // left key
          if (lineTaken > location.length()+2)
            lineTaken -= 2;

          sprintf(pos_str, "\e[%d;%dH", lineCount, lineTaken);
          terminal.write(pos_str);
          Serial.println("/Left/");
          break;
        case UP_ARROW:
          cmd = prev_cmd;
          break;
        case BACKSPACE: //delete key
          if (lineTaken > location.length()+2)
            lineTaken -= 2;

          sprintf(pos_str, "\e[%d;%dH", lineCount, lineTaken);
          terminal.write(pos_str);
          cmd.remove(cmd.length()-2, 1);
          Serial.println("/erase/");
          break;
      }
    }

    if (received_char == ENTER_KEY) // enter
    {
      cmd += " ";
      tprintln(terminal, "");
      Serial.println("");
      prompted = false;    
      current_word = "";
      parameter = "";
      prev_cmd = cmd;      

      for (int i = 0; i < cmd.length()+1; i++) 
      {
        Serial.print("at the char : \"");
        Serial.print(cmd[i]);
        Serial.println("\"");
        
        if (i == cmd.length()-1 || cmd[i] == SPACE_KEY) // end of command or a space
        {
          Serial.print("entered: \"");
          Serial.print(current_word);
          Serial.println("\"");
          
          if (cmd[i] == SPACE_KEY) 
          {
            parameter = "";
            
            while (i != cmd.length())
              parameter += cmd[i++];

            parameter.trim();
          }

          if (current_word == "echo" || current_word == "say") 
          {
            Serial.println("\"" + parameter + "\"");
            tprintln(terminal, "\"" + parameter + "\"");
          }
          else if (current_word == "cls" || current_word == "clear") 
          {
            Serial.println("clearing the screen...");
            tprintln(terminal, "clearing the screen...");
            terminal.write("\e[2J");// clear screen
            terminal.write("\e[1;1H");// move cursor to 1,1
            lineTaken = 1;
            lineCount = 1;
          }
          
          else if (current_word == "mkdir") 
          {
            if (parameter == "") 
            {
              Serial.println("Please enter the name of the directory you want to create.");
              tprintln(terminal, "Please enter the name of the directory you want to create.");
              break;
            }

            makeDir(SD, location + ((location == "/") ? "" : "/") + parameter);
          } 
          else if (current_word == "remove" || current_word == "rm") 
          {
            remove_(SD, location + ((location != "/") ? "/" : "") + parameter);
          } 
          else if (current_word == "ls" || current_word == "list") 
          {
            if (parameter == "") 
              listDir(SD, location);
            else
              listDir(SD, location + ((location == "/") ? "" : "/") + parameter);
          }
          else if (current_word == "chdir" || current_word == "cd")
          {
            if (parameter == "..")
            {
              if (location == "/")
                break;
                
              int last_bracket = location.lastIndexOf("/");
              location = location.substring(0, last_bracket);

              if (location == "")
                location = "/";
                
              break;
            }

            File test = SD.open(location + "/" + parameter);
            if (!test)
            {
              Serial.println("This location does not exist.");
              tprintln(terminal, "This location does not exist.");
            }
            else
            {
              location += (((location == "/") ? "" : "/") + parameter);
              test.close();
            }
          }
             
          else if (current_word == "touch" || current_word == "create") 
          {
            createFile(SD, location + ((location == "/") ? "" : "/") + parameter);
          }
          else if (current_word == "read") 
          {
            readFile(SD, location + ((location == "/") ? "" : "/") + parameter);
          }

          else if (current_word == "color") 
          {
            char color_str[20];
            sprintf(color_str, "\e[40;%dm", parameter.toInt());
            terminal.write(color_str);
            Serial.println("changed color");
          }  
          else if (current_word == "credits") 
          {
            Serial.println("This stuff was made by us");
            tprintln(terminal, "This stuff was made by us");
          }  
          else if (current_word == "help") 
          {
            Serial.println("To the rescue!");
            tprintln(terminal, "To the rescue!");
          }
          else if (current_word == "thanks" || current_word == "thank") 
          {
            Serial.println("You\'re most welcome!");
            tprintln(terminal, "You\'re most welcome!");
          }
          else 
          {
            Serial.println("Please enter a valid command!");
            tprintln(terminal, "Please enter a valid command!");
          }

          break;
        }
        else if (received_char < 123)
        {
          current_word += String(cmd[i]);
        }
      }

      cmd = "";
    }
    else 
    {
      String newChar = (received_char == '-') ? " " : String(received_char);
      cmd += newChar;
      //cmd = Serial.readString();
      Serial.print(newChar);
      if (received_char < 123 || received_char > 127) 
        tprint(terminal, newChar);
    }
  }
}
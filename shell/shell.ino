#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "fabgl.h"

#define SD_SCK 18
#define SD_MOSI 19
#define SD_MISO 23
#define SD_CS 5

SPIClass SD_SPI(HSPI);

fabgl::VGA16Controller display_controller;
fabgl::Terminal terminal;

String cmd, current_word = "", location = "/", parameter = "";
bool prompted = false;
byte lineCount = 1, lineTaken = 1;

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
  char pos_str[20];
  
  if (lineCount >= terminal.getRows())
  {
    t.write("\e[2J");// clear screen
    lineTaken = 1; lineCount = 1;
  }
  
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
  char pos_str[20];

  if (lineCount >= terminal.getRows())
  {
    t.write("\e[2J");// clear screen
    lineTaken = 1; lineCount = 1;
  }

  sprintf(pos_str, "\e[%d;%dH", lineCount++, lineTaken);
  t.write(pos_str);
  lineTaken = 1;

  for (int it = 0; it < text.length(); it++)
  {
    t.write(text[it]);
  }
}

void setup() 
{
  display_controller.begin(GPIO_NUM_26,
                          GPIO_NUM_25,
                          GPIO_NUM_27,
                          GPIO_NUM_14, 
                          GPIO_NUM_12, 
                          GPIO_NUM_13,
                          GPIO_NUM_33,
                          GPIO_NUM_32);
  display_controller.setResolution(VGA_640x480_60Hz);

  terminal.begin(&display_controller);
  terminal.enableCursor(true);

  terminal.write("\e[40;92m");// background: black, foreground: green
  terminal.write("\e[2J");// clear screen
  terminal.write("\e[1;1H");// move cursor to 1,1


  Serial.begin(115200);

  
  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if(!SD.begin(SD_CS, SD_SPI))
  {
      Serial.println("Card Mount Failed");
      tprintln(terminal, "Card Mount Failed");
      return;
  }
}

void loop() 
{
  if (!prompted) 
  {
    prompted = true;
    Serial.print(location + "> ");
    tprint(terminal, location + "> ");
  }

  //take input from the user
  if (Serial.available()) 
  {
      cmd = Serial.readString();
      Serial.println("\"" + cmd + "\"");
      tprintln(terminal, cmd);

      prompted = false;      
      current_word = "";
      parameter = "";

      for (int i = 0; i < cmd.length(); i++) 
      {
        if (String(cmd[i]) == "\n" || String(cmd[i]) == "\0" || String(cmd[i]) == " ") 
        {
          if (String(cmd[i]) == " ") 
          {
            parameter = "";
            
            while (String(cmd[i]) != "\0")
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
          }  
          else if (current_word == "credits") 
          {
            Serial.println("This stuff was made by us");
            tprintln(terminal, "This stuff was made by:");
            tprintln(terminal, "- Yusuf El-Nady");
            tprintln(terminal, "- Mazen El-Deeb");
            tprintln(terminal, "- Mostafa Khashan");
            tprintln(terminal, "- Retaj Tarek");
            tprintln(terminal, "- Rowan Ashraf");
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
        else 
        {
          current_word += String(cmd[i]);
        }
      }
  }
}

# ESP32-Computer
We've all been confused at the beginning about the difference between microcontrollers and microcomputers, and it took us a while to finally differentiate between them. But have you ever thought that both could be one?

This project is a fully functional barebone computer, made using the ESP32 microcontroller as its processing unit, a VGA display as its main display, a wifi keyboard as its main input method, and an SD card as a storage device.

## System
We developed a basic CLI as a frontend for the user and a backend to bind all the peripherals together seamlessly. The CLI enables the user to:
- Access the filesystem on the SD card and navigate through it via linux-like commands such as "cd", "ls", and others.
- Create and delete directories and files.
- Read text files. (Writing through a vim-like text editor will be available soon)
- Change the termainal's color scheme.
- Several other commands we added for fun such as "help", "credits", and "thanks".

## Display
We used a VGA display to display our output through a 64-color pin configuration. The standard resolution is 540 * 480 and operates at 60 fps. The serial monitor could also be used as an output method at 115200 baud.

## Input
Here's where the probelsm start. Initially we wanted to interface with a PS2 keyboard through the data and clock pins, and implement the communication protocol ourselves since it's simple. However, the only PS2 keyboard we had at hand had stopped working all of a sudden.
We then decided to use the same protocol but with a PS2-compatible USB keyboard, where the D+ and D- pins would be connected instead of the PS2's clock and data pins, but unfortunately the 5 USB keyboards we tried weren't PS2-compatible.
As a final resort, we decided to enter all sorts of input to the ESP through a webpage hosted on it, where POST and GET requests would be made through the ESP and the webpage dipslayed on any device. This was fortunately successful and we were able to display a web-based keyboard.
To spice it up even more, we went ahead and developed a Flutter app for android phones, where the phine would be connected to the ESP's as an access point.

## Storage
Here's another troublemaker that kept us on hold for a while. At first, we fabricated a PCB and soldered an SMD holder on it, and connected its pins to the ESP's SPI pins. On running the example code, the SD card gave no reply even though we checked our connections and they were all right. We bought a ready-made SD card module which worked perectly fine, and after a bit of troubleshooting we found out about the ESP's pin matrix architecture, and configured the HSPI to our SD card holder's pins, and it worked fine.

## Integration
You guessed right; the system went bonkers here as well. The system -at first- never worked as a whole, it was either the display and SD card or the display and the keyboard, but not the 3 together. 
At first we suspected it was a matter of resources, and we tried to use FreeRTOS to divide the tasks between the 2 cores of the ESP, but even that didn't help. 
After lots and lots of troubleshooting and research, we discovered that the HSPI pins we configured and the WiFi pins overlapped, and that the WiFi pins couldn't be configured like the other peripherals' pins, and unfortunately the SD card holder's pins were hard-wired, so we had to connect the ESP to the PCB via jumpers to connect the SD card holder to different pins, and
IT WORKED!!!

## Future Plans
- Develop a vim-like text editor to edit the text files on the SD card
- Implement an interpreter so that the user could write programs that could be executed on the computer
- Add more built-in programs such as a calculator, a calender, and maybe even some games
- Add the option of connecting to other wireless devices such as bluetooth keyboards and printers
- Design a 3d enclosure for the PCB
- Add more methods of input (maybe bit-bang USB?)
- Implement a graphics engine for built-in or user made programs

// Smart Remote Control Sketch
// Copyright 2014 Tony DiCola
// Released under an MIT license: http://opensource.org/licenses/MIT

// Based on examples from the IR remote library at: https://github.com/shirriff/Arduino-IRremote
// Depends on a fork of the library to fix a file name conflict in recent Arduino IDE releases.
// Download forked library from: https://github.com/tdicola/Arduino_IRremote

#include <ctype.h>
#include <Console.h>
#include "IRremote_library.h"
#include "IRremoteInt_library.h"

// Pin connected to the IR receiver.
#define RECV_PIN 11

// Length of time to delay between codes.
#define REPEAT_DELAY_MS 40

// Size of parsing buffer.
#define BUFFER_SIZE 100

// Parsing state.
char buffer[BUFFER_SIZE+1] = {0};
int index = 0;
decode_results command;
unsigned int rawbuf[RAWBUF] = {0};

// Remote code types.
#define TYPE_COUNT 9
char* type_names[TYPE_COUNT] = { "NEC", "SONY", "RC5", "RC6", "DISH", "SHARP", "PANASONIC", "JVC", "RAW" };
int type_values[TYPE_COUNT] = { 1, 2, 3, 4, 5, 6, 7, 8, -1 };

// IR send and receive class.
IRsend irsend;
IRrecv irrecv(RECV_PIN);

// Setup function called once at bootup.
void setup() {
  // Initialize bridge, console, and IR receiver.
  Bridge.begin();
  Console.begin();
  irrecv.enableIRIn();
}

// Loop function called continuously.
void loop() {
  // Check if any data is available and parse it.
  if (Console.available() > 0) {
    // Read a character at a time.
    char c = Console.read();
    if (c == ':') {
      // Parse remote code type as current buffer value when colon is found.
      char* type = current_word();
      for (int i = 0; i < TYPE_COUNT; ++i) {
        if (strcmp(type, type_names[i]) == 0) {
          command.decode_type = type_values[i];
        }
      }
    }
    else if (c == ' ' && command.decode_type == PANASONIC && command.panasonicAddress == 0 && index > 0) {
      // Parse panasonic address as first value before whitespace.
      command.panasonicAddress = (unsigned int)strtoul(current_word(), NULL, 16);
    }
    else if (c == ' ' && command.decode_type != UNKNOWN && command.decode_type != PANASONIC && command.bits == 0 && index > 0) {
      // Parse number of bits as first value before whitespace.
      command.bits = (int)strtol(current_word(), NULL, 16);
    }
    else if (c == ' ' && command.decode_type == UNKNOWN && index > 0) {
      // Parse mark/space length as value for unknown/raw code.
      if (command.rawlen < RAWBUF) {
        rawbuf[command.rawlen] = (unsigned int)strtoul(current_word(), NULL, 16);
        command.rawlen++;
      }
    }
    else if (c == '\n') {
      // Finish parsing command when end of line received.
      // Parse remaining data in buffer.
      if (index > 0) {
        // Grab buffer data as IR code value.
        command.value = strtoul(current_word(), NULL, 16);
        // Move buffer data to raw code array if parsing unknown/raw code.
        if (command.decode_type == UNKNOWN && command.rawlen < RAWBUF) {
          rawbuf[command.rawlen] = (unsigned int)command.value;
          command.value = 0;
          command.rawlen++;
        }
      }
      // Print code to be sent.
      Console.print("Sending remote code:");
      int type_index = command.decode_type > 0 ? command.decode_type - 1 : TYPE_COUNT - 1;
      Console.print("Type: "); Console.println(type_names[type_index]);
      Console.print("Address: "); Console.println(command.panasonicAddress, HEX);
      Console.print("Value: "); Console.println(command.value, HEX);
      if (command.rawlen > 0) {
        Console.println("Raw value: ");
        for (int i = 0; i < command.rawlen; ++i) {
          Console.print(rawbuf[i], HEX);
          Console.print(" ");
        }
        Console.println("");
      }
      Console.println("--------------------");
      // Send code.
      send_command();
      // Enable IR receiver again.
      irrecv.enableIRIn();
      // Clear the command to prepare for parsing again.
      command = decode_results();
    }
    else if (c != ' ' && c != '\n' && c != '\t') {
      // Add non-whitespace characters to the buffer.
      buffer[index] = toupper(c);
      index = index == (BUFFER_SIZE-1) ? 0 : index+1;
    }
  }
  // Check if an IR code has been received and print it.
  decode_results results;
  if (irrecv.decode(&results)) {
    Console.println("Decoded remote code:");
    print_code(&results);
    Console.println("--------------------");
    delay(20);
    irrecv.resume();
  }
  // Wait a small amount so the Bridge library is not overwhelmed with requests to read the console.
  delay(10);
}

// Send the parsed remote control command.
void send_command() {
  // Use the right send function depending on the command type.
  if (command.decode_type == NEC) {
    irsend.sendNEC(command.value, command.bits);
  }
  else if (command.decode_type == SONY) {
    // Sony codes are sent 3 times as a part of their protocol.
    for (int i = 0; i < 3; ++i) {
      irsend.sendSony(command.value, command.bits);
      delay(REPEAT_DELAY_MS);
    }
  }
  else if (command.decode_type == RC5) {
    // RC5 codes are sent 3 times as a part of their protocol.
    for (int i = 0; i < 3; ++i) {
      irsend.sendRC5(command.value, command.bits);
      delay(REPEAT_DELAY_MS);
    }
  }
  else if (command.decode_type == RC6) {
    // RC6 codes are sent 3 times as a part of their protocol.
    for (int i = 0; i < 3; ++i) {
      irsend.sendRC6(command.value, command.bits);
      delay(REPEAT_DELAY_MS);
    }
  }
  else if (command.decode_type == DISH) {
    irsend.sendDISH(command.value, command.bits);
  }
  else if (command.decode_type == SHARP) {
    irsend.sendSharp(command.value, command.bits);
  }
  else if (command.decode_type == PANASONIC) {
    irsend.sendPanasonic(command.panasonicAddress, command.value);
    delay(REPEAT_DELAY_MS);
  }
  else if (command.decode_type == JVC) {
    irsend.sendJVC(command.value, command.bits, 0);
  }
  else if (command.decode_type == UNKNOWN) {
    irsend.sendRaw(rawbuf, command.rawlen, 38);
  }
}

// Print received code to the Console.
void print_code(decode_results *results) {
  if (results->decode_type == NEC) {
    Console.print("NEC: ");
    Console.print(results->bits, HEX);
    Console.print(" ");
    Console.println(results->value, HEX);
  } 
  else if (results->decode_type == SONY) {
    Console.print("SONY: ");
    Console.print(results->bits, HEX);
    Console.print(" ");
    Console.println(results->value, HEX);
  } 
  else if (results->decode_type == RC5) {
    Console.print("RC5: ");
    Console.print(results->bits, HEX);
    Console.print(" ");
    Console.println(results->value, HEX);
  } 
  else if (results->decode_type == RC6) {
    Console.print("RC6: ");
    Console.print(results->bits, HEX);
    Console.print(" ");
    Console.println(results->value, HEX);
  }
  else if (results->decode_type == PANASONIC) {	
    Console.print("PANASONIC: ");
    Console.print(results->panasonicAddress,HEX);
    Console.print(" ");
    Console.println(results->value, HEX);
  }
  else if (results->decode_type == JVC) {
     Console.print("JVC: ");
    Console.print(results->bits, HEX);
    Console.print(" ");
     Console.println(results->value, HEX);
  }
  else {
    Console.print("RAW: ");
    for (int i = 1; i < results->rawlen; i++) {
      // Scale length to microseconds.
      unsigned int value = results->rawbuf[i]*USECPERTICK;
      // Adjust length based on error in IR receiver measurement time.
      if (i % 2) {
        value -= MARK_EXCESS;
      }
      else {
        value += MARK_EXCESS;
      }
      // Print mark/space length.
      Console.print(value, HEX);
      Console.print(" ");
    }
    Console.println("");
  }
}

// Grab the currently parsed word and clear the buffer.
char* current_word() {
  buffer[index] = 0;
  index = 0;
  return buffer;
}



#include <Arduino.h>
#include <IRremote.hpp>

#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

#define IR_RECEIVE_PIN 3
#define ENABLE_LED_FEEDBACK 1
#define IR_SEND_PIN 5
bool DecodeSignals = true;
bool DebugMode = false;
bool ReplyMode = true;

void setup() {
  Serial.begin(9600);
  delay(100);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrSender.begin(IR_SEND_PIN); // Start with IR_SEND_PIN as send pin and enable feedback LED at default feedback LED pin
  Serial.println("Connected.");
  if (DebugMode == true) {
    Serial.print(F("Ready to receive IR signals of protocols: "));
    printActiveIRProtocols(&Serial);  
    Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));
    Serial.println(F("Send IR signals at pin " STR(IR_SEND_PIN)));
  }
}

void receive_ir_data(bool print) {
  if (IrReceiver.decode()) {
    if(print == true) {
      Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX); // Print "old" raw data
      // USE NEW 3.x FUNCTIONS
      IrReceiver.printIRResultShort(&Serial); // Print complete received data in one line
      IrReceiver.printIRSendUsage(&Serial);   // Print the statement required to send this data
    }
    IrReceiver.resume(); // Enable receiving of the next value
  }
}

String message = "";
void ParseSerialData()
{
  while(Serial.available() > 0)
  {
    message += Serial.readString();
    Debug("message: " + message);
  }
  message.trim();
  while(message.length() > 0) 
  {
    int indexOfCommandEnd = message.indexOf(';');
    if (indexOfCommandEnd == -1) 
    {
      // either the command hasnt fully come in yet, or is malformed
      break;
    }
    String command = message.substring(0, indexOfCommandEnd + 1);
    command.trim();
    message.remove(0, indexOfCommandEnd + 1);
    if (command.startsWith("Settings:")) 
    {
      command.remove(0, 9);
      ExecuteSettingsOp(command);
    } 
    else if (command.startsWith("IRCommand:"))
    {
      command.remove(0, 10);
      ExecuteIRCommandOp(command);
    } else {
      Reply("Unknown command: " + command);
      continue;
    }
  }
}
void ExecuteSettingsOp(String message) {
  String arr[3] = { "", "", "" }; 
  ParseCommand(message, arr, 3);
  DecodeSignals = arr[0] == "true";
  DebugMode = arr[1] == "true";
  ReplyMode = arr[2] == "true";
  Debug("Settings Changed");
}

void ExecuteIRCommandOp(String message) {
  // delay, protocol, address, command
  String arr[4] = {"", "", "", "" };
  ParseCommand(message, arr, 4);
  Reply("Executing Protocol: '" + arr[1] + "' Address: '" + 
      arr[2] + "' Command: '" + arr[3] + "' Delay: " + arr[0]);

  if (arr[1] == "NEC") {
    IrSender.sendNEC((uint16_t)arr[2].toInt(), (uint8_t)arr[3].toInt(), 0);
    IrReceiver.restartAfterSend(); // Is a NOP if sending does not require a timer.
    Debug("Executed");
    Reply("Executed Protocol: '" + arr[1] + "' Address: '" + 
      arr[2] + "' Command: '" + arr[3] + "' Delay: " + arr[0]);
  } else {
    Reply("Protocol Not implemented yet");
  }
  delay(arr[0].toInt());
}

void ParseCommand(String message, String arr[], int length) {
  int state = 0;
  for (int i = 0; i < message.length(); ++i) {
    char curr = message[i];
    Debug("State: " + String(state, DEC));
    Debug("curr: " + String((int)curr) + " As String: " + String(curr));
    if (curr == ';') {
      return;
    } else if (curr == ',') {
      state += 1;
    } else {
      arr[state] += curr;
    }
  }
  return;
}

void loop() {
  ParseSerialData();
  receive_ir_data(DecodeSignals);
}

void Debug(String log) {
  if (DebugMode == false) {
    return;
  }
  Serial.println(log);
}

void Reply(String log) {
  if (ReplyMode == false) {
    return;
  }
  Serial.println(log);
}

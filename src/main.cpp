#include <Arduino.h>
#include "Communications.h"
#include <map>

#define TX_GPIO_NUM 16
#define RX_GPIO_NUM 17
#define MAIN_LED_GPIO 2
#define ACTIVITY_PIN_GPIO   12
#define FIRMWARE_VERSION  "1.0.0"
#define BLUETOOTH_VISIBLE_NAME "Custom Bluetooth Device"

// pointer to a void function
typedef void (*void_ptr)(void);

void LED_ON(){
  digitalWrite(MAIN_LED_GPIO, HIGH);       
  digitalWrite(ACTIVITY_PIN_GPIO, HIGH); 
  SerialBT.print("LED On");
};
void LED_OFF(){
  digitalWrite(MAIN_LED_GPIO, LOW); 
  digitalWrite(ACTIVITY_PIN_GPIO, LOW); 
  SerialBT.print("LED Off");
};
void GET_DEVICE_NAME(){
  SerialBT.print(BLUETOOTH_VISIBLE_NAME);
};
void GET_DEVICE_VERSION(){
  SerialBT.print(FIRMWARE_VERSION);
};
void GET_LED_STATUS(){
  SerialBT.printf("LED Status: %s", digitalRead(MAIN_LED_GPIO)==0?"Off":"On");
};
void RESET_DEVICE(){
  esp_restart();
};

// Functions to call
std::map<String, void_ptr> FunctionMap{
    {"AT01",LED_ON},
    {"AT02",LED_OFF},
    {"ATN",GET_DEVICE_NAME},
    {"ATS",GET_LED_STATUS},
    {"ATV",GET_DEVICE_VERSION},
    {"ATZ",RESET_DEVICE}
  };

String inputBuffer;

// not deleted anywhere (delete[] responseBuffer;))
char * responseBuffer = new char[32];

// Keeps a reference to the led task to pause or delete, etc
TaskHandle_t ledTask = NULL; 
// used to end the led task gracefully
int ledTaskCanEnd = false; 

// Task Function - to blink the LED (ensure the correct board gpio is set)
void LEDTask(void *params) {

  // get a pointer to the changing variable
  int *pEndTask = (int*)params;

  while(*pEndTask == 0) {
    digitalWrite(MAIN_LED_GPIO, HIGH); 
    vTaskDelay(50/portTICK_PERIOD_MS);
    digitalWrite(MAIN_LED_GPIO, LOW); 
    vTaskDelay(50/portTICK_PERIOD_MS);
  }

  // ensure LED is off at end of task
  digitalWrite(MAIN_LED_GPIO, LOW); 
  // Never exit a task without deleting it first
  vTaskDelete(NULL);

}


// Handle Bluetooth events
void BT_EventHandler(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_START_EVT) {
    // a task runs in parallel
    //xTaskCreate(LEDCycler, "Startup Indicator",1000, NULL, 1, NULL);
    Serial.println("Bluetooth Initialized...");
  }
  else if (event == ESP_SPP_SRV_OPEN_EVT ) {
    Serial.println("BT Client connected");
  }
  else if (event == ESP_SPP_CLOSE_EVT  ) {
    Serial.println("BT Client disconnected");
  }
  else if (event == ESP_SPP_DATA_IND_EVT ) {
    // INCOMING DATA HANDLED HERE...
    bool eof = false;

    // Save any input bytes to buffer
    while (SerialBT.available()) {

      // Read a character from the input (may be many characters to read)
      char incoming = SerialBT.read();

      // is the current character NOT a CR ('\r')? 
      if(!(eof = incoming == '\r'))
      {
        // a return character ('\r') defines the end of a packet
        // if no return is received, just continue to get data
        inputBuffer += incoming;
        continue;
      }

    }

    // packet is not full (no CR was rcvd), so just return
    if(!eof) return;

    // Process a full packet, CR was found
    inputBuffer.toUpperCase();

    // Take a look at what's in the buffer
    Serial.println(inputBuffer);

    // try to find a function to run based on input rcvd
    std::map<String, void_ptr>::iterator functionMapPointer = FunctionMap.find(inputBuffer);
    // if the pointer is not at its end, then we DID find an entry in the FunctionMap
    if(functionMapPointer != FunctionMap.end())
    {
      // 'second' refers to the object we created in 'FunctionMap' which is a 
      // pointer to a function - so, just call it by applying brackets "()"
      functionMapPointer->second();

      // Clear the input to be ready for another command
      inputBuffer.clear();

    }
  }
}
  
// setup() runs automatically, then loop() is called
void setup() {

  // Start a blinking LED to indicate setup is running, start by setting up gpio for board LED
  digitalWrite(MAIN_LED_GPIO, LOW); 
  pinMode(MAIN_LED_GPIO, OUTPUT);
  xTaskCreate(LEDTask, "Startup Indicator",1000, &ledTaskCanEnd, 1, &ledTask); // a task runs in parallel

  // Setup the serial port
  Serial.begin(9600);
  delay(800/portTICK_PERIOD_MS); // pause for serial port init
  Serial.println("Reset");

  InitBluetooth(BLUETOOTH_VISIBLE_NAME, BT_EventHandler);

  // Stop the blinking LED, also ends the task from inside
  ledTaskCanEnd = true;
  // also ends the led task, but can't be sure if led is off 
  //vTaskDelete(ledTask);
   
}

// Runs infinitely after setup
void loop() {

  // Remove the loop altogether...
  //vTaskDelete(NULL);

  vTaskDelay(500/portTICK_PERIOD_MS);

}

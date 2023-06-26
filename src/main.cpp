#include <Arduino.h>
#include "Communications.h"

#define TX_GPIO_NUM 16
#define RX_GPIO_NUM 17
#define MAIN_LED_GPIO 2
#define ACTIVITY_PIN_GPIO   12
#define FIRMWARE_VERSION  "1.0.0"
#define BLUETOOTH_VISIBLE_NAME "OS Jamme II"

//TaskHandle_t taskHandle_1 = NULL;

String inputBuffer;

// not deleted anywhere (delete[] responseBuffer;))
char * responseBuffer = new char[32];

// Task to blink the LED (ensure the correct board gpio is set)
void LEDCycler(void *params) {

  for(int i = 0;i<5;i++){
    digitalWrite(MAIN_LED_GPIO, HIGH); 
    vTaskDelay(50/portTICK_PERIOD_MS);
    digitalWrite(MAIN_LED_GPIO, LOW); 
    vTaskDelay(50/portTICK_PERIOD_MS);
  }

  // Never exit a task without deleting it first
  vTaskDelete(NULL);
}


// Handle things happening with Bluetooth - the 'Event Handler' functionIt'
void BT_EventHandler(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_START_EVT) {
    // a task runs in parallel
    xTaskCreate(LEDCycler, "Startup Indicator",1000, NULL, 1, NULL);
    Serial.println("Bluetooth Initialized...");
  }
  else if (event == ESP_SPP_SRV_OPEN_EVT ) {
    Serial.println("BT Client connected");
  }
  else if (event == ESP_SPP_CLOSE_EVT  ) {
    Serial.println("BT Client disconnected");
  }
  else if (event == ESP_SPP_DATA_IND_EVT ) {
    //Serial.println("Data received");

    inputBuffer.clear();
    while (SerialBT.available()) {
      char incoming = SerialBT.read();
      inputBuffer += incoming;
    }

    inputBuffer.toUpperCase();

    Serial.println(inputBuffer);
    
    if(inputBuffer.compareTo("AT01") == 0)
    {
      digitalWrite(MAIN_LED_GPIO, HIGH);       
      digitalWrite(ACTIVITY_PIN_GPIO, HIGH); 
      SerialBT.print("LED On");
    }

    else if(inputBuffer.compareTo("AT02") == 0)
    {
      digitalWrite(MAIN_LED_GPIO, LOW); 
      digitalWrite(ACTIVITY_PIN_GPIO, LOW); 
 
      SerialBT.print("LED Off");
    }

    else if(inputBuffer.compareTo("ATN") == 0)
    {
      
       SerialBT.print(BLUETOOTH_VISIBLE_NAME);
    }
    else if(inputBuffer.compareTo("ATV") == 0)
    {
      
       SerialBT.print(FIRMWARE_VERSION);
    }
    else if(inputBuffer.compareTo("ATS") == 0)
    {
      
       SerialBT.print("LED Status: ");
       SerialBT.print(digitalRead(MAIN_LED_GPIO)==0?"Off":"On");
    }
    else if(inputBuffer.compareTo("ATZ") == 0)
    {
      esp_restart();
    }
  
  }
}


void setup() {

  // Setup the serial port
  Serial.begin(9600);

  // give the serial port a moment
  delay(800/portTICK_PERIOD_MS);
  Serial.println("Reset");

  // Set gpio pins to low
  digitalWrite(MAIN_LED_GPIO, LOW); 
  digitalWrite(ACTIVITY_PIN_GPIO, LOW); 

  // assign gpio pins as output (not input)
  pinMode(MAIN_LED_GPIO, OUTPUT);
  pinMode(ACTIVITY_PIN_GPIO, OUTPUT);

  InitBluetooth(BLUETOOTH_VISIBLE_NAME, BT_EventHandler);
  
}


void loop() {

  // Remove the loop altogether...
  //vTaskDelete(NULL);

  vTaskDelay(500/portTICK_PERIOD_MS);

}

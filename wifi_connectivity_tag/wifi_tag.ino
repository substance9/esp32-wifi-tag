#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

#define DEBUG
#define ADAFRUIT_FEATHER_ESP32_V2

#define US_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define MS_TO_S_FACTOR 1000  /* Conversion factor for milli seconds to seconds */
#define T_DEEPSLEEP_S 60
#define T_CONN_WAIT_S 5
#define CONN_RETRY_TIMES_THRESHOLD 2
#define T_S_CONN_BACKOFF_BASE 30
#define MAX_BACKOFF_TIMES 4  /* e.g., if base is 30s and max backoff times is 7, then the max wait time is 64 minutes */

#define Touch_Threshold 60 /* Greater the value, more the sensitivity */

#define VBATPIN A13
#if defined(ADAFRUIT_FEATHER_ESP32_V2)
#define PIN_NEOPIXEL 0
#define NEOPIXEL_I2C_POWER 2
#endif


#if defined(PIN_NEOPIXEL)
  Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif

const char* ssid     = "UCInet Mobile Access";
const char* password = "your-password";

const uint64_t max_sleep_time = 3600 * 24 * 7 * 1000000ull;

RTC_DATA_ATTR int backoff_counter = 0;

char debug_output_buffer[50];

touch_pad_t touchPin;

void LEDon(int color_code) {
#if defined(PIN_NEOPIXEL)
  pixel.begin(); // INITIALIZE NeoPixel
  pixel.setBrightness(20); // not so bright
  pixel.setPixelColor(0, color_code);
  pixel.show();
#endif
}

void LEDoff() {
#if defined(PIN_NEOPIXEL)
  pixel.setPixelColor(0, 0x0);
  pixel.show();
#endif
}

void IRAM_ATTR buttonCallback() {
  esp_sleep_enable_timer_wakeup(max_sleep_time);
  esp_deep_sleep_start();
}


void setup()
{
  pinMode(38, INPUT_PULLUP);
  attachInterrupt(38, buttonCallback, FALLING);
  
  pinMode(LED_BUILTIN, OUTPUT);

#ifdef DEBUG
  Serial.begin(115200);
#endif

  esp_sleep_enable_timer_wakeup(T_DEEPSLEEP_S * US_TO_S_FACTOR);
//  esp_sleep_enable_touchpad_wakeup();

//Setup interrupt on Touch Pad 3 (GPIO15)
//  touchAttachInterrupt(T3, touchCallback, Touch_Threshold);

  delay(100);

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);

}

// Interrupt callback function for the touch pin

//void touchCallback(){
//
// }




void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
#ifdef DEBUG
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif

  WiFi.begin(ssid);
  delay(T_CONN_WAIT_S * MS_TO_S_FACTOR);

  int times_tried_connecting = 1;

  while (WiFi.status() != WL_CONNECTED) {
#ifdef DEBUG
    Serial.print(".");
#endif
    if(times_tried_connecting >= CONN_RETRY_TIMES_THRESHOLD){
      //exponential backoff 
      uint64_t backoff_deep_sleep_time = T_S_CONN_BACKOFF_BASE * pow(2,backoff_counter) * US_TO_S_FACTOR * 1ull;
#ifdef DEBUG
      Serial.println("Max connection retry reached, go to deep sleep for exponential backoff time");
      sprintf(debug_output_buffer, "backoff counter: %d, deep sleep time: %d", backoff_counter, T_S_CONN_BACKOFF_BASE * int(pow(2,backoff_counter)));
      Serial.println(debug_output_buffer);
#endif
      if(backoff_counter < MAX_BACKOFF_TIMES){
        backoff_counter = backoff_counter + 1;
      }
      esp_sleep_enable_timer_wakeup(backoff_deep_sleep_time);
      esp_deep_sleep_start();
    }
    
    WiFi.begin(ssid);
    times_tried_connecting = times_tried_connecting + 1;
    delay(T_CONN_WAIT_S * MS_TO_S_FACTOR);

    
  }


//  Successfully connected at this point
  backoff_counter = 0;

  digitalWrite(LED_BUILTIN, LOW);
#ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup ESP32 to sleep for " + String(T_DEEPSLEEP_S) +
                 " Seconds");
  Serial.flush();
#endif

  WiFi.disconnect();  
  esp_sleep_enable_timer_wakeup(T_DEEPSLEEP_S * US_TO_S_FACTOR);
  esp_deep_sleep_start();
}

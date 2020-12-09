/*
分辨率默认配置：config.frame_size = FRAMESIZE_UXGA;
其他配置：
FRAMESIZE_UXGA （1600 x 1200）
FRAMESIZE_QVGA （320 x 240）
FRAMESIZE_CIF （352 x 288）
FRAMESIZE_VGA （640 x 480）
FRAMESIZE_SVGA （800 x 600）
FRAMESIZE_XGA （1024 x 768）
FRAMESIZE_SXGA （1280 x 1024）
*/

#include "esp_http_client.h"
#include "esp_camera.h"
#include <WiFi.h>

/*********************需要修改的地方**********************/
const char* ssid = "Z";           
const char* password = "zhouzhouzhou";          
const char*  post_url = "http://images.bemfa.com/upload/v1/upimages.php"; // 默认上传地址
const char*  uid = "d03b14054c51c465441979c4b5a5d963";   
const char*  topic = "mycam";    
/********************************************************/


bool internet_connected = false;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define KEY_PIN   15
#define FLASH   4

int buttontakephoto = 0;
int btnHold = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(KEY_PIN, INPUT);
  pinMode(FLASH,OUTPUT);
  digitalWrite(KEY_PIN,HIGH);
  if (init_wifi()) { // Connected to WiFi
    internet_connected = true;
    Serial.println("Internet connected");
  }

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}


/********初始化WIFI*********/
bool init_wifi()
{
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
    if (connAttempts > 10) return false;
    connAttempts++;
  }
  return true;
}

/********http请求处理函数*********/
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      Serial.println("HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      Serial.println("HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      Serial.println("HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      Serial.println();
      Serial.printf("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Write out data
        // printf("%.*s", evt->data_len, (char*)evt->data);
      }
      break;
    case HTTP_EVENT_ON_FINISH:
      Serial.println("");
      Serial.println("HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      Serial.println("HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}


/********推送图片*********/
static esp_err_t take_send_photo()
{
  Serial.println("Taking picture...");
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return ESP_FAIL;
  }

  esp_http_client_handle_t http_client;
  
  esp_http_client_config_t config_client = {0};
  config_client.url = post_url;
  config_client.event_handler = _http_event_handler;
  config_client.method = HTTP_METHOD_POST;

  http_client = esp_http_client_init(&config_client);

  esp_http_client_set_post_field(http_client, (const char *)fb->buf, fb->len);//设置http发送的内容和长度

  esp_http_client_set_header(http_client, "Content-Type", "image/jpg"); //设置http头部字段
  esp_http_client_set_header(http_client, "Authorization", uid);  //设置http头部字段
  esp_http_client_set_header(http_client, "Authtopic", topic);   //设置http头部字段
  
  esp_err_t err = esp_http_client_perform(http_client);//发送http请求
  if (err == ESP_OK) {
    Serial.print("esp_http_client_get_status_code: ");
    Serial.println(esp_http_client_get_status_code(http_client));
  }
  esp_http_client_cleanup(http_client);
  esp_camera_fb_return(fb);
}



void loop()
{
    
    buttontakephoto = digitalRead(KEY_PIN); //读取按键返回状态值
    if (buttontakephoto == LOW && btnHold == 0)
    { 
        delay(100);
        if (buttontakephoto == LOW) 
        {
            digitalWrite(FLASH,HIGH);
            take_send_photo();
            btnHold = 1;
            Serial.println("*************************");
            Serial.println("***Take Photo Success!***");
            Serial.println("*************************");
            digitalWrite(FLASH,LOW);
            digitalWrite(KEY_PIN,HIGH);
        }
    }
    if (buttontakephoto == HIGH)
    {
        btnHold = 0;
    }
}

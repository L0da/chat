#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>

#define LIGHT_PIN 4

const int PWMFreq = 1000;
const int PWMLightChannel = 3;
const int PWMResolution = 8;

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

const char* ssid     = "Wifi_Car";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");
uint32_t cameraClientId = 0;

String car = "stop";
int servo1 = 90, servo2, servo3 = 90, servo4 = 180, servo5 = 180, servo6 = 90, servo7, servo8, SPEED, v_angle = 90, h_angle = 90;

unsigned long ts, tm, td;

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
    
    td.btv {
      background:transparent;
      border: 3px solid #ff0000;
      height: 50px;
      width: 60px;
      border-radius: 50%;
      font-size:30px;
      color: #FFF;
    }

    td.bth {
      background:transparent;
      border: 3px solid #ff0000;
      height: 50px;
      width: 60px;
      border-radius: 50%;
      font-size:30px;
      color: #FFF;
    }
    td.bth:active,
    td.btv:active
    {
      transform: scale(1.3);
    }

    td.btn {
      background:transparent;
      border: 3px solid #ff0000;
      width:65px;
      height: 30px;
      border-radius:15px;
    }
    td.btn:active {
      transform: scale(1.3);
    }

    .blue
    {
      color:#001ef1;
    }

    td span,
    td.b
    {
      color:#FFF;
    }

    td.g
    {
      color:#ffa80a;
    }

    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }

    .slidecontainer,
    .st{
      width: 140px;
    }

    .slider {
      -webkit-appearance: none;
      width: 140px;
      height: 8px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }
  
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 25px;
      height: 25px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }
    </style>
  </head>
  <body class="noselect" align="center" style="padding: 0;margin: 0;box-sizing: border-box;
            background: #000;">
    <table id="mainTable" style="margin:auto;table-layout:fixed">
      <tr>
        <td colspan="5">
          <table id="mainTable" style="margin:auto;table-layout:fixed">
            <tr>
              <td colspan="5" style="height:10px"></td>
            </tr>
            <tr>
              <td colspan="2"></td>
              <td class="btv" ontouchstart='sendButtonInput("front")' ontouchend='sendButtonInput
                ("stop")'><span>F</span></td>
              <td colspan="2"></td>
            </tr>
      
            <tr>
              <td></td>
              <td class="bth" ontouchstart='sendButtonInput("left")' ontouchend='sendButtonInput
                ("stop")'><span>L</span></td>
              <td></td>
              <td class="bth" ontouchstart='sendButtonInput("right")' ontouchend='sendButtonInput
                ("stop")'><span>R</span></td>
              <td></td>
            </tr>
      
            <tr>
              <td colspan="2"></td>
              <td class="btv" ontouchstart='sendButtonInput("back")' ontouchend='sendButtonInput
                ("stop")'><span>B</span></td>
              <td colspan="2"></td>
            </tr>
          </table>
        </td>
        <td>
          <table id="mainTable" style="margin:auto;table-layout:fixed">
            <tr>
              <td style="border: 3px solid #ff0000;"><img id="cameraImage" src="" style="width:380px;height:200px;"></td>
            </tr>
          </table>
        </td>
        <td>
          <table id="mainTable" style="margin:auto;table-layout:fixed">
            <tr>
              <td style="height:10px"></td>
            </tr>
            <tr>
              <td class="blue" colspan="2">Grip</td>
              <td colspan="3">
                <div class="slidecontainer">
                  <input type="range" min="0" max="180" value="90" class="slider" id="grip"
                      oninput='sendButtonInput("grip",value)'>
                </div>
              </td>
            </tr>
            <tr>
              <td style="height:10px"></td>
            </tr>
            <tr>
              <td colspan="2" class="b" style="width:50%">W.Pitch</td>
              <td colspan="3">
                <div class="slidecontainer">
                  <input type="range" min="0" max="180" value="0" class="slider" id="pitch"
                      oninput='sendButtonInput("pitch",value)'>
                </div>
              </td>
            </tr>
            <tr>
              <td style="height:10px"></td>
            </tr>
            <tr class="st">
              <td class="blue" colspan="2">W.Roll</td>
              <td colspan="3">
                <div class="slidecontainer">
                  <input type="range" min="0" max="180" value="90" class="slider" id="roll"
                      oninput='sendButtonInput("roll",value)'>
                </div>
              </td>
            </tr>
            <tr>
              <td style="height:10px"></td>
            </tr>
            <tr>
              <td class="b" colspan="2">Elbow</td>
              <td colspan="3">
                <div class="slidecontainer">
                  <input type="range" min="0" max="180" value="180" class="slider" id="elbow"
                      oninput='sendButtonInput("elbow",value)'>
                </div>
              </td>
            </tr>
            <tr>
              <td style="height:10px"></td>
            </tr>
            <tr>
              <td class="b" colspan="2">Shoulder</td>
              <td colspan="3">
                <div class="slidecontainer">
                  <input type="range" min="0" max="180" value="180" class="slider" id="shoulder"
                      oninput='sendButtonInput("shoulder",value)'>
                </div>
              </td>
            </tr>
            <tr>
              <td style="height:10px"></td>
            </tr>
            <tr>
              <td class="blue" colspan="2">Waist</td>
              <td colspan="3">
                <div class="slidecontainer">
                  <input type="range" min="0" max="180" value="90" class="slider" id="waist"
                      oninput='sendButtonInput("waist",value)'>
                </div>
              </td>
            </tr>
          </table>
        </td>
      </tr>
      <tr>
        <td colspan="9" class="nav" style="width:100%;">
          <table id="mainTable" style="margin:auto;table-layout:fixed">
            <tr>
              <td class="b" colspan="2">Light </td>
              <td colspan="3">
                <div class="slidecontainer">
                  <input type="range" min="0" max="255" value="0" class="slider" id="light"
                    oninput='sendButtonInput("light",value)'>
                </div>
              </td>
              <td colspan="6" style="width:1px;"></td>
              <td class="btn" ontouchstart='sendButtonInput("camrt")' ontouchend='sendButtonInput
              ("")'><span>CamRT</span></td>
              <td colspan="5"></td>
              <td class="g">H Cam</td>
              <td colspan="5"></td>
              <td class="btn" ontouchstart='sendButtonInput("camlt")' ontouchend='sendButtonInput
              ("")'><span>CamLT</span></td>
              <td colspan="5"></td>
            </tr>
          </table>
        </td>
      </tr>
        
      <tr>
        <td colspan="9" class="nav" style="width:100%;">
          <table id="mainTable" style="margin:auto;table-layout:fixed">
            <tr>
              <td class="b" colspan="2">Speed</td>
              <td colspan="3">
                <div class="slidecontainer">
                  <input type="range" min="0" max="255" value="100" class="slider" id="speed"
                    oninput='sendButtonInput("speed",value)'>
                </div>
              </td>
              <td colspan="6"></td>
              <td class="btn" ontouchstart='sendButtonInput("camup")' ontouchend='sendButtonInput
              ("")'><span>CamUP</span></td>
              <td colspan="5"></td>
              <td class="g">V Cam</td>
              <td colspan="5"></td>
              <td class="btn" ontouchstart='sendButtonInput("camd")' ontouchend='sendButtonInput
              ("")'><span>CamD</span></td>
              <td colspan="5"></td>
            </tr>
            </tr>
          </table>
        </td>
      </tr>
    </table>
    <script>
      var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";      
      var websocketCamera;
      var websocketCarInput;
      
      function initCameraWebSocket() 
      {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onopen    = function(event){};
        websocketCamera.onclose   = function(event){setTimeout(initCameraWebSocket, 2000);};
        websocketCamera.onmessage = function(event)
        {
          var imageId = document.getElementById("cameraImage");
          imageId.src = URL.createObjectURL(event.data);
        };
      }
      
      function initCarInputWebSocket() 
      {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen    = function(event)
        {
          var gripButton = document.getElementById("grip");
          sendButtonInput("grip", gripButton.value);
          var pitchButton = document.getElementById("pitch");
          sendButtonInput("pitch", pitchButton.value);
          var rollButton = document.getElementById("roll");
          sendButtonInput("roll", rollButton.value);
          var elbowButton = document.getElementById("elbow");
          sendButtonInput("elbow", elbowButton.value);
          var shoulderButton = document.getElementById("shoulder");
          sendButtonInput("shoulder", shoulderButton.value);
          var waistButton = document.getElementById("waist");
          sendButtonInput("waist", waistButton.value);
          var speedButton = document.getElementById("speed");
          sendButtonInput("speed", speedButton.value);
          var lightButton = document.getElementById("light");
          sendButtonInput("light", lghtButton.value);
        };
        websocketCarInput.onclose   = function(event){setTimeout(initCarInputWebSocket, 2000);};
        websocketCarInput.onmessage = function(event){};        
      }
      
      function initWebSocket() 
      {
        initCameraWebSocket ();
        initCarInputWebSocket();
      }

      function sendButtonInput(key, value) 
      {
        var data = key + "," + value;
        websocketCarInput.send(data);
      }
    
      window.onload = initWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      break;
    case WS_EVT_DISCONNECT:
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        int valueInt = atoi(value.c_str());     
        if (key == "front")
        {
          car = "front";       
        }
        else if (key == "left")
        {
          car = "left";
        }
        else if (key == "right")
        {
          car = "right";        
        }
        else if (key == "back")
        {
          car = "back";        
        }else if (key == "stop")
        {
          car = "stop";        
        }else if (key == "grip")
        {
          servo1 = valueInt;        
        }else if (key == "pitch")
        {
          servo2 = valueInt;        
        }else if (key == "roll")
        {
          servo3 = valueInt;        
        }else if (key == "elbow")
        {
          servo4 = valueInt;        
        }else if (key == "shoulder")
        {
          servo5 = valueInt;        
        }else if (key == "waist")
        {
          servo6 = valueInt;        
        }else if (key == "speed")
        {
          SPEED = valueInt;     
        }else if (key == "light")
        {
          ledcWrite(PWMLightChannel, valueInt);         
        }
        else if (key == "camrt")
        {
          if(millis() - ts >= 50)
          {
            if(h_angle < 180)
            {
              h_angle++;
            }
            ts = millis();
          }
        }else if (key == "camlt")
        {
          if(millis() - ts >= 50)
          {
            if(h_angle > 0)
            {
              h_angle--;
            }
            ts = millis();
          }
        }else if (key == "camup")
        {
          if(millis() - ts >= 50)
          {
            if(v_angle < 180)
            {
              v_angle++;
            }
            ts = millis();
          }
        }else if (key == "camd")
        {
          if(millis() - ts >= 50)
          {
            if(v_angle > 0)
            {
              v_angle--;
            }
            ts = millis();
          }
        }             
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void onCameraWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      cameraClientId = client->id();
      break;
    case WS_EVT_DISCONNECT:
      cameraClientId = 0;
      break;
    case WS_EVT_DATA:
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void setupCamera()
{
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
  
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    return;
  }  

  if (psramFound())
  {
    heap_caps_malloc_extmem_enable(20000);  
  }  
}

void sendCameraPicture()
{
  if (cameraClientId == 0)
  {
    return;
  }
  unsigned long  startTime1 = millis();
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) 
  {
      return;
  }
  unsigned long  startTime2 = millis();
  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);
  while (true)
  {
    AsyncWebSocketClient * clientPointer = wsCamera.client(cameraClientId);
    if (!clientPointer || !(clientPointer->queueIsFull()))
    {
      break;
    }
    delay(1);
  }
  unsigned long  startTime3 = millis();  
}

void setup(void) 
{
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  Serial.println("HTTP server started");

  setupCamera();

  ledcSetup(PWMLightChannel, PWMFreq, PWMResolution);
  pinMode(LIGHT_PIN, OUTPUT); 
  ledcAttachPin(LIGHT_PIN, PWMLightChannel);
}


void loop() 
{
  wsCamera.cleanupClients(); 
  wsCarInput.cleanupClients(); 
  sendCameraPicture(); 

  tm = millis();

  if(tm - td >= 500)
  {
    Serial.print(car);        Serial.print("A");
    Serial.print(servo1);     Serial.print("B");
    Serial.print(servo2);     Serial.print("C");
    Serial.print(servo3);     Serial.print("D");
    Serial.print(servo4);     Serial.print("E");
    Serial.print(servo5);     Serial.print("F");
    Serial.print(servo6);     Serial.print("G");
    Serial.print(v_angle);     Serial.print("H");
    Serial.print(h_angle);     Serial.print("I");
    Serial.print(SPEED);     Serial.print("J");
    Serial.print("\n");
    td = tm;
  }
}

//Code versions//
//esp32 by espressif v3.3.7
//async TCP by ESP32Async v3.4.10
//ESP Async WebServer by ESP32Async v3.10.0
//Adafruit ILI9341 by Adafruit v1.6.3
//Adafruit GFX Library by Adafruit v1.12.4

// Import required libraries
#define CONFIG_ASYNC_TCP_RUNNING_CORE 1
#define CONFIG_ASYNC_TCP_USE_WDT 1

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "Adafruit_ILI9341.h"
#include "Adafruit_GFX.h"
#include <SPI.h>

// ILI9341 TFT LCD pins
#define TFT_CS   5
#define TFT_DC  21

// Replace with your network credentials
const char* ssid = "Wi-Fi";
const char* password = "password";

// Define GPIO numbers
int gpioNo3 = 2;

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 184);

// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);


// screen dimensions
int myWidth;
int myHeight;

// Create TFT object
Adafruit_ILI9341 TFT = Adafruit_ILI9341(TFT_CS, TFT_DC);


// Web page for voice recognition
const char index_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>VIDI X Voice Control</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            color: #e94560;
            text-align: center;
        }
        .container {
            background: rgba(255, 255, 255, 0.05);
            padding: 2rem;
            border-radius: 20px;
            box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
            backdrop-filter: blur(4px);
            border: 1px solid rgba(255, 255, 255, 0.18);
            width: 90%;
            max-width: 400px;
        }
        h2 { margin-bottom: 0.5rem; color: #00d2ff; }
        p { color: #888; margin-bottom: 2rem; }
        #status { font-size: 0.9rem; margin-top: 1rem; color: #aaa; }
        input {
            width: 100%;
            padding: 15px;
            margin: 10px 0;
            border-radius: 10px;
            border: none;
            background: rgba(0, 0, 0, 0.3);
            color: #00d2ff;
            font-size: 1.2rem;
            text-align: center;
            box-sizing: border-box;
        }
        button {
            background: #00d2ff;
            color: #1a1a2e;
            border: none;
            padding: 15px 30px;
            font-size: 1.1rem;
            font-weight: bold;
            border-radius: 50px;
            cursor: pointer;
            transition: transform 0.2s, background 0.2s;
            box-shadow: 0 4px 15px rgba(0, 210, 255, 0.3);
        }
        button:active { transform: scale(0.95); }
        button.recording { background: #e94560; color: white; animation: pulse 1.5s infinite; }
        @keyframes pulse {
            0% { box-shadow: 0 0 0 0 rgba(233, 69, 96, 0.7); }
            70% { box-shadow: 0 0 0 15px rgba(233, 69, 96, 0); }
            100% { box-shadow: 0 0 0 0 rgba(233, 69, 96, 0); }
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>VIDI X</h2>
        <p>Voice Control</p>
        <input type="text" id="speechToText" placeholder="..." readonly>
        <br>
        <button id="recordBtn" onclick="toggleRecognition()">Start Microphone</button>
        <div id="status">Ready</div>
    </div>

    <script>
        let recognition;
        let isRecording = false;

        function toggleRecognition() {
            if (isRecording) {
                recognition.stop();
                return;
            }

            if (!('webkitSpeechRecognition' in window)) {
                alert("Your browser does not support speech recognition. Please use Chrome.");
                return;
            }

            recognition = new webkitSpeechRecognition();
            recognition.lang = "en-US";
            recognition.interimResults = true;

            recognition.onstart = () => {
                isRecording = true;
                const btn = document.getElementById("recordBtn");
                btn.innerText = "Listening...";
                btn.classList.add("recording");
                document.getElementById("status").innerText = "Listening for commands...";
            };

            recognition.onresult = (event) => {
                const text = event.results[0][0].transcript;
                document.getElementById("speechToText").value = text;
                
                if (event.results[0].isFinal) {
                    fetch("/stt?text=" + encodeURIComponent(text));
                }
            };

            recognition.onerror = (event) => {
                console.error(event);
                stopUI();
            };

            recognition.onend = () => {
                stopUI();
            };

            recognition.start();
        }

        function stopUI() {
            isRecording = false;
            const btn = document.getElementById("recordBtn");
            btn.innerText = "Start Microphone";
            btn.classList.remove("recording");
            document.getElementById("status").innerText = "Sent to VIDI X";
        }
    </script>
</body>
</html>
)=====";

volatile bool newCommandReceived = false;
char voiceBuffer[64];

// main function for voice parsing
void parseVoiceCommand(const char* text) {
  String cmd = String(text); // Convert text to a "String" so we can compare it easily
  cmd.toLowerCase();         // Make it lowercase so "Light" and "light" both work

  TFT.print("You said: ");
  TFT.println(cmd);

  // Check which command was said
  if (cmd == "light" || cmd == "light.") {
    digitalWrite(gpioNo3, !digitalRead(gpioNo3));   
    TFT.println("Toggling light!");
  }
  else if (cmd == "kitchen" || cmd == "kitchen.") {
    digitalWrite(13, !digitalRead(13));
    TFT.println("Toggling kitchen!");
  }
  else if (cmd == "living room" || cmd == "living room.") {
    digitalWrite(14, !digitalRead(14));
    TFT.println("Toggling living room!");
  }
  else if (cmd == "bedroom" || cmd == "bedroom.") {
    digitalWrite(27, !digitalRead(27));
    TFT.println("Toggling bedroom!");
  }
  else if (cmd == "hallway" || cmd == "hallway.") {
    digitalWrite(4, !digitalRead(4));
    TFT.println("Toggling hallway!");
  } 
  else {
    TFT.println("Didn't understand");
  }
}


void setup(){
  Serial.begin(115200);

  // 1. Initialize Pins first
  pinMode(4, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(gpioNo3, OUTPUT);

  // 2. Initialize Screen
  TFT.begin();
  TFT.setRotation(3);
  TFT.fillScreen(ILI9341_BLACK);
  TFT.setTextColor(ILI9341_GREEN);
  TFT.println("System is starting...");

  // 3. Start WiFi with corrected subnet
  WiFi.mode(WIFI_STA); 
  // Check if your router is actually 255.255.255.0!
  if (!WiFi.config(local_IP, gateway, IPAddress(255, 255, 255, 0), primaryDNS)) {
    Serial.println("STA Failed");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected");
  TFT.println("WiFi Connected!");
  TFT.println(WiFi.localIP());

  // 4. Routes
  // Serve the main web page at the root address
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_page);
  });

  server.on("/stt", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Check if parameter exists without creating a String object yet
    if (request->hasParam("text")) {
        const AsyncWebParameter* p = request->getParam("text");
        // Copy the raw buffer directly
        size_t len = p->value().length();
        if (len > 63) len = 63;
        memcpy(voiceBuffer, p->value().c_str(), len);
        voiceBuffer[len] = '\0';
        newCommandReceived = true;
    }
    request->send(200, "text/html", "OK"); 
  });

  server.begin();
}

void loop() {
  if (newCommandReceived) {
    // Disable interrupts or use a mutex if you want to be 100% safe, 
    // but clearing the flag first is usually enough for this simple setup.
    newCommandReceived = false; 
    parseVoiceCommand(voiceBuffer);
  }
  
  // Give the background tasks a moment to breathe
  delay(1); 
}
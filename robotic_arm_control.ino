#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <firebase_config.h>
#include <ESP32Servo.h>
#include <vector>

struct ServoPins {
    Servo servo;
    int servoPin;
    String servoName;
    int initialPosition;
};

struct RecordedStep {
    int servoIndex;
    int value;
    unsigned long timestamp;  // Timestamp when the step was recorded
};

std::vector<ServoPins> servoPins = {
    { Servo(), 23, "Base", 90 },
    { Servo(), 22, "Shoulder", 90 },
    { Servo(), 19, "Elbow", 90 },
    { Servo(), 18, "Gripper", 90 },
};

std::vector<RecordedStep> recordedSteps;
bool recordSteps = false;
bool playRecordedSteps = false;
unsigned long recordStartTime = 0;
unsigned long playbackStartTime = 0;
size_t currentPlaybackStep = 0;

const char* ssid = "";
const char* password = "";

AsyncWebServer server(80);
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
      input[type=button] {
        background-color: red;
        color: white;
        border-radius: 30px;
        width: 100%;
        height: 40px;
        font-size: 20px;
        text-align: center;
        border: none;
        cursor: pointer;
        transition: background-color 0.3s;
      }
      
      .noselect {
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
      }

      .slidecontainer {
        width: 100%;
      }

      .slider {
        -webkit-appearance: none;
        width: 100%;
        height: 20px;
        border-radius: 5px;
        background: #d3d3d3;
        outline: none;
        opacity: 0.7;
        -webkit-transition: opacity .2s;
        transition: opacity .2s;
      }

      .slider:hover {
        opacity: 1;
      }
  
      .slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 40px;
        height: 40px;
        border-radius: 50%;
        background: red;
        cursor: pointer;
      }

      .slider::-moz-range-thumb {
        width: 40px;
        height: 40px;
        border-radius: 50%;
        background: red;
        cursor: pointer;
      }

      .status-indicator {
        width: 15px;
        height: 15px;
        border-radius: 50%;
        display: inline-block;
        margin-left: 10px;
        background-color: gray;
      }

      .status-text {
        font-size: 14px;
        margin-left: 5px;
        color: gray;
      }

      .status-container {
        display: flex;
        align-items: center;
        margin-top: 10px;
      }

    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <h1 style="color: teal;text-align:center;">Serial Servo Control</h1>
    <h2 style="color: teal;text-align:center;">Robot Arm</h2>
    
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr/><tr/>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Gripper:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Gripper" oninput='sendButtonInput("Gripper",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Elbow:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Elbow" oninput='sendButtonInput("Elbow",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Shoulder:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Shoulder" oninput='sendButtonInput("Shoulder",value)'>
          </div>
        </td>
      </tr>  
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Base:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Base" oninput='sendButtonInput("Base",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Record:</b></td>
        <td>
          <input type="button" id="Record" value="OFF" ontouchend='onclickButton(this)' onclick='onclickButton(this)'>
        </td>
        <td>
          <div class="status-container">
            <div id="recordIndicator" class="status-indicator"></div>
            <span id="recordStatus" class="status-text">Idle</span>
          </div>
        </td>
      </tr>
      <tr/><tr/> 
      <tr>
        <td style="text-align:left;font-size:25px"><b>Play:</b></td>
        <td>
          <input type="button" id="Play" value="OFF" ontouchend='onclickButton(this)' onclick='onclickButton(this)'>
        </td>
        <td>
          <div class="status-container">
            <div id="playIndicator" class="status-indicator"></div>
            <span id="playStatus" class="status-text">Idle</span>
          </div>
        </td>
      </tr>      
    </table>
  
    <script>
      var webSocketRobotArmInputUrl = "ws:\/\/" + window.location.hostname + "/RobotArmInput";      
      var websocketRobotArmInput;
      
      function initRobotArmInputWebSocket() {
        websocketRobotArmInput = new WebSocket(webSocketRobotArmInputUrl);
        websocketRobotArmInput.onopen = function(event) {
          console.log("WebSocket connected!");
          updateStatusIndicator('recordIndicator', 'recordStatus', 'Connected', '#32CD32');
          updateStatusIndicator('playIndicator', 'playStatus', 'Connected', '#32CD32');
        };

        websocketRobotArmInput.onclose = function(event) {
          console.log("WebSocket disconnected!");
          updateStatusIndicator('recordIndicator', 'recordStatus', 'Disconnected', '#FF0000');
          updateStatusIndicator('playIndicator', 'playStatus', 'Disconnected', '#FF0000');
          setTimeout(initRobotArmInputWebSocket, 2000);
        };

        websocketRobotArmInput.onmessage = function(event) {
          var keyValue = event.data.split(",");
          for (var i = 0; i < keyValue.length; i += 2) {
            var key = keyValue[i];
            var value = keyValue[i + 1];
            var button = document.getElementById(key);
            
            if (button) {
              button.value = value;
              if (button.id == "Record" || button.id == "Play") {
                button.style.backgroundColor = (value == "ON" ? "green" : "red");
                enableDisableButtonsSliders(button);
                updateStatusIndicator(
                  button.id.toLowerCase() + 'Indicator',
                  button.id.toLowerCase() + 'Status',
                  value == "ON" ? "Active" : "Idle",
                  value == "ON" ? "#32CD32" : "gray"
                );
              } else {
                button.value = value;
              }
            }
          }
        };
      }
      
      function updateStatusIndicator(indicatorId, statusId, text, color) {
        document.getElementById(indicatorId).style.backgroundColor = color;
        document.getElementById(statusId).textContent = text;
        document.getElementById(statusId).style.color = color;
      }
      
      function sendButtonInput(key, value) {
        var data = key + "," + value;
        if (websocketRobotArmInput.readyState === WebSocket.OPEN) {
          websocketRobotArmInput.send(data);
        }
      }
      
      function onclickButton(button) {
        button.value = (button.value == "ON") ? "OFF" : "ON";
        button.style.backgroundColor = (button.value == "ON" ? "green" : "red");
        var value = (button.value == "ON") ? 1 : 0;
        sendButtonInput(button.id, value);
        enableDisableButtonsSliders(button);
      }
      
      function enableDisableButtonsSliders(button) {
        if (button.id == "Play") {
          var disabled = button.value == "ON" ? "none" : "auto";
          document.getElementById("Gripper").style.pointerEvents = disabled;
          document.getElementById("Elbow").style.pointerEvents = disabled;
          document.getElementById("Shoulder").style.pointerEvents = disabled;
          document.getElementById("Base").style.pointerEvents = disabled;
          document.getElementById("Record").style.pointerEvents = disabled;
        }
        if (button.id == "Record") {
          var disabled = button.value == "ON" ? "none" : "auto";
          document.getElementById("Play").style.pointerEvents = disabled;
        }
      }
           
      window.onload = initRobotArmInputWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event) {
        event.preventDefault()
      });      
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "File Not Found");
}

void sendCurrentRobotArmState() {
    String state = "";
    for (size_t i = 0; i < servoPins.size(); i++) {
        state += servoPins[i].servoName + "," + String(servoPins[i].servo.read()) + ",";
    }
    state += "Record," + String(recordSteps ? "ON" : "OFF") + ",";
    state += "Play," + String(playRecordedSteps ? "ON" : "OFF");
    wsRobotArmInput.textAll(state);
}

void startRecording() {
    recordedSteps.clear();
    recordSteps = true;
    recordStartTime = millis();
    Serial.println("Started recording");
}

void stopRecording() {
    recordSteps = false;
    Serial.printf("Stopped recording. Total steps: %d\n", recordedSteps.size());
    // Print recorded sequence for debugging
    for (const auto& step : recordedSteps) {
        Serial.printf("Servo: %d, Position: %d, Timestamp: %lu\n", 
            step.servoIndex, step.value, step.timestamp);
    }
}

void startPlayback() {
    if (recordedSteps.empty()) {
        Serial.println("No steps to play");
        playRecordedSteps = false;
        return;
    }
    playRecordedSteps = true;
    playbackStartTime = millis();
    currentPlaybackStep = 0;
    Serial.println("Started playback");
}

void stopPlayback() {
    playRecordedSteps = false;
    currentPlaybackStep = 0;
    Serial.println("Stopped playback");
}

void onRobotArmInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len
) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            sendCurrentRobotArmState();
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            {
                String message = "";
                for (size_t i = 0; i < len; i++) {
                    message += (char)data[i];
                }

                int separatorIndex = message.indexOf(',');
                String key = message.substring(0, separatorIndex);
                String value = message.substring(separatorIndex + 1);

                if (key == "Record") {
                    if (value == "1") {
                        startRecording();
                    } else {
                        stopRecording();
                    }
                }
                else if (key == "Play") {
                    if (value == "1") {
                        startPlayback();
                    } else {
                        stopPlayback();
                    }
                }
                else {
                    // Handle servo movement
                    for (size_t i = 0; i < servoPins.size(); i++) {
                        if (servoPins[i].servoName == key) {
                            int pos = value.toInt();
                            servoPins[i].servo.write(pos);
                            
                            if (recordSteps) {
                                // Record the step with timestamp
                                RecordedStep step = {
                                    static_cast<int>(i),
                                    pos,
                                    millis() - recordStartTime
                                };
                                recordedSteps.push_back(step);
                                Serial.printf("Recorded step: Servo %d to %d at %lu ms\n", 
                                    i, pos, step.timestamp);
                            }
                            break;
                        }
                    }
                }
                sendCurrentRobotArmState();
            }
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing...");
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nConnected to WiFi");
    Serial.println(WiFi.localIP());

    // Initialize servos
    for (size_t i = 0; i < servoPins.size(); i++) {
        servoPins[i].servo.attach(servoPins[i].servoPin);
        servoPins[i].servo.write(servoPins[i].initialPosition);
        delay(500);
    }

    server.on("/", HTTP_GET, handleRoot);
    server.onNotFound(handleNotFound);
    
    wsRobotArmInput.onEvent(onRobotArmInputWebSocketEvent);
    server.addHandler(&wsRobotArmInput);
    
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    wsRobotArmInput.cleanupClients();  // Clean up disconnected clients
    
    if (playRecordedSteps && !recordedSteps.empty()) {
        unsigned long currentTime = millis();
        unsigned long playbackElapsed = currentTime - playbackStartTime;

        // Check if we need to move to the next step
        while (currentPlaybackStep < recordedSteps.size() && 
               playbackElapsed >= recordedSteps[currentPlaybackStep].timestamp) {
            
            const RecordedStep& step = recordedSteps[currentPlaybackStep];
            
            // Execute the movement
            if (step.servoIndex >= 0 && step.servoIndex < servoPins.size()) {
                servoPins[step.servoIndex].servo.write(step.value);
                Serial.printf("Playing step %d: Servo %d to %d at %lu ms\n", 
                    currentPlaybackStep, step.servoIndex, step.value, playbackElapsed);
            }
            
            currentPlaybackStep++;
            
            // If we've reached the end of the sequence
            if (currentPlaybackStep >= recordedSteps.size()) {
                Serial.println("Playback complete");
                stopPlayback();
                sendCurrentRobotArmState(); // Update UI state
                break;
            }
        }
    }

    // Optional: Add a small delay to prevent overwhelming the ESP32
    delay(10);
}
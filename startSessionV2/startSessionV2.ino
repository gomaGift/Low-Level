#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
String sessionId;
struct Student {
    String studentId;
    String fingerprintTemplate;  // Changed from fingerprintTemplate to fullName as per your original example
};
std::vector<Student> students;

// Wi-Fi credentials
const char* wlan_ssid = "Self";
const char* wlan_password = "59515852";

// WebSocket server configuration
const char* ws_host = "192.168.1.116";
const int ws_port = 8053;

// Base URL for WebSocket connection
const char* ws_baseurl = "/attendance/";

// Unique device ID for this ESP32 (Change this to match your device's ID)
const char* deviceId = "CS";

WebSocketsClient webSocket;

void setup() {
  Serial.begin(115200);
  connectToWifi();
  connectToWebSocket();
}

void loop() {
  webSocket.loop();
}

// Handle WebSocket events
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      // Once connected, subscribe to the device-specific topic
      subscribeToTopic(deviceId);
      break;
    case WStype_TEXT:
      {  String text = (char*) payload;
        if (payload[0] == 'h') {

          Serial.println("Heartbeat!");

        } 
        
        else if (payload[0] == 'o') {

          // on open connection
          char *msg = "[\"CONNECT\\naccept-version:1.1,1.0\\nheart-beat:10000,10000\\n\\n\\u0000\"]";
          webSocket.sendTXT(msg);
          delay(1000);

        } 
      
        else if (text.startsWith("a[\"MESSAGE")) {
          processJsonData(text);
        }

        break;
        }
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }
}


// Subscribe to a specific topic based on the device ID
void subscribeToTopic(const char* deviceId) {
  // Construct a STOMP subscription message
  String msg = "[\"SUBSCRIBE\\nid:sub-0\\ndestination:/topic/sessions/" + String(deviceId) + "\\n\\n\\u0000\"]";
  Serial.printf("Subscribing to topic: /topic/sessions/%s\n", deviceId);
  webSocket.sendTXT(msg);
}

// Process JSON data and print the student details
void processJsonData(String _received) {
  String json = extractString(_received);
  json.replace("\\", "");
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, json);
  sessionId = doc["sessionId"].as<String>();
  // Clear the previous student data
  students.clear();
  // Access student information and store in the global vector
  for (JsonObject student : doc["students"].as<JsonArray>()) {
      Student newStudent;
      newStudent.studentId = student["studentId"].as<String>();
      newStudent.fingerprintTemplate = student["fingerprintTemplate"].as<String>();
      students.push_back(newStudent);
  }
  // Call markAttendance function to process and print student details
  markAttendance();


}

String extractString(String _received) {
  char startingChar = '{';
  char finishingChar = '}';
  char arrayClosingChar = ']';

  String tmpData = "";
  bool _flag = false;
  for (int i = 0; i < _received.length(); i++) {
    char tmpChar = _received[i];
    if (tmpChar == startingChar) {
      tmpData += startingChar;
      _flag = true;
    }
    else if (tmpChar == finishingChar) {
      tmpData += finishingChar;
      char nextChar = _received[i+1];
      if (nextChar)
           continue;
      break;
    }
    else if (_flag == true) {
      tmpData += tmpChar;
    }
  }

  return tmpData;

}
// Function to connect to Wi-Fi
void connectToWifi() {
  delay(500);
  Serial.print("Connecting to WLAN: ");
  Serial.print(wlan_ssid);
  Serial.print(" ...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wlan_ssid, wlan_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" success.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect to WebSocket server
void connectToWebSocket() {
  String socketUrl = ws_baseurl;
  socketUrl += random(0, 999);
  socketUrl += "/";
  socketUrl += random(0, 999999);
  socketUrl += "/websocket";

  // Connect to the WebSocket server
  Serial.print("Connecting to WebSocket URL: ");
  Serial.println(String(ws_host) + ":" + String(ws_port) + socketUrl);

  webSocket.begin(ws_host, ws_port, socketUrl);
  webSocket.onEvent(webSocketEvent);
}

void markAttendance() {
    Serial.println("Marking attendance for the following students:");
    for (const Student& student : students) {
        Serial.printf("Student ID: %s, Fingerprint: %s\n", student.studentId.c_str(), student.fingerprintTemplate.c_str());
    }
}

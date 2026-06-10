#include "arduino_secrets.h"
#include <WiFi.h>
#include <HTTPClient.h>

// --- WIFI & THINGSPEAK SETTINGS ---
const char* ssid = "";
const char* password = ""; // 
String apiKey = "";        

unsigned long lastThingSpeakTime = 0;

// --- CLOUD GRAPH VARIABLES (Replacing the old Arduino Cloud ones) ---
float tremor_hz = 0.0;
long motor_rpm = 0;
long run_duration = 0;

// --- PINS ---
const int escPin = 27;  
const int vibSensorPin = 4; // SW-420 Gatekeeper
const int xPin = 34;
const int yPin = 35;
const int zPin = 32;

// --- ESC PWM SETTINGS ---
const int pwmFreq = 50;
const int pwmChannel = 0;
const int pwmResolution = 14; 
const int STOP_DUTY = 820;   
const int MAX_DUTY = 1640;   

// --- TIMING VARIABLES ---
unsigned long runDurationMs = 0;      
unsigned long cooldownDuration = 0; 

// --- STATE MACHINE TRACKING ---
enum SystemState { IDLE, MOTOR_ON, COOLDOWN };
SystemState currentState = IDLE;
unsigned long stateTimer = 0;
int currentMotorDuty = STOP_DUTY;

void setup() {
  Serial.begin(9600);
  delay(1500); // Give serial monitor time to connect

  // --- WIFI SETUP ---
  Serial.print("Connecting to WiFi hotspot: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected! Ready to send to ThingSpeak.");

  // --- HARDWARE SETUP ---
  pinMode(vibSensorPin, INPUT);
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(escPin, pwmChannel);

  Serial.println("Arming ESC...");
  ledcWrite(pwmChannel, STOP_DUTY); 
  delay(3000); 
  Serial.println("--- SYSTEM READY ---");
}

// --- CORE SENSOR FUNCTION (ADXL335) ---
float getCleanSensorReading() {
  const int numReadings = 100;
  int xReadings[numReadings], yReadings[numReadings], zReadings[numReadings];
  int xMin = 4095, xMax = 0, yMin = 4095, yMax = 0, zMin = 4095, zMax = 0;
  long xSum = 0, ySum = 0, zSum = 0;
  
  for(int i = 0; i < numReadings; i++) {
    xReadings[i] = analogRead(xPin); yReadings[i] = analogRead(yPin); zReadings[i] = analogRead(zPin);
    if(xReadings[i] < xMin) xMin = xReadings[i]; if(xReadings[i] > xMax) xMax = xReadings[i];
    if(yReadings[i] < yMin) yMin = yReadings[i]; if(yReadings[i] > yMax) yMax = yReadings[i];
    if(zReadings[i] < zMin) zMin = zReadings[i]; if(zReadings[i] > zMax) zMax = zReadings[i];
    xSum += xReadings[i]; ySum += yReadings[i]; zSum += zReadings[i];
    delay(2); 
  }
  
  int xAmp = xMax - xMin, yAmp = yMax - yMin, zAmp = zMax - zMin;
  
  // Hardware Noise Filter
  if (xAmp < 400 && yAmp < 400 && zAmp < 400) return 0.0; 
  
  int xAvg = xSum / numReadings, yAvg = ySum / numReadings, zAvg = zSum / numReadings;
  int crosses = 0; bool isAbove = false; int hysteresis = 100; 
  
  for(int i = 0; i < numReadings; i++) {
    int energy = abs(xReadings[i] - xAvg) + abs(yReadings[i] - yAvg) + abs(zReadings[i] - zAvg);
    if (energy > hysteresis) { if (!isAbove) { isAbove = true; crosses++; } } 
    else { isAbove = false; }
  }
  return (float)crosses * 5.0; 
}

void loop() {
  // --- THINGSPEAK UPLOAD (Fires only once every 15 seconds) ---
  if (millis() - lastThingSpeakTime > 15000) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      
      String serverPath = "http://api.thingspeak.com/update?api_key=" + apiKey + 
                          "&field1=" + String(tremor_hz) + 
                          "&field2=" + String(motor_rpm) + 
                          "&field3=" + String(run_duration);
      
      http.begin(serverPath.c_str());
      int httpResponseCode = http.GET();
      
      if (httpResponseCode == 200) {
        Serial.println("--> Data graphed on ThingSpeak!");
      } 
      http.end();
      lastThingSpeakTime = millis();
    }
  }

  // --- SENSOR & MOTOR STATE MACHINE ---
  switch (currentState) {
    
    case IDLE:
      {
        if (digitalRead(vibSensorPin) == HIGH) {
          
          float hz = getCleanSensorReading();
          
          if (hz >= 4.0) {
            float safeHz = hz;
            if (safeHz > 12.0) safeHz = 12.0; 

            long targetRPM = map(safeHz * 10, 40, 120, 9000, 11400); 
            currentMotorDuty = map(targetRPM, 0, 20000, STOP_DUTY, MAX_DUTY);
            if (currentMotorDuty > MAX_DUTY) currentMotorDuty = MAX_DUTY;
            if (currentMotorDuty < STOP_DUTY) currentMotorDuty = STOP_DUTY;

            if (safeHz >= 4.0 && safeHz < 8.0) {
              runDurationMs = 5000; cooldownDuration = 7000; 
            } else if (safeHz >= 8.0) {
              runDurationMs = 7000; cooldownDuration = 14000; 
            }

            // Update variables so ThingSpeak loop catches them
            tremor_hz = safeHz;
            motor_rpm = targetRPM;
            run_duration = runDurationMs / 1000; // Convert to seconds

            Serial.print("Tremor Detected! ");
            Serial.print(tremor_hz); Serial.println(" Hz");

            ledcWrite(pwmChannel, currentMotorDuty);
            stateTimer = millis(); 
            currentState = MOTOR_ON; 
          } 
        } else {
          ledcWrite(pwmChannel, STOP_DUTY);
          
          // Drop graph back to 0 when hand is still
          tremor_hz = 0.0;
          motor_rpm = 0;
          run_duration = 0;
        }
      }
      break;

    case MOTOR_ON:
      if (millis() - stateTimer >= runDurationMs) {
        ledcWrite(pwmChannel, STOP_DUTY); 
        stateTimer = millis();
        currentState = COOLDOWN;
        
        // --- ADDED: Entering Cooldown Output ---
        Serial.print(">>> Motor stopped. Entering COOLDOWN for ");
        Serial.print(cooldownDuration / 1000);
        Serial.println(" seconds...");
        
        // Drop graph to 0 during cooldown
        tremor_hz = 0.0;
        motor_rpm = 0;
        run_duration = 0;
      }
      break;

    case COOLDOWN:
      if (millis() - stateTimer >= cooldownDuration) {
        currentState = IDLE;
        
        // --- ADDED: Exiting Cooldown Output ---
        Serial.println(">>> COOLDOWN complete. System IDLE and ready for next tremor.");
      }
      break;
  }
}
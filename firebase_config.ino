#ifndef FIREBASE_CONFIG_H
#define FIREBASE_CONFIG_H

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Network Configuration
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase Project Configuration
#define FIREBASE_API_KEY "YOUR_FIREBASE_API_KEY"
#define FIREBASE_PROJECT_ID "YOUR_FIREBASE_PROJECT_ID"
#define DATABASE_URL "https://your-project-default-rtdb.firebaseio.com/"

// Authentication Configuration
#define USER_EMAIL "admin@robotarm.com"
#define USER_PASSWORD "StrongPassword123!"

// Logging Configurations
#define MAX_SERVO_LOG_ENTRIES 1000
#define MAX_SESSION_LOG_ENTRIES 100
#define LOG_BUFFER_SIZE 512

// Servo Configuration Struct
struct ServoLogEntry {
    String servoName;
    int position;
    unsigned long timestamp;
    String sessionId;
};

// Recording Session Struct
struct RecordingSessionLog {
    String sessionId;
    unsigned long startTime;
    unsigned long endTime;
    int totalSteps;
    String status;
};

// Firebase Management Class
class FirebaseManager {
private:
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    
    // Internal logging buffers
    ServoLogEntry servoLogBuffer[MAX_SERVO_LOG_ENTRIES];
    RecordingSessionLog sessionLogBuffer[MAX_SESSION_LOG_ENTRIES];
    
    size_t servoLogIndex = 0;
    size_t sessionLogIndex = 0;

public:
    // Core Firebase Management Methods
    bool initialize();
    bool authenticate();
    void processLogs();

    // Servo Logging Methods
    void logServoPosition(const String& servoName, int position);
    void logRecordingSession(bool isRecording);
    
    // Advanced Logging Methods
    String generateUniqueSessionId();
    void clearOldLogs();
    
    // Error Handling and Diagnostics
    bool isFirebaseReady();
    String getLastError();
};

// Global Firebase Manager Instance
extern FirebaseManager firebaseManager;

// Utility Functions
String generateRandomString(int length);
unsigned long getCurrentTimestamp();

#endif // FIREBASE_CONFIG_H
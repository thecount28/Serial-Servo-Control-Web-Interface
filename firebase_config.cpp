#include "firebase_config.h"
#include "secrets.h"

FirebaseManager firebaseManager;

bool FirebaseManager::initialize() {
    // Configure Firebase settings
    config.api_key = FIREBASE_API_KEY;
    config.database_url = DATABASE_URL;
    
    // Configure authentication
    auth.user.email = ADMIN_EMAIL;
    auth.user.password = ADMIN_PASSWORD;
    
    // Set up token callback
    config.token_status_callback = tokenStatusCallback;
    
    // Initialize Firebase
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    return Firebase.ready();
}

bool FirebaseManager::authenticate() {
    // Attempt authentication
    if (Firebase.ready()) {
        Serial.println("Firebase Authentication Successful");
        return true;
    }
    
    Serial.println("Firebase Authentication Failed");
    return false;
}

void FirebaseManager::logServoPosition(const String& servoName, int position) {
    if (!Firebase.ready()) return;
    
    // Create log entry
    String path = "/servo_logs/" + generateUniqueSessionId();
    
    FirebaseJson json;
    json.add("servo", servoName);
    json.add("position", position);
    json.add("timestamp", millis());
    
    // Send to Firebase
    if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
        Serial.printf("Logged %s servo position: %d\n", servoName.c_str(), position);
    } else {
        Serial.println("Logging Failed: " + fbdo.errorReason());
    }
}

String FirebaseManager::generateUniqueSessionId() {
    // Simple unique ID generation
    return String(millis()) + "_" + String(random(10000));
}
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>
#include <PDM.h>

short sampleBuffer[256];
volatile int samplesRead = 0;

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while(1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }

  PDM.onReceive(onPDMdata);

  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM microphone.");
    while(1);
  }
}

void loop() {
  static float movement = 1.0; 
  static int proximity = 255;
  static int c = 1000;
  static int sound_level = 0;
  float x, y, z;
  int r, g, b;

  // imu
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    movement = sqrt(x * x + y * y + z * z);
  }

  // proximity
  if (APDS.proximityAvailable()) {
    proximity = APDS.readProximity();
  }

  // light
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
  }

  // microphone
  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    sound_level = sum / samplesRead;
    samplesRead = 0;
  }
  
  bool sound = sound_level > 200;
  bool dark = c < 1000;
  bool moving = movement > 1;
  bool near = proximity < 50;

  String states[] = {"QUIET_BRIGHT_STEADY_FAR", "NOISY_BRIGHT_STEADY_FAR", "QUIET_DARK_STEADY_NEAR", "NOISY_BRIGHT_MOVING_NEAR"};
  static int matches[] = {0, 0, 0, 0};

  if (sound) {
    matches[1]++;
    matches[3]++;
  } else {
    matches[0]++;
    matches[2]++;
  }

  if (dark) {
    matches[2]++;
  } else {
    matches[0]++;
    matches[1]++;
    matches[3]++;
  }

  if (moving) {
    matches[3]++;
  } else {
    matches[0]++;
    matches[1]++;
    matches[2]++;
  }

  if (near) {
    matches[2]++;
    matches[3]++;
  } else {
    matches[0]++;
    matches[1]++;
  }

  static unsigned long lastPrintTime = 0;

  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();

    int stateIndex = 0;
    int mostMatches = matches[0];
    for (int i = 1; i < 4; i++) {
      if (matches[i] > mostMatches) {
        mostMatches = matches[i];
        stateIndex = i;
      }
    }

    Serial.print("raw,mic=");
    Serial.print(sound_level);
    Serial.print(",clear=");
    Serial.print(c);
    Serial.print(",motion=");
    Serial.print(movement);
    Serial.print(",prox=");
    Serial.println(proximity);

    Serial.print("flags,sound=");
    Serial.print(sound);
    Serial.print(",dark=");
    Serial.print(dark);
    Serial.print(",moving=");
    Serial.print(moving);
    Serial.print(",near=");
    Serial.println(near);

    Serial.print("state,");
    Serial.println(states[stateIndex]);
    Serial.println();
    
    for (int i = 0; i < 4; i++) matches[i] = 0;
  }
}

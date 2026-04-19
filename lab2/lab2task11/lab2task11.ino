#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>
#include <Arduino_HS300x.h>

void setup() {
  Serial.begin(115200);
  delay(1500);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU.");
    while(1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while(1);
  }

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize humidity/temperature sensor.");
    while (1);
  }
}

void loop() {
  static float field, prevField = 0;
  static int r, g, b, c, prevR = 0, prevG = 0, prevB = 0, prevC = 0;
  static float temp, prevTemp = 0;
  static float humid, prevHumid = 0;

  // Magnetic field
  if (IMU.magneticFieldAvailable()) {
    float x, y, z;
    IMU.readMagneticField(x, y, z);
    field = sqrt(x * x + y * y + z * z);
  }

  // Light and color
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
  }

  static String events[] = {"BASELINE_NORMAL", "BREATH_OR_WARM_AIR_EVENT", "MAGNETIC_DISTURBANCE_EVENT", "LIGHT_OR_COLOR_CHANGE_EVENT"};
  static unsigned long lastPrintTime = 0;
  static unsigned long lastBaselineTime = 0;

  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();

    // Temperature and humidity
    temp = HS300x.readTemperature();
    humid = HS300x.readHumidity();

    bool humidJump = false, tempRise = false, magShift = false, lightOrColorChange = false;

    if (humid - prevHumid > 10) {
      humidJump = true;
    }
    if (temp - prevTemp > 5) {
      tempRise = true;
    }
    if (abs(field - prevField) > 10) {
      magShift = true;
    }
    if (abs(r - prevR) > 200 || abs(g - prevG) > 200 || abs(b - prevB) > 200 || abs(c - prevC) > 200) {
      lightOrColorChange = true;
    }

    int eventIndex = 0;
    if (humidJump || tempRise) {
      eventIndex = 1;
    } else if (magShift) {
      eventIndex = 2;
    } else if (lightOrColorChange) {
      eventIndex = 3;
    }

    Serial.print("raw,rh=");
    Serial.print(humid);
    Serial.print(",temp=");
    Serial.print(temp);
    Serial.print(",mag=");
    Serial.print(field);
    Serial.print(",r=");
    Serial.print(r);
    Serial.print(",g=");
    Serial.print(g);
    Serial.print(",b=");
    Serial.print(b);
    Serial.print(",clear=");
    Serial.println(c);

    Serial.print("flags,humid_jump=");
    Serial.print(humidJump);
    Serial.print(",temp_rise=");
    Serial.print(tempRise);
    Serial.print(",mag_shift=");
    Serial.print(magShift);
    Serial.print(",light_or_color_change=");
    Serial.println(lightOrColorChange);

    Serial.print("event,");
    Serial.println(events[eventIndex]);
    Serial.println();

    prevField = field;
    prevR = r;
    prevG = g;
    prevB = b;
    prevC = c;
  }

  // Update temperature and humidity
  if (millis() - lastBaselineTime >= 1000) {
    lastBaselineTime = millis();
    prevTemp = temp;
    prevHumid = humid;
  }
}

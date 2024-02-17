// Include the libraries for ESP32 and OLED display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define the pins for the battery voltage sensor and the OLED display
#define BATTERY_PIN 34
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create an object for the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Define some constants for the battery monitor
#define VOLTAGE_THRESHOLD 12.0 // The voltage threshold to determine charging or discharging
#define MEASURE_INTERVAL 600000 // The interval to measure voltage in milliseconds (10 minutes)
#define MEASURE_COUNT 72 // The number of measurements to store in an array (12 hours)
#define STORAGE_SIZE 128 // The size of the storage array
#define ANALYSIS_HOUR 5 // The hour to use for analysis (5th hour)

// Declare some global variables for the battery monitor
float voltageArray[MEASURE_COUNT]; // The array to store the voltage measurements
float storageArray[STORAGE_SIZE][MEASURE_COUNT]; // The array to store the voltage arrays
int voltageIndex = 0; // The index to store the current voltage measurement
int storageIndex = 0; // The index to store the current voltage array
bool isCharging = false; // The flag to indicate if the battery is charging or not
unsigned long lastMeasureTime = 0; // The time of the last voltage measurement

// The setup function runs once when the ESP32 starts
void setup() {
  // Initialize the serial monitor for debugging
  Serial.begin(115200);

  // Initialize the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Car Battery Monitor");
  display.display();

  // Initialize the battery voltage sensor
  pinMode(BATTERY_PIN, INPUT);
}

// The loop function runs repeatedly after the setup function
void loop() {
  // Get the current time in milliseconds
  unsigned long currentTime = millis();

  // Check if it is time to measure the battery voltage
  if (currentTime - lastMeasureTime >= MEASURE_INTERVAL) {
    // Measure the battery voltage and map it to a range of 0 to 15 volts
    int sensorValue = analogRead(BATTERY_PIN);
    float voltage = map(sensorValue, 0, 4095, 0, 1500) / 100.0;

    // Print the voltage to the serial monitor
    Serial.print("Voltage: ");
    Serial.println(voltage);

    // Store the voltage in the voltage array
    voltageArray[voltageIndex] = voltage;
    voltageIndex++;

    // Check if the voltage array is full
    if (voltageIndex == MEASURE_COUNT) {
      // Copy the voltage array to the storage array
      for (int i = 0; i < MEASURE_COUNT; i++) {
        storageArray[storageIndex][i] = voltageArray[i];
      }
      storageIndex++;

      // Check if the storage array is full
      if (storageIndex == STORAGE_SIZE) {
        // Remove the oldest voltage array and shift the others
        for (int i = 0; i < STORAGE_SIZE - 1; i++) {
          for (int j = 0; j < MEASURE_COUNT; j++) {
            storageArray[i][j] = storageArray[i + 1][j];
          }
        }
        storageIndex--;
      }

      // Reset the voltage index
      voltageIndex = 0;
    }

    // Update the last measure time
    lastMeasureTime = currentTime;
  }

  // Check if the battery is charging or discharging
  if (voltageArray[0] >= VOLTAGE_THRESHOLD) {
    // The battery is charging
    // Check if the previous state was discharging
    if (!isCharging) {
      // The previous state was discharging
      // Trigger the analytical function
      analyzeBattery();
    }
    // Set the charging flag to true
    isCharging = true;
  } else {
    // The battery is discharging
    // Set the charging flag to false
    isCharging = false;
  }
}

// The function to analyze the battery performance
void analyzeBattery() {
  // Declare some variables for the analysis
  float averageVoltage = 0.0; // The average voltage of the 5th hour of each voltage array
  float dischargeRate = 0.0; // The discharge rate of the battery
  int count = 0; // The number of voltage arrays to use for the analysis

  // Loop through the storage array from the oldest to the newest
  for (int i = storageIndex; i < STORAGE_SIZE; i++) {
    // Check if the voltage array has enough data
    if (voltageArray[MEASURE_COUNT - 1] > 0.0) {
      // Calculate the average voltage of the 5th hour
      averageVoltage += voltageArray[ANALYSIS_HOUR * 6];
      // Calculate the discharge rate of the battery
      dischargeRate += (voltageArray[0] - voltageArray[MEASURE_COUNT - 1]) / 12.0;
      // Increment the count
      count++;
    }
  }
  for (int i = 0; i < storageIndex; i++) {
    // Check if the voltage array has enough data
    if (voltageArray[MEASURE_COUNT - 1] > 0.0) {
      // Calculate the average voltage of the 5th hour
      averageVoltage += voltageArray[ANALYSIS_HOUR * 6];
      // Calculate the discharge rate of the battery
      dischargeRate += (voltageArray[0] - voltageArray[MEASURE_COUNT - 1]) / 12.0;
      // Increment the count
      count++;
    }
  }

  // Check if there is any data for the analysis
  if (count > 0) {
    // Calculate the average voltage and the discharge rate
    averageVoltage /= count;
    dischargeRate /= count;

    // Print the results to the serial monitor
    Serial.print("Average Voltage: ");
    Serial.println(averageVoltage);
    Serial.print("Discharge Rate: ");
    Serial.println(dischargeRate);

    // Plot the average voltage on the OLED display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Average Voltage:");
    display.drawLine(0, 10, 127, 10, WHITE);
    display.drawLine(0, 10, 0, 63, WHITE);
    display.setCursor(0, 54);
    display.print("0V");
    display.setCursor(100, 54);
    display.print("15V");
    display.setCursor(50, 10);
    display.print(averageVoltage);
    display.print("V");
    int x = map(averageVoltage, 0, 15, 0, 127);
    display.drawLine(x, 10, x, 63, WHITE);
    display.display();
  } else {
    // There is no data for the analysis
    // Print a message to the serial monitor
    Serial.println("No data for analysis");

    // Display a message on the OLED display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("No data for analysis");
    display.display();
  }
}

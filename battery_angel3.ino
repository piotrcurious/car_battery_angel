// Define the pins for the battery voltage sensor and the OLED display
#define BATTERY_PIN 34
#define OLED_SDA 21
#define OLED_SCL 22

// Define the voltage threshold for charging and discharging (in volts)
#define CHARGE_THRESHOLD 12.6
#define DISCHARGE_THRESHOLD 12.0

// Define the time interval for measuring voltage during discharging (in milliseconds)
#define MEASURE_INTERVAL 600000 // 10 minutes

// Define the size of the result array and the storage array
#define RESULT_SIZE 60 // 10 hours
#define STORAGE_SIZE 128 // 128 records

// Include the libraries for the OLED display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Create an object for the OLED display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire, -1);

// Declare the global variables for the result array, the storage array, and the state of the battery
float result[RESULT_SIZE]; // The array to store the voltage measurements during discharging
float storage[STORAGE_SIZE][RESULT_SIZE]; // The array to store the result arrays after each charging cycle
int state; // The state of the battery: 0 for charging, 1 for discharging, -1 for unknown

// Declare the function prototypes
void setup();
void loop();
void measureVoltage();
void copyResult();
void shiftStorage();
void plotVoltage();
float readVoltage();

// The setup function runs once when the board is powered on or reset
void setup() {
  // Initialize the serial monitor for debugging
  Serial.begin(115200);

  // Initialize the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  // Initialize the result array and the storage array with zeros
  for (int i = 0; i < RESULT_SIZE; i++) {
    result[i] = 0.0;
  }
  for (int i = 0; i < STORAGE_SIZE; i++) {
    for (int j = 0; j < RESULT_SIZE; j++) {
      storage[i][j] = 0.0;
    }
  }

  // Initialize the state of the battery as unknown
  state = -1;
}

// The loop function runs repeatedly after the setup function is completed
void loop() {
  // Read the battery voltage
  float voltage = readVoltage();

  // Check the state of the battery
  if (voltage >= CHARGE_THRESHOLD) {
    // The battery is charging
    if (state != 0) {
      // The state has changed from discharging to charging
      Serial.println("The battery is charging.");
      // Copy the result array to the storage array
      copyResult();
      // Trigger the analytical function
      plotVoltage();
      // Reset the result array
      for (int i = 0; i < RESULT_SIZE; i++) {
        result[i] = 0.0;
      }
      // Update the state
      state = 0;
    }
  } else if (voltage <= DISCHARGE_THRESHOLD) {
    // The battery is discharging
    if (state != 1) {
      // The state has changed from charging to discharging
      Serial.println("The battery is discharging.");
      // Update the state
      state = 1;
    }
    // Measure the voltage and store it in the result array
    measureVoltage();
  } else {
    // The battery is neither charging nor discharging
    if (state != -1) {
      // The state has changed from charging or discharging to unknown
      Serial.println("The battery is neither charging nor discharging.");
      // Update the state
      state = -1;
    }
  }

  // Delay for one second
  delay(1000);
}

// The function to measure the voltage and store it in the result array
void measureVoltage() {
  // Read the battery voltage
  float voltage = readVoltage();

  // Print the voltage to the serial monitor
  Serial.print("The battery voltage is ");
  Serial.print(voltage);
  Serial.println(" V.");

  // Check if the result array is full
  if (result[RESULT_SIZE - 1] != 0.0) {
    // The result array is full, only update the last field
    result[RESULT_SIZE - 1] = voltage;
  } else {
    // The result array is not full, find the first empty field and store the voltage there
    for (int i = 0; i < RESULT_SIZE; i++) {
      if (result[i] == 0.0) {
        result[i] = voltage;
        break;
      }
    }
  }

  // Delay for the measure interval
  delay(MEASURE_INTERVAL);
}

// The function to copy the result array to the storage array
void copyResult() {
  // Check if the storage array is full
  if (storage[STORAGE_SIZE - 1][RESULT_SIZE - 1] != 0.0) {
    // The storage array is full, shift the records by one and remove the oldest record
    shiftStorage();
  }

  // Find the first empty record in the storage array and copy the result array there
  for (int i = 0; i < STORAGE_SIZE; i++) {
    if (storage[i][0] == 0.0) {
      for (int j = 0; j < RESULT_SIZE; j++) {
        storage[i][j] = result[j];
      }
      break;
    }
  }
}

// The function to shift the storage array by one and remove the oldest record
void shiftStorage() {
  // Loop through the storage array from the second record to the last record
  for (int i = 1; i < STORAGE_SIZE; i++) {
    // Copy the current record to the previous record
    for (int j = 0; j < RESULT_SIZE; j++) {
      storage[i - 1][j] = storage[i][j];
    }
  }

  // Clear the last record
  for (int i = 0; i < RESULT_SIZE; i++) {
    storage[STORAGE_SIZE - 1][i] = 0.0;
  }
}

// The function to plot the voltage from the 5th hour of each record on the OLED display and print the last recorded discharge voltage value
void plotVoltage() {
  // Clear the display
  display.clearDisplay();

  // Set the text size and color
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Print the title
  display.setCursor(0, 0);
  display.println("Voltage from 5th hour of each record");

  // Print the last recorded discharge voltage value
  display.setCursor(0, 8);
  display.print("Last recorded voltage: ");
  display.print(result[RESULT_SIZE - 1]);
  display.println(" V");

  // Draw the x-axis and the y-axis
  display.drawLine(10, 56, 118, 56, SSD1306_WHITE); // x-axis
  display.drawLine(10, 16, 10, 56, SSD1306_WHITE); // y-axis

  // Draw the x-axis labels
  display.setCursor(10, 58);
  display.print("0");
  display.setCursor(54, 58);
  display.print("64");
  display.setCursor(108, 58);
  display.print("128");

  // Draw the y-axis labels
  display.setCursor(0, 16);
  display.print("15");
  display.setCursor(0, 36);
  display.print("12");
  display.setCursor(0, 56);
  display.print("9");

  // Loop through the storage array from the oldest record to the newest record
  for (int i = 0; i < STORAGE_SIZE; i++) {
    // Check if the record has enough data
    if (storage[i][RESULT_SIZE - 1] != 0.0) {
      // Get the voltage from the 5th hour of the record
      float voltage = storage[i][30];

      // Map the voltage to the pixel coordinates
      int x = map(i, 0, STORAGE_SIZE - 1, 11, 117);
      int y = map(voltage, 9, 15, 55, 17);

      // Draw a point on the display
      display.drawPixel(x, y, SSD1306_WHITE);
    }
  }

  // Update the display
  display.display();
}

// The function to read the battery voltage from the sensor pin
float readVoltage() {
  // Read the analog value from the sensor pin
  int value = analogRead(BATTERY_PIN);

  // Convert the analog value to voltage (assuming a 12-bit ADC and a 3.3V reference voltage)
  float voltage = value * 3.3 / 4095.0;

  // Return the voltage
  return voltage;
}

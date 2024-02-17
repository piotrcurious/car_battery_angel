// Include the libraries for SD card and RTC
#include <SD.h>
#include <RTClib.h>

// Define the pins for battery voltage, SD card and LED
#define BATTERY_PIN 35
#define SD_CS_PIN 5
#define LED_PIN 2

// Define the voltage threshold for charging and discharging
#define CHARGING_THRESHOLD 13.0
#define DISCHARGING_THRESHOLD 12.0

// Define the interval for measuring voltage when discharging
#define MEASURE_INTERVAL 600000 // 10 minutes in milliseconds

// Create an instance of RTC
RTC_DS3231 rtc;

// Create a variable to store the battery voltage
float battery_voltage = 0.0;

// Create a variable to store the battery status
// 0: unknown, 1: charging, 2: discharging
int battery_status = 0;

// Create an array to store the voltage measurements
float voltage_array[100];

// Create a variable to store the index of the array
int array_index = 0;

// Create a variable to store the file name
char file_name[20];

// Create a function to read the battery voltage
float read_battery_voltage() {
  // Read the analog value from the battery pin
  int analog_value = analogRead(BATTERY_PIN);

  // Convert the analog value to voltage
  // Assuming a voltage divider with 10K and 2.2K resistors
  float voltage = analog_value * (3.3 / 4095.0) * (10.0 + 2.2) / 2.2;

  // Return the voltage
  return voltage;
}

// Create a function to write the voltage array to a file
void write_voltage_array() {
  // Generate a file name with the current date and time
  DateTime now = rtc.now();
  sprintf(file_name, "%04d%02d%02d_%02d%02d%02d.txt", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  // Open the file for writing
  File file = SD.open(file_name, FILE_WRITE);

  // Check if the file is opened
  if (file) {
    // Write the voltage array to the file
    for (int i = 0; i < array_index; i++) {
      file.println(voltage_array[i]);
    }

    // Close the file
    file.close();

    // Reset the array index
    array_index = 0;
  }
}

// Create a function to blink the LED
void blink_led(int times, int duration) {
  // Loop for the given number of times
  for (int i = 0; i < times; i++) {
    // Turn on the LED
    digitalWrite(LED_PIN, HIGH);

    // Wait for half of the duration
    delay(duration / 2);

    // Turn off the LED
    digitalWrite(LED_PIN, LOW);

    // Wait for half of the duration
    delay(duration / 2);
  }
}

// Setup function
void setup() {
  // Initialize the serial monitor
  Serial.begin(9600);

  // Initialize the LED pin as output
  pinMode(LED_PIN, OUTPUT);

  // Initialize the SD card
  if (!SD.begin(SD_CS_PIN)) {
    // If the SD card initialization fails, print an error message and stop
    Serial.println("SD card initialization failed!");
    while (1);
  }

  // Initialize the RTC
  if (!rtc.begin()) {
    // If the RTC initialization fails, print an error message and stop
    Serial.println("RTC initialization failed!");
    while (1);
  }

  // Check if the RTC is running
  if (!rtc.isrunning()) {
    // If the RTC is not running, print a warning message and set the current time
    Serial.println("RTC is not running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

// Loop function
void loop() {
  // Read the battery voltage
  battery_voltage = read_battery_voltage();

  // Print the battery voltage
  Serial.print("Battery voltage: ");
  Serial.println(battery_voltage);

  // Check the battery status
  if (battery_voltage >= CHARGING_THRESHOLD) {
    // If the battery voltage is above the charging threshold, the battery is charging
    if (battery_status != 1) {
      // If the previous battery status was not charging, print a message and blink the LED twice
      Serial.println("Battery is charging!");
      blink_led(2, 500);

      // If the previous battery status was discharging, write the voltage array to a file
      if (battery_status == 2) {
        write_voltage_array();
      }
    }

    // Set the battery status to charging
    battery_status = 1;
  }
  else if (battery_voltage <= DISCHARGING_THRESHOLD) {
    // If the battery voltage is below the discharging threshold, the battery is discharging
    if (battery_status != 2) {
      // If the previous battery status was not discharging, print a message and blink the LED once
      Serial.println("Battery is discharging!");
      blink_led(1, 500);
    }

    // Set the battery status to discharging
    battery_status = 2;

    // Measure the voltage every 10 minutes and store it in the array
    if (millis() % MEASURE_INTERVAL == 0) {
      // Check if the array is full
      if (array_index < 100) {
        // If the array is not full, store the voltage in the array
        voltage_array[array_index] = battery_voltage;

        // Increment the array index
        array_index++;
      }
      else {
        // If the array is full, print a warning message and stop
        Serial.println("Voltage array is full!");
        while (1);
      }
    }
  }
  else {
    // If the battery voltage is between the thresholds, the battery status is unknown
    if (battery_status != 0) {
      // If the previous battery status was not unknown, print a message and blink the LED three times
      Serial.println("Battery status is unknown!");
      blink_led(3, 500);
    }

    // Set the battery status to unknown
    battery_status = 0;
  }
}

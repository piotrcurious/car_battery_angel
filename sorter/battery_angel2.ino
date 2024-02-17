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

// Define the minimum number of lines in a file
#define MIN_LINES 30

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

// Create a function to compare two file names
// Return 1 if the first file name is newer than the second file name
// Return -1 if the first file name is older than the second file name
// Return 0 if the file names are the same or invalid
int compare_file_names(char* file_name_1, char* file_name_2) {
  // Check if the file names are valid
  if (strlen(file_name_1) != 19 || strlen(file_name_2) != 19) {
    // If the file names are invalid, return 0
    return 0;
  }

  // Extract the year, month, day, hour, minute and second from the file names
  int year_1 = (file_name_1[0] - '0') * 1000 + (file_name_1[1] - '0') * 100 + (file_name_1[2] - '0') * 10 + (file_name_1[3] - '0');
  int month_1 = (file_name_1[4] - '0') * 10 + (file_name_1[5] - '0');
  int day_1 = (file_name_1[6] - '0') * 10 + (file_name_1[7] - '0');
  int hour_1 = (file_name_1[9] - '0') * 10 + (file_name_1[10] - '0');
  int minute_1 = (file_name_1[11] - '0') * 10 + (file_name_1[12] - '0');
  int second_1 = (file_name_1[13] - '0') * 10 + (file_name_1[14] - '0');

  int year_2 = (file_name_2[0] - '0') * 1000 + (file_name_2[1] - '0') * 100 + (file_name_2[2] - '0') * 10 + (file_name_2[3] - '0');
  int month_2 = (file_name_2[4] - '0') * 10 + (file_name_2[5] - '0');
  int day_2 = (file_name_2[6] - '0') * 10 + (file_name_2[7] - '0');
  int hour_2 = (file_name_2[9] - '0') * 10 + (file_name_2[10] - '0');
  int minute_2 = (file_name_2[11] - '0') * 10 + (file_name_2[12] - '0');
  int second_2 = (file_name_2[13] - '0') * 10 + (file_name_2[14] - '0');

  // Compare the year, month, day, hour, minute and second
  if (year_1 > year_2) {
    return 1;
  }
  else if (year_1 < year_2) {
    return -1;
  }
  else {
    if (month_1 > month_2) {
      return 1;
    }
    else if (month_1 < month_2) {
      return -1;
    }
    else {
      if (day_1 > day_2) {
        return 1;
      }
      else if (day_1 < day_2) {
        return -1;
      }
      else {
        if (hour_1 > hour_2) {
          return 1;
        }
        else if (hour_1 < hour_2) {
          return -1;
        }
        else {
          if (minute_1 > minute_2) {
            return 1;
          }
          else if (minute_1 < minute_2) {
            return -1;
          }
          else {
            if (second_1 > second_2) {
              return 1;
            }
            else if (second_1 < second_2) {
              return -1;
            }
            else {
              return 0;
            }
          }
        }
      }
    }
  }
}

// Create a function to sort an array of file names in chronological order
void sort_file_names(char file_names[][20], int size) {
  // Use bubble sort algorithm
  for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - i - 1; j++) {
      // Compare the file names at index j and j+1
      int result = compare_file_names(file_names[j], file_names[j+1]);

      // If the file name at index j is newer than the file name at index j+1, swap them
      if (result == 1) {
        char temp[20];
        strcpy(temp, file_names[j]);
        strcpy(file_names[j], file_names[j+1]);
        strcpy(file_names[j+1], temp);
      }
    }
  }
}

// Create a function to analyze the files stored on SD card
void analyze_files() {
  // Create an array to store the sample voltages
  float sample_voltages[100];

  // Create a variable to store the index of the array
  int sample_index = 0;

  // Create an array to store the file names
  char file_names[100][20];

  // Create a variable to store the number of files
  int file_count = 0;

  // Open the root directory of the SD card
  File root = SD.open("/");

  // Loop through all the files in the root directory
  while (true) {
    // Read the next file
    File file = root.openNextFile();

    // Check if the file exists
    if (!file) {
      // If the file does not exist, break the loop
      break;
    }

    // Check if the file is a text file
    if (file.name()[strlen(file.name()) - 4] == '.') {
      // If the file is a text file, copy its name to the file
      // If the file is a text file, copy its name to the file names array
      strcpy(file_names[file_count], file.name());

      // Increment the file count
      file_count++;

      // Close the file
      file.close();
    }
  }

  // Close the root directory
  root.close();

  // Sort the file names in chronological order
  sort_file_names(file_names, file_count);

  // Loop through the sorted file names
  for (int i = 0; i < file_count; i++) {
    // Print the file name
    Serial.print("Analyzing file: ");
    Serial.println(file_names[i]);

    // Open the file for reading
    File file = SD.open(file_names[i], FILE_READ);

    // Check if the file is opened
    if (file) {
      // Create a variable to store the line number
      int line_number = 0;

      // Create a variable to store the voltage value
      float voltage_value = 0.0;

      // Loop through all the lines in the file
      while (file.available()) {
        // Read the next line
        String line = file.readStringUntil('\n');

        // Convert the line to a float
        voltage_value = line.toFloat();

        // Increment the line number
        line_number++;

        // Check if the line number is 30
        if (line_number == 30) {
          // If the line number is 30, this means 5 hours have passed since the beginning of the array
          // Store the voltage value in the sample voltages array
          sample_voltages[sample_index] = voltage_value;

          // Increment the sample index
          sample_index++;

          // Break the loop
          break;
        }
      }

      // Check if the file has enough data
      if (line_number < MIN_LINES) {
        // If the file has less than the minimum number of lines, print a warning message and skip the file
        Serial.println("File has not enough data!");
        continue;
      }

      // Close the file
      file.close();
    }
  }

  // Print the sample voltages array
  Serial.println("Sample voltages:");
  for (int i = 0; i < sample_index; i++) {
    Serial.println(sample_voltages[i]);
  }
}

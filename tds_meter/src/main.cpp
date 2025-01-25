#include <Arduino.h>

#define TdsSensorPin 27
#define VREF 3.3              // Analog reference voltage (Volt) of the ADC
#define SCOUNT  30            // Number of sample points

int analogBuffer[SCOUNT];     // Store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;       // Current temperature for compensation

// Median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen) {
    int bTab[iFilterLen];
    for (byte i = 0; i < iFilterLen; i++)
        bTab[i] = bArray[i];

    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++) {
        for (i = 0; i < iFilterLen - j - 1; i++) {
            if (bTab[i] > bTab[i + 1]) {
                bTemp = bTab[i];
                bTab[i] = bTab[i + 1];
                bTab[i + 1] = bTemp;
            }
        }
    }
    if ((iFilterLen & 1) > 0) {
        bTemp = bTab[(iFilterLen - 1) / 2];
    } else {
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    }
    return bTemp;
}

float TDSreading(float temperature) {
    static unsigned long analogSampleTimepoint = millis();
    static unsigned long printTimepoint = millis();

    if (millis() - analogSampleTimepoint > 40U) { // Every 40 milliseconds
        analogSampleTimepoint = millis();
        analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); // Read analog value
        analogBufferIndex++;
        if (analogBufferIndex == SCOUNT) {
            analogBufferIndex = 0;
        }
    }

    if (millis() - printTimepoint > 800U) { // Every 800 milliseconds
        printTimepoint = millis();
        for (int copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
            analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
        }

        // Median filtering and voltage calculation
        float averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;

        // Temperature compensation
        float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
        float compensationVoltage = averageVoltage / compensationCoefficient;

        // Convert voltage to TDS value
        float tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
                          - 255.86 * compensationVoltage * compensationVoltage
                          + 857.39 * compensationVoltage) * 0.5;

        return tdsValue; // Return the calculated TDS value
    }

    return -1; // Return -1 if no new TDS value is ready
}

void setup() {
    Serial.begin(115200); // Initialize serial communication at 115200 baud rate
}

void loop() {
    
    float tds = TDSreading(temperature);
    if (tds >= 0) { // Check if a valid TDS value is returned
        Serial.print("TDS Value: ");
        Serial.print(tds, 0);
        Serial.println(" ppm");
    }

}

/* code for synthsizer project
this project will be a mini synthesier, with 13 keys. (8 white 5 black)
it will run using a rapsberry Pi zero and hopefully work independently of an external device.
it will also have dials for volume, octave, and other features that modify its sound.
it will be able to freely switch between headphone and usage of built in speakers
*/
#include <Wire.h>
#include <Adafruit_ADS1015.h>

const int KEY_C       = 4;   // Note C
const int KEY_CSHARP  = 17;  // Note C#
const int KEY_D       = 27;  // Note D
const int KEY_DSHARP  = 22;  // Note D#
const int KEY_E       = 23;  // Note E
const int KEY_F       = 24;  // Note F
const int KEY_FSHARP  = 25;  // Note F#
const int KEY_G       = 5;   // Note G
const int KEY_GSHARP  = 6;   // Note G#
const int KEY_A       = 12;  // Note A
const int KEY_ASHARP  = 13;  // Note A#
const int KEY_B       = 16;  // Note B
const int KEY_HIGH_C  = 26;  // Note High C

const int PIN_SW_WAVE  = 9;   // Wave
const int PIN_SW_GLIDE = 11;  // Glide
const int PIN_SW_ARP   = 8;   // Arp
const int OCT_1 = 14; 
const int OCT_2 = 15;
const int OCT_3 = 0;
const int OCT_4 = 1;

const float NOTE_FREQS[13] = {
    261.63, // C
    277.18, // C#
    293.66, // D
    311.13, // D#
    329.63, // E
    349.23, // F
    369.99, // F#
    392.00, // G
    415.30, // G#
    440.00, // A
    466.16, // A#
    493.88, // B
    523.25  // High C
};

const int KEY_PINS[13] = {KEY_C, KEY_CSHARP, KEY_D, KEY_DSHARP, KEY_E, KEY_F, KEY_FSHARP, KEY_G, KEY_GSHARP, KEY_A, KEY_ASHARP, KEY_B, KEY_HIGH_C};
const int OCT_PINS[4]  = {OCT_1, OCT_2, OCT_3, OCT_4};

Adafruit_ADS1115 ads;

float currentFreq = 0.0;
float targetFreq = 0.0;
float phase = 0.0;
unsigned long lastSampleTime = 0;

int waveformType = 0;   
bool glideEnabled = false;
float glideSpeed = 0.08;  
float filterCutoff = 0.5;
float filterLastVal = 0.0;

bool lastButtonState = HIGH; 

void setup() {
    Wire.begin();
    
    for (int i = 0; i < 13; i++) {
        pinMode(KEY_PINS[i], INPUT_PULLUP);
    }
    
    pinMode(PIN_SW_WAVE, INPUT_PULLUP);
    pinMode(PIN_SW_GLIDE, INPUT_PULLUP);
    pinMode(PIN_SW_ARP, INPUT_PULLUP);
    
    for (int i = 0; i < 4; i++) {
        pinMode(OCT_PINS[i], INPUT_PULLUP);
    }

    ads.begin();
    ads.setGain(GAIN_ONE); 
    
    lastSampleTime = micros();
}

void loop() {

    float octaveMultiplier = 1.0;
    if (digitalRead(OCT_1) == LOW) octaveMultiplier = 0.25; /
    else if (digitalRead(OCT_2) == LOW) octaveMultiplier = 0.5;  
    else if (digitalRead(OCT_3) == LOW) octaveMultiplier = 1.0;  
    else if (digitalRead(OCT_4) == LOW) octaveMultiplier = 2.0; 

    uint16_t adc0 = ads.readADC_SingleEnded(0);
    filterCutoff = (float)adc0 / 32768.0; 
    if (filterCutoff < 0.01) filterCutoff = 0.01; 

    glideEnabled = (digitalRead(PIN_SW_GLIDE) == LOW);

    bool currentButtonState = digitalRead(PIN_SW_WAVE);
    if (currentButtonState == LOW && lastButtonState == HIGH) {
        waveformType = (waveformType + 1) % 3; 
        delay(150); 
    }
    lastButtonState = currentButtonState;

    float triggeredFreq = 0.0;
    bool notePressedThisCycle = false;

    for (int i = 0; i < 13; i++) {
        if (digitalRead(KEY_PINS[i]) == LOW) {
            triggeredFreq = NOTE_FREQS[i] * octaveMultiplier;
            notePressedThisCycle = true;
            break; 
        }
    }

    if (notePressedThisCycle) {
        targetFreq = triggeredFreq;
    } else {
        if (!glideEnabled) targetFreq = 0.0;
    }

    if (glideEnabled && notePressedThisCycle) {
        currentFreq += (targetFreq - currentFreq) * glideSpeed;
    } else {
        currentFreq = targetFreq;
    }

    if (currentFreq > 0.0 && (notePressedThisCycle || (glideEnabled && currentFreq > 1.0))) {
        unsigned long currentMicros = micros();
        float deltaTime = (float)(currentMicros - lastSampleTime) / 1000000.0;
        lastSampleTime = currentMicros;

        phase += 2.0 * PI * currentFreq * deltaTime;
        if (phase >= 2.0 * PI) phase -= 2.0 * PI;

        float rawSample = 0.0;

        if (waveformType == 0) {      
            rawSample = 1.0 - (2.0 * (phase / (2.0 * PI)));
        } else if (waveformType == 1) {
            rawSample = (phase < PI) ? 1.0 : -1.0;
        } else {                 
            rawSample = 2.0 * abs(2.0 * (phase / (2.0 * PI)) - 1.0) - 1.0;
        }

        float filteredSample = filterLastVal + filterCutoff * (rawSample - filterLastVal);
        filterLastVal = filteredSample;

    } else {
        lastSampleTime = micros(); 
    }

    delayMicroseconds(50);
}


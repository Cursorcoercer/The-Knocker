#include <vector>

using namespace std;

// initialize all time varibles
unsigned long currentTime = 0;
unsigned long micCheck = 0;
unsigned long soundUpdate = 0;
unsigned long breathe = 0;

// now the state variables
bool locked = false;
bool open = true;
const int bits = 120;
vector<bool> sounds (bits, 0);
int prevsound = 0;
int sound = 0;
bool breath = false;
int mic = A3;
Servo myservo;
int servo = A4;
int door = D1;
int override = D0;
int ledpin = D2;
int code2pin = D4;
int code3pin = D5;
int modepin = A2;

vector<vector<int>> makeCode(vector<int> simpleCode) {
    // tool for making a basic kind of code, alternating stretches with no gaps
    // up to you to make sure sum of simpleCode is less than bits
    vector<vector<int>> result;
    int place = 0;
    int alt = 1;
    for (int i=simpleCode.size()-1; i>=0; i--) {
        result.push_back({alt,place,place+simpleCode[i]});
        place += simpleCode[i];
        alt = 1-alt;
    }
    return result;
}

// set the codes
// the best way to find the right numbers is to practice the knock a bunch of times 
// then find the numbers that match the data 
//vector<vector<int>> code1 = makeCode({3,4,2,1,2,1,2,4,2,9,4,2,4});
vector<vector<int>> code1 = makeCode({5,2,2,1,2,1,2,2,4,5,6,1,5});
vector<vector<int>> code2 = makeCode({3,19,3});
vector<vector<int>> code3 = makeCode({3,8,3,8,3});

bool match(vector<bool> sounds, vector<vector<int>> code) {
    // find the max value in the code vector, so we know how many times to try it
    int maxval = 0;
    for (int i=0; i<code.size(); i++) {
        maxval = max(maxval, code[i][1]);
        maxval = max(maxval, code[i][2]);
    }
    int tries = sounds.size()-maxval;
    // see if the sound vector fits the code
    bool fits = false;
    for (int offset=0; offset<tries; offset++){
        bool good = true;
        for (int i=0; i<code.size(); i++) {
            bool brk = false;
            if (code[i][0] == 0) {
                // confirm all 0's in sound range
                for (int j=code[i][1]+offset; j<code[i][2]+offset; j++) {
                    if (sounds[j]) {
                        brk = true;
                        break;
                    }
                }
            } else {
                // confirm at least one 1 in sound range
                brk = true;
                for (int j=code[i][1]+offset; j<code[i][2]+offset; j++) {
                    if (sounds[j]) {
                        brk = false;
                        break;
                    }
                }
            }
            if (brk) {
                // code element did not match
                good = false;
                break;
            }
        }
        if (good) {
            // all code elements mathced, we're done here
            fits = true;
            break;
        }
    }
    return fits;
}

void unlock() {
    myservo.write(0);
}

void lock() {
    myservo.write(110);
}

void setup() {
    
    // mostly just set all the pins to the right modes
    Serial.begin(9600);
    pinMode(D7, OUTPUT);
    pinMode(mic, INPUT);
    myservo.attach(servo);
    pinMode(override, INPUT_PULLDOWN);
    pinMode(door, INPUT_PULLDOWN);
    pinMode(ledpin, INPUT_PULLDOWN);
    pinMode(emailpin, INPUT_PULLDOWN);
    pinMode(code2pin, INPUT_PULLDOWN);
    pinMode(code3pin, INPUT_PULLDOWN);
    pinMode(modepin, INPUT_PULLDOWN);
}

void loop() {
    
    // get current time
    currentTime = millis();
    
    // read mic value
    if ((currentTime-micCheck) >= 2) {
        micCheck = currentTime;
        sound = max(sound, analogRead(mic));
    }
    
    // update the sound vector
    if ((currentTime-soundUpdate) >= 50) {
        soundUpdate = currentTime;
        sounds.insert(sounds.begin(), ((prevsound+50)<sound));
        sounds.pop_back();
        if (digitalRead(modepin)) {
            Serial.println(sound);
        } else {
            Serial.print(sounds[0]);
        }
        prevsound = sound;
        sound = 0;
    }
    
    // show that the blood is pumping
    if ((currentTime-breathe) >= 500) {
        breathe = currentTime;
        breath = !breath;
        digitalWrite(D7, (breath && digitalRead(ledpin)));
        if (breath && !digitalRead(modepin)) {
            Serial.println();
        }
    }
    
    // check if door is open or closed
    if (!digitalRead(door)) {
        // door closed
        locked = true;
        if (open) {
            // debounce door sensor
            delay(50);
        }
        open = false;
    } else {
        // door open
        locked = false;
        if (not open) {
            // door was closed, publish the door opened event
            Particle.publish("door_open");
            // debouncing important here so that we don't publish a bunch of events
            delay(50);
        }
        open = true;
    }
    
    // check if the override button is pressed
    if (digitalRead(override)) {
        locked = false;
    }
    
    // if it is locked, check if a secret knock is done 
    if (locked) {
        if (match(sounds, code1)) {
            locked = false;
        }
    }
    
    // check the other codes, if their mode is active
    if (locked && digitalRead(code2pin)) {
        if (match(sounds, code2)) {
            locked = false;
        }
    }
    
    if (locked && digitalRead(code3pin)) {
        if (match(sounds, code3)) {
            locked = false;
        }
    }
    
    // execute the locking
    if (locked) {
        lock();
    } else {
        unlock();
    }
}

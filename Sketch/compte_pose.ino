
// Défintion de la broche reliée au relai.
static const byte RELAY_PIN = A0;

// Définition des broches de commande du décodeur BCD vers décimal (5411).
static const byte A = 7;
static const byte B = 6;
static const byte C = 5;
static const byte D = 4;

// Définition des broche de commande d'affichage des digits
static const byte MIN_TEN_PIN = 8;
static const byte MIN_ONE_PIN = 9;
static const byte SEC_TEN_PIN = 10;
static const byte SEC_ONE_PIN = 11;

// Définition de la broche permettant sélectionner les minutes ou les secondes ou de revenir à l'état normal
static const byte CHANGE_TIME_PIN = 12;
 
// Définition des broches de l'encodeur
static const byte ENCODER_PIN_A = 2; // Interruption
static const byte ENCODER_PIN_B = 13; 

static const byte MIN_TEN = 0;
static const byte MIN_ONE = 1;
static const byte SEC_TEN = 2;
static const byte SEC_ONE = 3;

// Le bouton poussoir permettant de lancer le compte-à-rebourd est relié à une broche d'interruption.
static const int START_COUNTDOWN_PIN = 3;

// Constante de temps de rebond 
static unsigned long debounce = 200;
const static byte ENCODER_DEBOUNCE = 100;
static byte buttonDebounceTime = 100;

// Dernier instant considéré de changement de valeur du bouton poussoir de compte-à-rebourd pour le temps de rebond
unsigned long previousChange = 0;

static const int DEFAULT_START_TIME = 180; // 3 minutes


void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(D, OUTPUT);

  pinMode(MIN_TEN_PIN, OUTPUT);
  pinMode(MIN_ONE_PIN, OUTPUT);
  pinMode(SEC_TEN_PIN, OUTPUT);
  pinMode(SEC_ONE_PIN, OUTPUT);

  pinMode(CHANGE_TIME_PIN, INPUT_PULLUP);
  
  pinMode(START_COUNTDOWN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(START_COUNTDOWN_PIN), countDownStart, FALLING);

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), changeTimeAlt, FALLING);
  //attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), changeTime, CHANGE);
  
  digitalWrite(A, LOW);
  digitalWrite(B, LOW);
  digitalWrite(C, LOW);
  digitalWrite(D, LOW);

  digitalWrite(MIN_TEN_PIN, LOW);
  digitalWrite(MIN_ONE_PIN, LOW);
  digitalWrite(SEC_TEN_PIN, LOW);
  digitalWrite(SEC_ONE_PIN, LOW);
  
  
}

volatile byte minutes = 0;
volatile byte seconds = 0;

byte savedMinutes = 0;
byte savedSeconds = 0;
//volatile int startTime = DEFAULT_START_TIME;
bool isCountdownStarted = false;

int count = 0;
int digit = MIN_TEN;
unsigned long previous = 0;
unsigned long previousCountDown = 0;

unsigned long previousEncoderMillis = 0;

unsigned long previousPushTime = 0;
static const int PUSH_DEBOUNCE = 500;

static const byte MINUTES_SELECTED = 1;
static const byte SECONDS_SELECTED = 2;
static const byte READY_FOR_COUNTDOWN = 4;
static const int BLINK_TIME = 200; // 500ms
unsigned long previousBlinkTime = 0;

byte pSeqA = 0;
volatile byte seqA = 0;
volatile byte seqB = 0;

byte timeSelection = READY_FOR_COUNTDOWN;

unsigned int countSeq = 0;
volatile unsigned int countChangeTimeCalls = 0;
void loop() {
  byte changeTimeState = digitalRead(CHANGE_TIME_PIN);
  
  if(changeTimeState == 0 && millis() - previousPushTime > PUSH_DEBOUNCE) {
    timeSelection <<= 1;
    if(timeSelection > READY_FOR_COUNTDOWN) timeSelection = MINUTES_SELECTED;   
    if(timeSelection != READY_FOR_COUNTDOWN) previousBlinkTime = millis();
    previousPushTime = millis();
    Serial.println(timeSelection); 
  }
  
  if( (millis() - previous) > 3) {
    // SEC_ONE case
    int actualDigitPin = SEC_ONE_PIN;
    int previousDigitPin = SEC_TEN_PIN;
    int digitValue = 15; // ne peut être affiché

    // Rappel : si le résultat de l'opération est un nombre décimal, arduino conserve uniquement
    //          la partie entière sans approximation.
//    int displayedTime = startTime;

    int minutesDisplayed = minutes;//displayedTime / 60;
    int secondsDisplayed = seconds;//displayedTime - minutesDisplayed * 60;
    
    int minTenValue = minutesDisplayed / 10;
    int minOneValue = minutesDisplayed - minTenValue * 10;
    
    int secTenValue = secondsDisplayed / 10;
    int secOneValue = secondsDisplayed - secTenValue * 10;


    //int secTenValue = (displayedTime - (minTenValue*10 + minOneValue) * 60) / 10;
    //int secOneValue = displayedTime - (minTenValue*10 + minOneValue) * 60 - secTenValue * 10;

    
    
    switch(digit) {
      case MIN_TEN:
        actualDigitPin = MIN_TEN_PIN;
        previousDigitPin = SEC_ONE_PIN;
        digitValue = minTenValue;
        break;
      case MIN_ONE:
        actualDigitPin = MIN_ONE_PIN;
        previousDigitPin = MIN_TEN_PIN;
        digitValue = minOneValue;
        break;
      case SEC_TEN:
        actualDigitPin = SEC_TEN_PIN;
        previousDigitPin = MIN_ONE_PIN;
        digitValue = secTenValue;
        break;
      default: // SEC_ONE
        digitValue = secOneValue;
    }

    digitalWrite(previousDigitPin, LOW); 
    setDigitValue(digitValue);

    if(     (timeSelection == MINUTES_SELECTED && (actualDigitPin == MIN_TEN_PIN || actualDigitPin == MIN_ONE_PIN))
         || (timeSelection == SECONDS_SELECTED && (actualDigitPin == SEC_TEN_PIN || actualDigitPin == SEC_ONE_PIN))
      ) {
      
      unsigned int currentBlinkTime = millis() - previousBlinkTime;
      if(currentBlinkTime < BLINK_TIME) {
        digitalWrite(actualDigitPin, HIGH);     
      }
      if(currentBlinkTime > 2*BLINK_TIME) {
        previousBlinkTime = millis();
      }
      
    } else {  
      digitalWrite(actualDigitPin, HIGH);     
    }
  
    digit++;
    if(digit > 3) digit = MIN_TEN;
    previous = millis();
  }

/*
  if(seqA != pSeqA) {
    Serial.print("Print seq: ");Serial.println(countSeq);
    Serial.print("changeTime calls: ");Serial.println(countChangeTimeCalls);
    Serial.print("A");Serial.println(seqA, BIN);
    Serial.print("B");Serial.println(seqB, BIN); 
    Serial.flush();
    pSeqA = seqA;
    countSeq++;
  }
*/  

  
  // previousCountDown est initialisé dans ::countDownStart() pour 
  // avoir un comptage du temps qui se fini au moment ou le temps 
  // passe à 00:00
  if(isCountdownStarted && (millis() - previousCountDown > 1000)) {
    seconds--;
    if(minutes == 0 && seconds == 0) {
      digitalWrite(RELAY_PIN, LOW);
      seconds = 0;
      isCountdownStarted = false;
      minutes = savedMinutes;
      seconds = savedSeconds;
      
    } else if(seconds < 0) {
      minutes--;
      seconds = 59;
    }
    
    previousCountDown = millis();
  }

}

unsigned long previousStartCountdownBounce = 0;

void countDownStart() {
  unsigned long currentTime = millis();
  if(!isCountdownStarted  
      && READY_FOR_COUNTDOWN == timeSelection 
      && (currentTime - previousStartCountdownBounce) > buttonDebounceTime
      && (minutes > 0 || seconds >0)) {
     isCountdownStarted = true;
     digitalWrite(RELAY_PIN, HIGH);

     savedMinutes = minutes;
     savedSeconds = seconds;
     
     previousStartCountdownBounce = currentTime;
     previousCountDown = currentTime;
  }
}

/*
void changeTime() {
  //Ne pas utiliser de serial ici ça ralenti le système
  byte a = digitalRead(ENCODER_PIN_A);
  byte b = digitalRead(ENCODER_PIN_B);
  countChangeTimeCalls++;
  seqA = ((seqA << 1) | a) & 0b00001111;
  seqB = ((seqB << 1) | b) & 0b00001111;
 
 if (seqA == 0b00001001 && seqB == 0b00000011){
    if(MINUTES_SELECTED == timeSelection) {
      startTime -= 60;
    } else if(SECONDS_SELECTED == timeSelection){
      startTime--; 
    }
    if(startTime < 0) startTime =0;
  } else if (seqA == 0b00000011 && seqB == 0b00001001 ){
    if(MINUTES_SELECTED == timeSelection) {
      startTime += 60;
    } else if(SECONDS_SELECTED == timeSelection){
      startTime++; 
    }
  } 
}
*/
void changeTimeAlt() {
  //Ne pas utiliser de serial ici ça ralenti le système
  byte a = digitalRead(ENCODER_PIN_A);
  byte b = digitalRead(ENCODER_PIN_B);
  countChangeTimeCalls++;

 if (a == 0 && b == 0){
    if(MINUTES_SELECTED == timeSelection) {
      minutes--;
      if(minutes < 0) minutes = 99;
    } else if(SECONDS_SELECTED == timeSelection){
      seconds--; 
      if(seconds < 0) seconds = 59;
    }
  } else if (a == 0 && b == 1 ){
    if(MINUTES_SELECTED == timeSelection) {
      minutes++;
      if(minutes > 99) minutes = 0;
    } else if(SECONDS_SELECTED == timeSelection){
      seconds++; 
      if(seconds > 59) seconds = 0;
    }
  }
}

void setDigitValue(int valueToDisplay) {
    int a0 = bitRead(valueToDisplay, 0);
    int a1 = bitRead(valueToDisplay, 1);
    int a2 = bitRead(valueToDisplay, 2);
    int a3 = bitRead(valueToDisplay, 3);

    digitalWrite(A, a0);
    digitalWrite(B, a1);
    digitalWrite(C, a2);
    digitalWrite(D, a3);
}

void plusOne() {
  if ((millis() - previousChange) > debounce) {
    count++;
    if (count > 9) count = 0;
    previousChange = millis();
  }
}


const int ledPin = 1; // led connected to digital pin 13
//const int ledPin = 13; // led connected to digital pin 13
const int knockSensor = 1; // the piezo is connected to analog pin 0
//const int knockSensor = A1; // the piezo is connected to analog pin 0
const int lockPin = 3;         // The pin that activates the solenoid lock.
//const int lockPin = 10;         // The pin that activates the solenoid lock.
const int knockThreshold = 3; // threshold value to decide when the detected sound is a knock or not
const int maximumKnocks = 20; // Maximum number of knocks to listen for.
const int maximumKnockDelay = 2000; // Longest time to wait for a knock before we assume that it's finished. (milliseconds)
const int lockOperateTime = 2500;  // Milliseconds that we operate the lock solenoid latch before releasing it.
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock. Typical values 10-30
const int averageRejectValue = 15; // If the average timing of all the knocks is off by this percent we don't unlock. Typical values 5-20

int knockReadings[maximumKnocks];    // When someone knocks this array fills with the delays between knocks.
byte validKnockPattern[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."

void setup()
{
  pinMode(ledPin, OUTPUT); // declare the ledPin as as OUTPUT
  pinMode(lockPin, OUTPUT); // declare the ledPin as as OUTPUT
  resetKnockReadings();
}

void loop()
{
  // Lock
  digitalWrite(lockPin, LOW);
  
  if (heardKnockPattern()) {
    if (isValidateKnockPattern()) {
      openLock(lockOperateTime);
      //replayKnocks();
    } else {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    resetKnockReadings();
  }
  /*
  //we've got our knock recorded, lets see if it's valid
  if (validateKnock() == true){
    doorUnlock(lockOperateTime); 
  } else {
    // knock is invalid. Blink the LED as a warning to others.
    for (i=0; i < 4; i++){          
      digitalWrite(ledPin, HIGH);
      delay(50);
      digitalWrite(ledPin, LOW);
      delay(50);
    }
  }
  */
}

bool heardKnockPattern()
{
  long lastKnockTime = knock();                 // Reference for when this knock started.
  long currentKnockTime = 0;
  int lastKnockNumber = 0;                 // Position counter for the array.
  
  if (lastKnockTime <= 0) {
    return false;
  }
  
  do {
    currentKnockTime = knock();

    if (currentKnockTime > 0) {
      knockReadings[lastKnockNumber] = currentKnockTime - lastKnockTime;
      lastKnockNumber++;                             
      lastKnockTime = currentKnockTime;
    }
    
    // Stop listening if there are too many knocks or there is too much time between knocks.
  } while ((millis() - lastKnockTime < maximumKnockDelay) && (lastKnockNumber < maximumKnocks));

  return true;
}

long knock()
{
  long knockStart = 0;
  long sensorReading = analogRead(knockSensor);
  
  if (sensorReading >= knockThreshold) {
    knockStart = millis();

    int iterations = (150 / 20);      // Wait for the peak to dissipate before listening to next one.
    for (int i = 0; i < iterations; i++) {
      delay(10);
      analogRead(knockSensor);                  // This is done in an attempt to defuse the analog sensor's capacitor that will give false readings on high impedance sensors.
      delay(10);
    }
  }

  return knockStart;
}

bool isValidateKnockPattern()
{
  int i = 0;
 
  int testKnockCount = 0;
  int validKnockCount = 0;
  int maxKnockInterval = 0;
  
  for (i = 0; i < maximumKnocks; i++) {
    if (knockReadings[i] > 0) {
      testKnockCount++;
    }
    
    if (validKnockPattern[i] > 0) {
      validKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval) {   // Collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }
  
  if (testKnockCount != validKnockCount) {  // Easiest check first. If the number of knocks is wrong, don't unlock.
    return false;
  }
  
  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (ie: if you do the same pattern slow or fast it should still open the door.)
      This makes it less picky, which while making it less secure can also make it
      less of a pain to use if you're tempo is a little slow or fast. 
  */
  int totaltimeDifferences = 0;
  int timeDiff = 0;
  for (i = 0; i < maximumKnocks; i++) {    // Normalize the times
    knockReadings[i] = map(knockReadings[i], 0, maxKnockInterval, 0, 100);
    timeDiff = abs(knockReadings[i] - validKnockPattern[i]);
    if (timeDiff > rejectValue) {        // Individual value too far out of whack. No access for this knock!
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences / validKnockCount > averageRejectValue) {
    return false; 
  }

  return true;
}

void openLock(int delayTime){
  digitalWrite(ledPin, HIGH);
  digitalWrite(lockPin, HIGH);
  delay(delayTime);
  digitalWrite(lockPin, LOW);
  digitalWrite(ledPin, LOW);  
  delay(500);   // This delay is here because releasing the latch can cause a vibration that will be sensed as a knock.
}

void replayKnocks()
{
  int i = 0;

  digitalWrite(ledPin, HIGH);
  delay(20);
  digitalWrite(ledPin, LOW);
  
  for (i = 0; i < maximumKnocks; i++) {
    if (knockReadings[i] > 0) {
      delay(knockReadings[i]);
      digitalWrite(ledPin, HIGH);
      delay(20);
      digitalWrite(ledPin, LOW);
    }
  }
}

void resetKnockReadings()
{
  int i = 0;
  
  // First reset the listening array.
  for (i = 0; i < maximumKnocks; i++) {
    knockReadings[i] = 0;
  }
}

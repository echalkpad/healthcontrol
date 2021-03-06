#include <SoftwareSerial.h>
SoftwareSerial esp8266(2, 3);

#define DEBUG true

//  Variáveis
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
String comandoGet, tamanhoComando;                       // variável utilizada para o comando GET
String endServidor = "/healthcontrol/sendData.php?bpm=";
String endHost = "luiseduardoluz.com\r\n\r\n";


// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = false;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse



void setup() {
  esp8266.begin(115200);
  // Reseta o módulo
  sendData("AT+RST\r\n", 2000, DEBUG);
  delay(1000);
  Serial.println("\n\nVersao de firmware");
  delay(3000);
  sendData("AT+GMR\r\n", 2000, DEBUG); // rst
  // Configure na linha abaixo a velocidade desejada para a
  // comunicacao do modulo ESP8266 (9600, 19200, 38400, etc)
  sendData("AT+CIOBAUD=115200\r\n", 2000, DEBUG);
  Serial.println("\n\nConectando na rede Wi-Fi:");
  delay(3000);
  sendData("AT+CWJAP=\"UTFPR-WEB\",\"\"\r\n", 2000, DEBUG);
  sendData("AT+CWMODE=1\r\n", 1000, DEBUG);
  // Mostra o endereco IP
  Serial.println("\n\nEndereco IP:");
  delay(3000);
  sendData("AT+CIFSR\r\n", 1000, DEBUG);
  Serial.println("\n\n** Final **");
  pinMode(blinkPin, OUTPUT);        // pin that will blink to your heartbeat!
  pinMode(fadePin, OUTPUT);         // pin that will fade to your heartbeat!
  Serial.begin(9600);             // we agree to talk fast!
  Serial.print("BPM: ");
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
  // IF YOU ARE POWERING The Pulse Sensor AT VOLTAGE LESS THAN THE BOARD VOLTAGE,
  // UN-COMMENT THE NEXT LINE AND APPLY THAT VOLTAGE TO THE A-REF PIN
  //   analogReference(EXTERNAL);
}
void interruptSetup() {
  // Initializes Timer2 to throw an interrupt every 2mS.
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED
}

//  Where the Magic Happens
void loop() {
  // serialOutput() ;
  if (QS == true) {
    // A Heartbeat Was Found
    // BPM and IBI have been Determined
    // Quantified Self "QS" true when arduino finds a heartbeat
    fadeRate = 255;         // Makes the LED Fade Effect Happen
    // Set 'fadeRate' Variable to 255 to fade LED with pulse
    serialOutputWhenBeatHappens();   // A Beat Happened, Output that to serial.
    QS = false;                      // reset the Quantified Self flag for next time
  }

  ledFadeToBeat();                      // Makes the LED Fade Effect Happen
  delay(50);                             //  take a break
}





void ledFadeToBeat() {
  fadeRate -= 15;                         //  set LED fade value
  fadeRate = constrain(fadeRate, 0, 255); //  keep LED fade value from going into negative numbers!
  analogWrite(fadePin, fadeRate);         //  fade LED
}


//----------------------------------------------------------------------------------------------------------//

//  Decides How To OutPut BPM and IBI Data
void serialOutputWhenBeatHappens() {
  if (serialVisual == true) {           //  Code to Make the Serial Monitor Visualizer Work
    //Serial.print("*** Heart-Beat Happened *** ");  //ASCII Art Madness
    // Serial.print("BPM: ");
    Serial.print(BPM);
    Serial.print("\n ");
    } 
  else {
    sendDataToSerial('B', BPM);  // send heart rate with a 'B' prefix
    sendDataToSerial('Q', IBI);  // send time between beats with a 'Q' prefix
    comandoGet = ">GET " + endServidor + BPM + " HTTP/1.1\r\nHost: " + endHost;
    tamanhoComando = "AT+CIPSEND=" + (comandoGet.length() - 5 - 4); // quantidade de caracteres do comando GET
    sendData(tamanhoComando, 2000, DEBUG);

    sendData(comandoGet, 2000, DEBUG);
  }
}



//  Sends Data to Pulse Sensor Processing App, Native Mac App, or Third-party Serial Readers.
void sendDataToSerial(char symbol, int data ) {
  Serial.print(symbol);

  Serial.println(data);
}



//--------------------------------------------------------------------------------------------------

volatile int rate[10];                    // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int P = 512;                     // used to find peak in pulse wave, seeded
volatile int T = 512;                     // used to find trough in pulse wave, seeded
volatile int thresh = 525;                // used to find instant moment of heart beat, seeded
volatile int amp = 100;                   // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false;      // used to seed rate array so we startup with reasonable BPM





// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
ISR(TIMER2_COMPA_vect) {                        // triggered when Timer2 counts to 124
  cli();                                      // disable interrupts while we do this
  Signal = analogRead(pulsePin);              // read the Pulse Sensor
  sampleCounter += 2;                         // keep track of the time in mS with this variable
  int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise

  //  find the peak and trough of the pulse wave
  if (Signal < thresh && N > (IBI / 5) * 3) { // avoid dichrotic noise by waiting 3/5 of last IBI
    if (Signal < T) {                       // T is the trough
      T = Signal;                         // keep track of lowest point in pulse wave
    }
  }

  if (Signal > thresh && Signal > P) {        // thresh condition helps avoid noise
    P = Signal;                             // P is the peak
  }                                        // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (N > 250) {                                  // avoid high frequency noise
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3) ) {
      Pulse = true;                               // set the Pulse flag when we think there is a pulse
      digitalWrite(blinkPin, HIGH);               // turn on pin 13 LED
      IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
    }
  }

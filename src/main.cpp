/*
    Data Logger implementation in STM32 using Arduino Framework.
    Inputs:
        x2 Digital (Frequency) Inputs;
        x3 Analog Inputs;
        1x Serial Output;
    In this set, it is designed for a 200Hz sample rate.
    All the data are saved periodically (every 0.25s) to a folder in the SD card.
    To read the data, use the file "read_struct_2.0.c" in the folder results.
    
   Implemented by Einstein "Hashtag" Gustavo(Electronics Coordinator 2019) 
   at Mangue Baja Team, UFPE.

  Here is the base code to get usual information:

// Projeto para Criar / Ler Arquivos .txt junto ao Cartão SD Arduino
// Visite nossa loja através do link www.usinainfo.com.br
// Mais projetos em www.www.usinainfo.com.br/blog/</pre>
#include <SD.h>
#include <SPI.h>
 
File myFile;
 
int pinSS = 10; // Pin 53 para Mega / Pin 10 para UNO
 
void setup() { // Executado uma vez quando ligado o Arduino
 
Serial.begin(9600); // Define BaundRate
pinMode(pinSS, OUTPUT); // Declara pinSS como saída
 
if (SD.begin()) { // Inicializa o SD Card
Serial.println("SD Card pronto para uso."); // Imprime na tela
}
 
else {
Serial.println("Falha na inicialização do SD Card.");
return;
}
 
myFile = SD.open("usina.txt", FILE_WRITE); // Cria / Abre arquivo .txt
 
if (myFile) { // Se o Arquivo abrir imprime:
Serial.println("Escrevendo no Arquivo .txt"); // Imprime na tela
myFile.println("Usinainfo 1, 2 ,3 ..."); // Escreve no Arquivo
myFile.close(); // Fecha o Arquivo após escrever
Serial.println("Terminado."); // Imprime na tela
Serial.println(" ");
}
 
else {     // Se o Arquivo não abrir
Serial.println("Erro ao Abrir Arquivo .txt"); // Imprime na tela
}
 
myFile = SD.open("usina.txt"); // Abre o Arquivo
 
if (myFile) {
Serial.println("Conteúdo do Arquivo:"); // Imprime na tela
 
while (myFile.available()) { // Exibe o conteúdo do Arquivo
Serial.write(myFile.read());
}
 
myFile.close(); // Fecha o Arquivo após ler
}
 
else {
Serial.println("Erro ao Abrir Arquivo .txt"); // Imprime na tela
}
 
}
 
void loop() {
 
// Como a função é executada somente uma vez, esta área permanece em branco
 
}
*/

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Ticker.h>
#include <RingBuf.h>

#define SERIAL_BAUD 1000000                     // Board baud rate
#define BUFFER_SIZE 800                         // Acquisition buffer
#define SAVE_WHEN 50                            // Number of packets to save (fail safe)
#define SAMPLE_FREQ 200                         // Frequency in Hz

/* Data structure */
typedef struct
{
  uint16_t analog0;
  uint16_t analog1;
  uint16_t analog2;
  uint16_t pulses_chan1;
  uint16_t pulses_chan2;
  uint32_t time_stamp;
} packet_t;

const int debug = PB9;
const int pinSS = PB12;                            // NSS SPI Pin
const int warning = PA12;
const int logging = PA15;
const int freq_channel1 = PB5;
const int freq_channel2 = PB4;
const int pot0 = PB1;
const int pot1 = PB0;
const int pot2 = PA7;
const int start = PB6;
bool running = false;                             // Device status
int buffer_counter = 0;                           // Packet currently in buffer                            
RingBuf<packet_t, BUFFER_SIZE> buffer;            // Acquisition buffer
uint16_t pulse_counter1 = 0,
         pulse_counter2 = 0;
         
uint32_t count_files_in_sd(const char *fsrc);     // Compute number of files in SD
void sampleISR();                                 // Data acquisition ISR
void freq_channel1_ISR();                         // Frequency counter ISR, channel 1
void freq_channel2_ISR();                         // Frequency counter ISR, channel 2
void toggle_logging();                            // Start button ISR
void IRQHandler(void);
Ticker acq(IRQHandler, 1000, 0, MILLIS);
void setup() 
{
  Serial.begin(SERIAL_BAUD);                         // Define BaundRate
  pinMode(warning, OUTPUT);
  pinMode(debug, OUTPUT);                          // When device is ready, led is permanently OFF
  pinMode(logging, OUTPUT);                          // When data is beign acquired, led is ON
  pinMode(freq_channel1, INPUT_PULLUP);
  pinMode(freq_channel2, INPUT_PULLUP);
  pinMode(start, INPUT_PULLUP);
}

void loop()
{
  while(!SD.begin(pinSS)) 
  {                                           // Inites SD Card
    digitalWrite(warning, 1);
    delay(1000);
    digitalWrite(warning, 0);
  }

  digitalWrite(logging, 0);                   // logging led OFF
  int num_parts = 0,                          // Number of parts already saved
      num_files = 0,                          // Number of files in SD
      svd_pck = 0;                            // Number of saved packets (in current part)
  char name_dir[12];                          // Name of current folder (new RUN)
  char name_file[20];                         // Name of current file (partX)
  File myFile;
  packet_t temp;

  num_files = count_files_in_sd("/");
  sprintf(name_dir, "%s%d", "/RUN", num_files+1);

  attachInterrupt(start,toggle_logging,FALLING);                 // Attach start button ISR

  while(!running)
  {                                                             // Wait button press
    digitalWrite(warning, 1);
  }
  
  /* Create RUN directory */
  SD.mkdir(name_dir);
  digitalWrite(warning, 0);
  sprintf(name_file, "%s%s%d", name_dir, "/part", num_parts+1);
  myFile = SD.open(name_file, FILE_WRITE);                            // Creates first data file                                             // Start device timer
  attachInterrupt(freq_channel1, freq_channel1_ISR, FALLING);
  attachInterrupt(freq_channel2, freq_channel2_ISR, FALLING);
  acq.start();
  acq.update();
  digitalWrite(logging, 1);                                           // logging led ON

  while(running)
    {
      acq.update();
      if(buffer.isFull())
      {
        myFile.close();
        digitalWrite(warning, 1);                // Turn warning led ON if buffer gets full (abnormal situation)
        Serial.write('X');                       // Debug message
      }
      else if(!buffer.isEmpty())
      {   
        Serial.write('G');                         // Debug message

        /* Remove packet from buffer and writes it to file */
        buffer.pop(temp);                  
        buffer_counter--;
        myFile.write((uint8_t*)&temp,sizeof(packet_t));
        svd_pck++;

        /* Create new data file */
        if(svd_pck == SAVE_WHEN)
        {   
          //myFile.close();
          //sprintf(name_file, "%s%s%d", name_dir, "/part", num_parts++);
          //myFile = SD.open(name_file, FILE_WRITE);
          //Serial.println("%d\r\n", buffer_counter);                   // Debug message
          svd_pck = 0;
        }
      }
  }
  /* Reset device if start button is pressed while logging */
  myFile.close();
  digitalWrite(logging, 0);
  //NVIC_SystemReset();
}

void sampleISR()
{
    static uint16_t last_acq = millis();        // Time of last acquisition
    packet_t acq_pck;                           // Current data packet

    acq_pck.analog0 = analogRead(pot0);          // Read analog sensor 0            
    acq_pck.analog1 = analogRead(pot1);          // Read analog sensor 1
    acq_pck.analog2 = analogRead(pot2);          // Read analog sensor 2
    acq_pck.pulses_chan1 = pulse_counter1;       // Store frequence channel 1
    acq_pck.pulses_chan2 = pulse_counter2;       // Store frequence channel 2
    acq_pck.time_stamp = millis() - last_acq;    // Timestamp of data acquistion
    
    pulse_counter1= 0;
    pulse_counter2= 0;
    buffer.push(acq_pck);
    buffer_counter++;
    TIM2->SR = 0x00U;
}

uint32_t count_files_in_sd(const char *fsrc)
{   
  File d = SD.open(fsrc);
  uint32_t counter = 0;

  counter = d.count("FOLDERS");
    return counter;
}

void freq_channel1_ISR()
{
  pulse_counter1++;
}

void freq_channel2_ISR()
{
  pulse_counter2++;
}

void toggle_logging()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 500ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 500)
  {
    running = !running;
  }
  last_interrupt_time = interrupt_time;
}

void IRQHandler(void)
{
    digitalWrite(debug, !debug);
}
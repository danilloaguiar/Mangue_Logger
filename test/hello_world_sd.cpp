#include <SD.h>
#include <SPI.h>
 
File myFile;
 
int pinSS = PB12; // Pin 53 para Mega / Pin 10 para UNO
int warning = PA0;

void setup() { // Executado uma vez quando ligado o Arduino
 
Serial.begin(9600); // Define BaundRate
pinMode(warning, OUTPUT); 
 
if (SD.begin(pinSS)) { // Inicializa o SD Card
digitalWrite(warning, 1);
}
 
else {
digitalWrite(warning, 0);
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

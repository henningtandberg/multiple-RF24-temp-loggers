#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>

#define RF24_CE         10  //Used as dig pin
#define RF24_CSN        9  //Used as dig pin
#define TMPSENS_1       A0  //Readpin for tmp sensor

typedef struct pakcet {
    float value;
} packet;

RF24 radio(RF24_CE, RF24_CSN);
uint8_t addr[6] = { "node1" };
packet pack = {0};
uint8_t packlen = sizeof(pack);

float get_temperature(int readpin);

void setup() {

    Serial.begin(9600);

    radio.begin();
    radio.setDataRate(RF24_250KBPS);
    radio.openWritingPipe(addr);
    radio.stopListening();

    radio.printDetails();

}

void loop() {

    pack.value = analogRead(TMPSENS_1);//get_temperature(TMPSENS_1);
    radio.write(&pack, packlen);

    Serial.println("Packet sent!");
    Serial.print("Celcius = ");
    Serial.println(pack.value, 2);

}

float get_temperature(int readpin){
    float voltage = (analogRead(readpin) * 5.0);
    voltage /= 1024.0;

    /*
    Serial.print("Voltage: ");
    Serial.print(voltage);
    */

    return (voltage - 0.5) * 100;
} //END get_temperature

#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <Adafruit_CC3000.h>

#define CC3K_IRQ    3   //Must be an interrupt pin!
#define CC3K_VBAT   5   //This can be any pin
#define CC3K_CS     10  //CS pin for WIFI
#define SD_CS       4   //CS pin for SD-card

#define RF24_CE     A1  //Used as dig pin
#define RF24_CSN    A2  //Used as dig pin
#define NODE_NUM    3   //Number of nodes in the network

#define TMPSENS_1   A0  //Readpin for tmp sensor

#define _PORT       80              //TCP port
#define _SSID       "NASA-HQ"       //WLAN SSID must be <= 32 chars
#define _PASS       "PASSWORD123"   //WLAN password <= 32 chars?
#define _SECU       WLAN_SEC_WPA2   //WLAN security
                                    //Other options:
                                    //WLAN_SEC_UNSEC
                                    //WLAN_SEC_WEP
                                    //WLAN_SEC_WPA

typedef struct pakcet {
    float value;
} packet;


Adafruit_CC3000 wifi = Adafruit_CC3000(CC3K_CS, CC3K_IRQ, CC3K_VBAT, SPI_CLOCK_DIVIDER);
Adafruit_CC3000_Client client;
RF24 radio(RF24_CE, RF24_CSN);

char website[] = "192.168.0.13";
uint32_t ip = 0;

uint8_t nodes[][6] = {"node1", "node2", "node3"}; //Adresses
packet packets[NODE_NUM] = {0};
uint8_t packlen = sizeof(packet[0]);

float celcius = 0.;

void init_wifi_shield();
void delete_profiles();
void connect_wifi();
void request_dhcp();
void get_ip();
bool connect_to_client();
void post_request();
float get_temperature(int readpin);
String float_to_string(float f, uint8_t nl, uint8_t dl);


void setup() {

    Serial.begin(9600);

    //INITIALIZING WIFISHIELD
    //init_wifi_shield();

    //DELETING OLD CONNECTION PROFILES
    //delete_profiles();

    //ESTABLISHING WIFI CONNECTION
    //connect_wifi();

    //REQUEST DHCP
    //request_dhcp();

    //SET STATIC IP
    /* set_static_ip(); */

    //GET SERVER IP
    //get_ip();

    //SET UP FOR nRF24L01
    radio.begin();
    radio.setDataRate(RF24_250KBPS); //Both ends must be configured like this
    for(uint8_t i = 1; i <= NODE_NUM; i++) {
        radio.openReadingPipe(i, nodes[i-1]);
    }
    radio.startListening();

    for(int i = 0; i < NODE_NUM; i++)
        packets[i].value = 0.;

    //INITIALIZING TEMP PINS
    pinMode(TMPSENS_1, INPUT);

    Serial.println("## INITIALIZION COMPLETE! ##");
    Serial.print("Starting in:");
    for(int i = 1; i <= 3; i++) {
        Serial.print(" " + (String)i);
        delay(1000);
    } Serial.println(" LIVE!");

} //END setup

void loop() {

    //CALCULATING Temperature
    celcius = get_temperature(TMPSENS_1);

    //FIX FOR MULTIPLE NODES!
    //RECV from nRF24L01
    //for(uint8_t i = 1; i <= NODE_NUM; i++) {
        while(radio.available()) {
            radio.read(&packets[0], packlen);
        }
    //}

    dump_nrf_packets(packets, NODE_NUM);
    Serial.print("Server temp: ");
    Serial.println(celcius, 2);

    //POSTING REQUEST TO CLIENT
    //post_request();

    //waits 10 seconds
    delay(2000); //Use this?

} //END loop

void init_wifi_shield() {
    Serial.print(F("Iitialaizing CC3000 -> "));
    if(!wifi.begin()){
        Serial.println(F("Failed!\n"));
        while(1);
    } Serial.println("OK!");
} //END init_wifi_shield

void delete_profiles() {
    Serial.print(F("Deleting old profiles -> "));
    if (!wifi.deleteProfiles()) {
        Serial.println(F("Failed!"));
        while (1);
    } Serial.println("OK!");
} //END delete_profiles

void connect_wifi() {
    Serial.print(F("Connecting to: ")); Serial.print(_SSID);
    Serial.print(" -> ");
    if(!wifi.connectToAP(_SSID, _PASS, _SECU)){
        Serial.println(F("Failed!"));
        while(1);
    } Serial.println("OK!");
} //END connect_wifi

void set_static_ip(){
    uint32_t staticip = wifi.IP2U32(192, 168, 0, 100);
    uint32_t netmask = wifi.IP2U32(255, 255, 255, 0);
    uint32_t gateway = wifi.IP2U32(192, 168, 0, 1);
    uint32_t dns = wifi.IP2U32(8, 8, 8, 8);

    Serial.print("Setting ip to: " + (String)staticip );
    Serial.print("-> ");
    if(!wifi.setStaticIPAddress(staticip, netmask, gateway, dns)){
        Serial.println("Failed!");
        while(1);
    } Serial.println("OK!");
} //END set_static_ip

void request_dhcp() {
    Serial.print(F("Requesting DHCP -> "));
    uint8_t timeout = 0;

    while(!wifi.checkDHCP()){
        if(timeout > 9){
            Serial.println(F("Failed"));
            while(1);
        }
        timeout++;
        delay(100);
    } Serial.println(F("OK!"));
} //END request_dhcp

void get_ip(){
    Serial.print("Getting IP from: "); Serial.print(website);
    Serial.print(" -> ");
    while (ip == 0) {
      if (! wifi.getHostByName(website, &ip) ) { //Ip will be set to webpage's Ip
        Serial.println(F("Failed!"));
        while(1);
      }
      delay(500);
    }
    Serial.println("OK!");
} //END get_ip

bool connect_to_client() {
    Serial.print(F("Connecting to client: ")); wifi.printIPdotsRev(ip);
    Serial.print(F(" -> "));
    client = wifi.connectTCP(ip, _PORT);
    if(!client.connected()){
        Serial.println(F("FAILED!"));
        return false;
    } Serial.println(F("OK!"));
    return true;
} //END connect_to_client

void post_request() {
    if(connect_to_client()) {
        //String data = "temp1=" + float_to_string(celcius, 2, 1);
        String data = "temp1=" + (String)celcius;

        Serial.println("\nPosting request to client..");
        client.println("POST /rokovn/add.php HTTP/1.1");
        client.println("Host: 192.168.1.208"); // SERVER ADDRESS HERE TOO
    	client.println("Content-Type: application/x-www-form-urlencoded");
    	client.print("Content-Length: ");
    	client.println(data.length());
    	client.println();
    	client.print(data);

        Serial.print("Closing connection -> ");
        client.close();
        Serial.println("OK!");
    }
} //END post_request

void dump_nrf_packets(packet p[], uint8_t pcount) {
    for(int i = 0; i < pcount; i++) {
        Serial.print("Node["+(String)(i+1)+"] - CELCIUS: ");
        Serial.println(p[i].value, 2);
    } Serial.println();
} //END dump_nrf_packets

String float_to_string(float f, uint8_t nl, uint8_t dl) {
    String str = "";
    char buf[nl+dl+1];

    dtostrf(f, nl, dl, buf);

    for(uint8_t i = 0; i < (nl+dl+1); i++) {
        str += buf[i];
    }

    return str;
} //END float_to_string

float get_temperature(int readpin){
    float voltage = (analogRead(readpin) * 5.0);
    voltage /= 1024.0;

    /*
    Serial.print("Voltage: ");
    Serial.print(voltage);
    */

    return (voltage - 0.5) * 100;
} //END get_temperature

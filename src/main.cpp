#include <Arduino.h>
#include <WiFi.h>


int local_wifiSendAndReceive(uint8_t* buffer, int bufferLen)
{
	WiFiClient 	client;
	int			tt;
	int			maxTimeInCentSec;

	tt = client.connect("192.168.105.254", 40001);
	if(!tt){ 
        return 1;
	}

	// Send
	client.write(buffer, bufferLen);

	// Wait for response
	maxTimeInCentSec = 100;
	for(tt = 0; tt <maxTimeInCentSec; tt++){
		if(client.available()){
			break;
		}
		delay(10);
	}

	if(tt == maxTimeInCentSec){
		//	No Response
		client.stop();
		return 2;
	}

	// Check response
	tt = client.read(buffer,256);
	if(tt <  0){
		client.stop();
		return 3;
	}	

	if(tt == 0){
		client.stop();
		return 4;
	}	
	
	tt = memcmp(buffer,"OK",2);
	if(tt != 0){
		// Server response was different from expected
		client.stop();
        return 5;
	}

    client.stop();
	return 0;
}
//-----------------------------------------------------------------------------

void setup() 
{
    uint32_t diff;
    uint32_t msBegin;

    Serial.begin(115200);
	WiFi.begin("Tower", "barbacabelobigode");

    delay(1000);
    
    msBegin = millis();

	do{
        if (WiFi.status() == WL_CONNECTED){
            break;
        }
        diff = millis();
        diff -= msBegin;
    }while(diff < 5000);
    
    
    if(diff < 5000){
        Serial.println("\n\n");
        Serial.println("Conectado");
        Serial.println("\n\n");
        delay(10000);
        pinMode(5,  OUTPUT);
        digitalWrite(5, HIGH);
    }

    Serial.println("\n\n");
    Serial.println("falha");
    Serial.println("\n\n");
    delay(10000);
    pinMode(5,  OUTPUT);
    digitalWrite(5, HIGH);
}
//-------------------------------------------------------------------------------------------------

void loop() 
{

}


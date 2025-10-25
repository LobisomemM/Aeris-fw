// #include <Arduino.h>
// #include <WiFi.h>

// int local_wifiSendAndReceive(uint8_t* buffer, int bufferLen)
// {
// 	WiFiClient 	client;
// 	int			tt;
// 	int			maxTimeInCentSec;

// 	tt = client.connect("192.168.105.254", 40009);
// 	if(!tt){
//         return 1;
// 	}

// 	// Send
// 	client.write(buffer, bufferLen);

// 	// Wait for response
// 	maxTimeInCentSec = 100;
// 	for(tt = 0; tt <maxTimeInCentSec; tt++){
// 		if(client.available()){
// 			break;
// 		}
// 		delay(10);
// 	}

// 	if(tt == maxTimeInCentSec){
// 		//	No Response
// 		client.stop();
// 		return 2;
// 	}

// 	// Check response
// 	tt = client.read(buffer,256);
// 	if(tt <  0){
// 		client.stop();
// 		return 3;
// 	}

// 	if(tt == 0){
// 		client.stop();
// 		return 4;
// 	}

// 	tt = memcmp(buffer,"OK",2);
// 	if(tt != 0){
// 		// Server response was different from expected
// 		client.stop();
//         return 5;
// 	}

//     client.stop();
// 	return 0;
// }
// //-----------------------------------------------------------------------------

// char 		g_WIFI_SSID[] = "Tower";
// char 		g_WIFI_PASS[] = "barbacabelobigode";
// uint32_t	g_firstTick;

// void setup()
// {
//     uint32_t diff;
//     uint32_t msBegin;

//     Serial.begin(115200);
// 	delay(1000);

// 	WiFi.begin(g_WIFI_SSID, g_WIFI_PASS);
//     g_firstTick = millis();
// }
// //-------------------------------------------------------------------------------------------------

// void loop()
// {

// 	if (WiFi.status() == WL_CONNECTED){
// 		uint32_t	diff;

// 		diff = millis();
// 		diff -= g_firstTick;
// 		Serial.println("\n\n");
//         Serial.print("Conectado: ");
//         Serial.print(diff);
// 		Serial.println(" ms\n\n");

// 		char stsend[] = "EU NAO ME CONTROLO TOM BOMBADIL";
// 		local_wifiSendAndReceive((uint8_t*)stsend,strlen(stsend));
// 		delay(5000);
// 	}

// 	if (Serial.available() > 0) {
// 		byte dado = Serial.read();   // lê 1 byte (0–255)

// 		if(dado== ' '){
// 			pinMode(5,  OUTPUT);
// 			digitalWrite(5, HIGH);
// 		}

// 		Serial.print("Recebido: ");
//   		Serial.println(dado, HEX);   // imprime em hexadecimal

// 	}
// 	delay(10);
// }

#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <esp_system.h>   // esp_read_mac
#include <LittleFS.h>

#define DHTPIN    		4       
#define DHTTYPE   		DHT22   
#define RX_BUF_SIZE		256


uint8_t 				g_rxIndex; 
uint8_t					g_rxBuffer[RX_BUF_SIZE];

volatile uint32_t 		g_bytesToSend 		= 0;
uint8_t           		g_localBuffer[512];
uint8_t*          		g_bufferToSend 		= g_localBuffer;

char 					WIFI_SSID[128] 		= "Tower";
char 					WIFI_PSK[128]  		= "barbacabelobigode";
char              		g_hostToSend[128] 	= "192.168.105.254";
char              		g_portToSend[8] 	= "40009";

uint8_t					g_lfsMountedFlag	= 0;

volatile bool 			g_flagGotIP = false;
volatile bool 			g_flagDisconnected = false;

static WiFiEventId_t 	s_evtIdConnected;
static WiFiEventId_t 	s_evtIdGotIP;
static WiFiEventId_t 	s_evtIdDisconnected;

DHT dht(DHTPIN, DHTTYPE);




int local_saveConfig(const String& ssid, const String& pass, const String& host, const String& port) 
{
	if(g_lfsMountedFlag == 0){
		return -20;
	}

	// Line 1 - SSID
	// Line 2 - PASS
	// Line 3 - host
	// Line 4 - port
	File f = LittleFS.open("/config.txt", FILE_WRITE);
	if (!f) {
		return -1;
	}

	f.printf("ssid=%s\npass=%s\nhost=%s\nport=%s\n", ssid.c_str(), pass.c_str(), host.c_str(), port.c_str());
	f.close();

	return 0;
}
//-----------------------------------------------------------------------------

int local_readConfig(void) 
{
	char* 	buf;
	size_t 	bufSize = 1024;
	int		flg;
	char* 	ssid;
	char* 	pass;
	char* 	host;
	char* 	port;

	if(g_lfsMountedFlag == 0){
		return -20;
	}

	File f = LittleFS.open("/config.txt", FILE_READ);
	if (!f) {
		return -1;
	}
	
	buf = (char*)malloc(bufSize);
	if(buf == 0){
		return -10;
	}

	size_t total = 0;
	while (f.available() && total < bufSize - 1) {
		buf[total++] = f.read();
	}
	buf[total] = '\0';  
	f.close();


	char* line = strtok(buf, "\n");
	if (line != nullptr) {
	    flg = memcmp(line,"ssid=",5);
		if(flg){
			free(buf);
			return -11;
		}
		ssid = &line[5];
	}

	line = strtok(nullptr, "\n");
	if (line != nullptr) {
	    flg = memcmp(line,"pass=",5);
		if(flg){
			free(buf);
			return -12;
		}
		pass = &line[5];
	}

	line = strtok(nullptr, "\n");
	if (line != nullptr) {
	    flg = memcmp(line,"host=",5);
		if(flg){
			free(buf);
			return -13;
		}
		host = &line[5];
	}

	line = strtok(nullptr, "\n");
	if (line != nullptr) {
	    flg = memcmp(line,"port=",5);
		if(flg){
			free(buf);
			return -14;
		}
		port = &line[5];
	}

	strncpy(WIFI_SSID,ssid,128);
	strncpy(WIFI_PSK,pass,128);
	strncpy(g_hostToSend,host,128);
	strncpy(g_portToSend,port,8);
	
	free(buf);
	return 0;
}
//-----------------------------------------------------------------------------

void local_processLine(uint8_t* line, int lineLen)
{
	int ret;

	if(line[0] == '?'){
		Serial.printf("\r\nSSID = %s\nPASS = %s\nHOST = %s\nPORT = %s\n", WIFI_SSID, WIFI_PSK, g_hostToSend, g_portToSend);			
		Serial.println("OK");
		return;
	}

	if (lineLen < 5){
		return;
	}

	if(memcmp(line, "SSID=", 5) == 0){
		if (lineLen < 6){
			return;
		}

		strncpy(WIFI_SSID,(char*)&line[5],128);
		ret = local_saveConfig(WIFI_SSID,WIFI_PSK,g_hostToSend,g_portToSend);
		if(ret == 0){
			Serial.println("OK");
		}
		return;
	}

	if(memcmp(line, "PASS=", 5) == 0){
		strncpy(WIFI_PSK,(char*)&line[5],128);
		ret = local_saveConfig(WIFI_SSID,WIFI_PSK,g_hostToSend,g_portToSend);
		if(ret == 0){
			Serial.println("OK");
		}
		return;
	}

	if(memcmp(line, "HOST=", 5) == 0){
		if (lineLen < 9){
			return;
		}

		strncpy(g_hostToSend,(char*)&line[5],128);
		ret = local_saveConfig(WIFI_SSID,WIFI_PSK,g_hostToSend,g_portToSend);
		if(ret == 0){
			Serial.println("OK");
		}
		return;
	}

	if(memcmp(line, "PORT=", 5) == 0){
		if (lineLen < 6){
			return;
		}


		char* endPtr;
		unsigned long temp = strtoul((char*)&line[5], &endPtr, 10);  // base 10
		if (*endPtr != '\0'){
			Serial.println("Error");
			return;
		}

		strncpy(g_portToSend,(char*)&line[5],8);
		ret = local_saveConfig(WIFI_SSID,WIFI_PSK,g_hostToSend,g_portToSend);
		if(ret == 0){
			Serial.println("OK");
		}
		return;
	}
	
}
//-----------------------------------------------------------------------------




bool tcpSendBuffer(const char* host, const char* portIn, const uint8_t *buf, size_t len, uint32_t connectTimeoutMs, uint32_t writeTimeoutMs)
{
	WiFiClient client;

	char* endPtr;
	unsigned long temp = strtoul(portIn, &endPtr, 10);  // base 10
	if (*endPtr != '\0') return false;
	uint16_t port = (uint16_t)temp;


	if (!host || !buf || len == 0){
		return false;
	}

	uint32_t t0 = millis();
	while (!client.connected()){
		if (client.connect(host, port))
			break;
		if (millis() - t0 > connectTimeoutMs)
		{
			Serial.printf("[TCP] Timeout conectando em %s:%u\r\n", host, port);
			return false;
		}
		delay(50);
	}

	// Escrever tudo, tratando escrita parcial
	size_t offset = 0;
	uint32_t tw0 = millis();
	while (offset < len){
		size_t chunk = client.write(buf + offset, len - offset);
		if (chunk > 0)
		{
			offset += chunk;
			tw0 = millis(); // reset no watchdog de timeout de escrita
		}
		else
		{
			// Nada foi escrito — cheque timeout de escrita
			if (millis() - tw0 > writeTimeoutMs)
			{
				Serial.println("[TCP] Timeout de escrita.");
				client.stop();
				return false;
			}
			delay(10);
		}
	}

	// (Opcional) garantir que saiu do buffer do socket
	client.flush();

	// Não é requerido aguardar resposta; feche
	client.stop();
	return true;
}
//-----------------------------------------------------------------------------

void onWifiConnected()
{
	IPAddress ip = WiFi.localIP();
	uint32_t pending = g_bytesToSend; // snapshot (evita race de leitura repetida)
	Serial.printf("[WiFi] Conectado! IP: %s\r\n", ip.toString().c_str());

	if (pending > 0 && g_bufferToSend != nullptr)
	{
		Serial.printf("[TX] Pendentes: %lu bytes para %s:%s\r\n", (unsigned long)pending, g_hostToSend, g_portToSend);

		bool ok = tcpSendBuffer(g_hostToSend, g_portToSend, g_bufferToSend, (size_t)pending, 2000, 2000);

		if (ok)
		{
			Serial.println("[TX] Envio concluido com sucesso.");
			g_bytesToSend = 0;
		}
		else
		{
			Serial.println("[TX] Falha no envio TCP (mantendo bytes pendentes).");
			// Se quiser, você pode tentar re-agendar um envio futuro aqui.
			// Ex.: marcar um timestamp para tentar novamente no loop.
		}
	}
}
//-----------------------------------------------------------------------------

void onWifiDisconnected()
{
	Serial.println("[WiFi] Desconectado. Tentando reconectar...");
	if (WiFi.status() != WL_CONNECTED)
	{
		WiFi.reconnect();
	}
}
//-----------------------------------------------------------------------------

void setup()
{
	uint32_t 	t0;
	int			pcount;
	int 		wokeFromPin = 1;
	float 		hum;
	float 		tem;
	int 		ret;
	
	dht.begin();

	if(wokeFromPin){
		Serial.begin(115200);		
		while (!Serial && millis() < 3000){
			delay(10);
		}
	}

	//-------------
	//-   LFS   ---
	//-------------
	if (!LittleFS.begin(true)) {  // true → formata se falhar montar
		if(wokeFromPin){
			Serial.println("Falha ao montar LittleFS");
		}
	}
	else{
		g_lfsMountedFlag = 1;
		if(wokeFromPin){
			Serial.println("LittleFS montado com sucesso");
		}
	}

	if (!LittleFS.exists("/config.txt")) {
		local_saveConfig(WIFI_SSID,  WIFI_PSK,  g_hostToSend, g_portToSend);
	}
	else{
		ret = local_readConfig();
		if(ret && wokeFromPin){
			Serial.printf("Error: Falha na leitura da configuracao: %d\r\n",ret);
		}
	}

	//-----------------------------------
	//-   Buffer to send to BackEnd   ---
	//-----------------------------------
	memcpy (&g_bufferToSend[0],  "IOTCONANAERIS",13);
	memcpy (&g_bufferToSend[13], "0000001",7);
	memcpy (&g_bufferToSend[20], "FALHA\r\n",7);
	g_bufferToSend[27] = 0;

	//----------------
	//-   DHT 22   ---
	//----------------
	t0 = 0;
	do{
		hum = dht.readHumidity();
		tem = dht.readTemperature();
		delay(50);

		if(!(isnan(hum)) &&  !(isnan(tem))){
			sprintf((char*)&g_bufferToSend[20],"%.1f|%.1f\r\n\0", hum, tem);
		}

		if(++t0 >= 20){
			break;
		}
	}while (isnan(hum) || isnan(tem));

	if(wokeFromPin){
		int len = strlen((char*)&g_bufferToSend[0]);
		Serial.println((char*)&g_bufferToSend[0]);
	}


	//---------------
	//-   WI-FI   ---
	//---------------

	// before Wi-Fi
	s_evtIdConnected 	= WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) {}								, ARDUINO_EVENT_WIFI_STA_CONNECTED);
	s_evtIdGotIP 		= WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) { g_flagGotIP = true; }			, ARDUINO_EVENT_WIFI_STA_GOT_IP);
	s_evtIdDisconnected = WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) { g_flagDisconnected = true; }	, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

	WiFi.mode(WIFI_STA);
	WiFi.persistent(false);
	WiFi.setAutoReconnect(true);
	WiFi.setHostname("Aeris - Conan");
	WiFi.begin(WIFI_SSID, WIFI_PSK);

	if(wokeFromPin){
		Serial.printf("Conectando em %s\r\n", WIFI_SSID);
	}

	//----------------------------------------
	//-   WI-FI   (wait to enter on wifi   ---
	//----------------------------------------
	t0 		= millis();
	pcount 	= 0;
	while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 10000){
		
		if(wokeFromPin){
			Serial.print(".");
			if (!++pcount % 40){
				Serial.print("\r\n");
			}
		}
		delay(25);
	}
	
	if(wokeFromPin){
		if(WiFi.status() == WL_CONNECTED){
			int len;

			len = strlen(WIFI_SSID);
			Serial.println("\r\n");
			for (int i=0; i < len+16;  i++)  { Serial.print("-"); }
			Serial.printf("\r\n- Conectado a %s -\r\n", WIFI_SSID);
			for (int i=0; i < len+16;  i++)  { Serial.print("-"); }			
			Serial.print("\r\n");
		}
	}

	//-------------------------------
	//-   WI-FI   (wait for IP)   ---
	//-------------------------------
	while (g_flagGotIP == false);

	t0 = millis();
	if(wokeFromPin){
		Serial.printf("Enviando: %lu\r\n", t0);
	}
	
	//------------------------------
	//-   WI-FI   (send buffer)  ---
	//------------------------------
	bool ok = tcpSendBuffer(g_hostToSend, g_portToSend, g_bufferToSend, (size_t)100, 2000, 2000);
	if(ok){
		if(wokeFromPin){
			if(WiFi.status() == WL_CONNECTED){
				int t1 = millis() - t0;
				Serial.printf("Sucesso. Tempo: %lu ms\r\n", t1);
				Serial.print((char*) g_bufferToSend);
			}
		}
	}
}
//-----------------------------------------------------------------------------

void loop()
{
	// if (g_flagGotIP){
	// 	g_flagGotIP = false;
	// 	onWifiConnected();
	// }

	// if (g_flagDisconnected){
	// 	g_flagDisconnected = false;
	// 	onWifiDisconnected();
	// }

	// if (millis() - g_lastTick > 1000){
	// 	g_lastTick = millis();
	// 	Serial.printf("[uptime] %lus | WiFi=%s\r\n", (unsigned long)(millis() / 1000), WiFi.isConnected() ? "ON" : "OFF");
	// }


	while (Serial.available()) {
		char c = Serial.read();

		// Protege contra buffer cheio
		if (g_rxIndex < RX_BUF_SIZE - 1) {
			g_rxBuffer[g_rxIndex++] = c;
		}

		if (c == '\n') {
			g_rxIndex --;
			g_rxBuffer[g_rxIndex] = '\0';

			if (g_rxIndex  && g_rxBuffer[g_rxIndex - 1] == '\r') {
				g_rxIndex --;
				g_rxBuffer[g_rxIndex] = '\0';
			}

			local_processLine(g_rxBuffer, g_rxIndex);
			g_rxIndex = 0;  // zera para próxima linha
		}
	}
}
//-----------------------------------------------------------------------------

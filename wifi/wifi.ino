	#include <SoftwareSerial.h>
	#include <WiFiEsp.h>
	
	#define WIFI_SSID "MakeNTU2020-B-5G"  //填入WiFi帳號
	#define WIFI_PASSWORD "innovation"  //填入WiFi密碼

	// IFTTT setup
	const char *host = "maker.ifttt.com";
	const char *Maker_Event = "Your EVENT Name";
	const char *Your_Key = "Your key on Maker Channel";
	
	int WiFi_Status = WL_IDLE_STATUS; //ＷiFi狀態
	
	unsigned long prevMillis = 0; //暫存經過時間(單位:毫秒)
	
	SoftwareSerial ESP8266(5,6);  //設定序列埠物件
	DHT dht(5,DHT22);  //設定DHT物件
	
	WiFiEspClient espClient;  //設定WiFiEspClient物件
	PubSubClient client(espClient); //設定PubSubClient物件(帶入espClient)
	
	void setup() {
	  
	    //設定序列埠傳輸速率(9600bps)
	    Serial.begin(9600);   
	
	    //wifi設定
	    wifi_Setting();
	}
	
	void loop() {
		Serial.print("Connecting to ");
  		Serial.println(host);
  
  		// Use WiFiClient class to create TCP connections
  		WiFiClient client;
  		const int httpPort = 80;
  		if (!client.connect(host, httpPort)) 
  		{
    		Serial.println("Connection failed");
    		return;
  		}
  
  		// We now create a URI for the request
  		String url = "/trigger/";
  		url += event;
  		url += "/with/key/";
  		url += Your_Key;
  
  		Serial.print("Requesting URL: ");
  		Serial.println(url);
  
  		// This will send the request to the server
  		client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");

  		// Read all the lines of the reply from server and print them to Serial,
  		// the connection will close when the server has sent all the data.
  		while(client.connected())
  		{
    		if(client.available())
    		{
      			String line = client.readStringUntil('\r');
      			Serial.print(line);
    		}
    		else 
    		{
      			// No data yet, wait a bit
      			delay(50);
    		};
  		}
  
  		// All done
  		Serial.println();
  		Serial.println("closing connection");

  		client.stop();
	}
	
	//wifi設定方法
	void wifi_Setting(){
	    //設定ESP8266傳輸速率(9600bps)
	    ESP8266.begin(9600); 
	    
	    //初始化ESP模組
	    WiFi.init(&ESP8266);
	
	    Serial.print("進行WiFi設定!\r\n");
	    do{
	        Serial.println("WiFi 連接中 ...");
	        WiFi_Status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	        delay(500);
	    } while (WiFi_Status != WL_CONNECTED);
	    
	    Serial.print("ＷiFi 連接成功!\r\n");
	    Serial.print("IP 位址: ");       
	    Serial.println(WiFi.localIP());
	    Serial.print("SSID: ");     
	    Serial.println(WiFi.SSID());
	    Serial.println("WiFi 設定結束\r\n");
	}
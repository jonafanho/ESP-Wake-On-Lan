#include <Arduino.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
ESP8266WebServer server(80);
#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
WebServer server(80);
#endif

String wifi_ssid = "";
String wifi_pass = "";
String target_mac = "";

WiFiUDP UDP;
WakeOnLan WOL(UDP);

bool loadSettings()
{
	if (!LittleFS.begin())
	{
		Serial.println("Failed to mount file system");
		return false;
	}

	File file = LittleFS.open("/settings.txt", "r");
	if (!file)
	{
		Serial.println("Failed to open settings.txt");
		return false;
	}

	if (file.available())
	{
		wifi_ssid = file.readStringUntil('\n');
	}
	if (file.available())
	{
		wifi_pass = file.readStringUntil('\n');
	}
	if (file.available())
	{
		target_mac = file.readStringUntil('\n');
	}

	file.close();
	wifi_ssid.trim();
	wifi_pass.trim();
	target_mac.trim();

	if (wifi_ssid.length() == 0 || target_mac.length() == 0)
	{
		Serial.println("Settings file is empty or missing data!");
		return false;
	}

	Serial.println("SSID: " + wifi_ssid);
	Serial.println("MAC:  " + target_mac);
	return true;
}

void checkWiFi()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.print("Connecting to WiFi...");
		WiFi.mode(WIFI_STA);
		WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());

		unsigned long startAttempt = millis();
		while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000)
		{
			delay(500);
			Serial.print(".");
		}

		if (WiFi.status() == WL_CONNECTED)
		{
			Serial.print("\nIP address: ");
			Serial.println(WiFi.localIP());
		}
		else
		{
			Serial.println("\nWiFi connection failed; retrying later");
		}
	}
}

void handleWake()
{
	Serial.println("Received wake request");
	WOL.sendMagicPacket(target_mac);
	server.send(200, "text/plain", "Magic packet sent to " + target_mac);
}

void setup()
{
	Serial.begin(115200);
	delay(1000);
	Serial.println("");

	if (!loadSettings())
	{
		while (true)
		{
			delay(1000);
		}
	}

	WOL.setRepeat(3, 100);
	server.on("/", handleWake);
	server.begin();
	checkWiFi();
}

void loop()
{
	checkWiFi();
	server.handleClient();
}

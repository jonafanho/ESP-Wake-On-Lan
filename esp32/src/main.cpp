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

String wifiSSID = "";
String wifiPassword = "";
String targetMAC = "";
unsigned long lastWiFiCheck = 0;

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
		wifiSSID = file.readStringUntil('\n');
	}
	if (file.available())
	{
		wifiPassword = file.readStringUntil('\n');
	}
	if (file.available())
	{
		targetMAC = file.readStringUntil('\n');
	}

	file.close();
	wifiSSID.trim();
	wifiPassword.trim();
	targetMAC.trim();

	if (wifiSSID.length() == 0 || targetMAC.length() == 0)
	{
		Serial.println("Settings file is empty or missing data!");
		return false;
	}

	Serial.println("SSID: " + wifiSSID);
	Serial.println("MAC:  " + targetMAC);
	return true;
}

void checkWiFi()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.print("Connecting to WiFi...");
		WiFi.mode(WIFI_STA);
		WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

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

String getISOTime()
{
	time_t now = time(nullptr);
	struct tm *timeinfo = gmtime(&now);
	char buffer[30];
	strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
	return String(buffer);
}

void handleRoot()
{
	String json = "{";
	json += "\"timestamp\":\"" + getISOTime() + "\"";
	json += "}";
	server.send(200, "application/json", json);
}

void handleWake()
{
	Serial.println("Received wake request");
	WOL.sendMagicPacket(targetMAC);
	String json = "{";
	json += "\"mac\":\"" + targetMAC + "\",";
	json += "\"timestamp\":\"" + getISOTime() + "\"";
	json += "}";
	server.send(200, "application/json", json);
}

void setup()
{
	Serial.begin(115200);
	delay(1000);
	Serial.println("");

	if (!loadSettings())
	{
		ESP.restart();
	}

	checkWiFi();

	if (WiFi.status() == WL_CONNECTED)
	{
		UDP.begin(9);
		configTime(0, 0, "pool.ntp.org", "time.nist.gov");
	}

	WOL.setRepeat(3, 100);
	server.on("/", handleRoot);
	server.on("/wake", handleWake);
	server.begin();
}

void loop()
{
	if (millis() - lastWiFiCheck > 10000)
	{
		checkWiFi();
		lastWiFiCheck = millis();
	}

	server.handleClient();
}

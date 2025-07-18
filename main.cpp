#include <Arduino.h>
#include "DHT.h"     // Incluye la librería DHT
#include <WiFi.h>    // Incluye la librería para Wi-Fi
#include <HTTPClient.h> // Para hacer peticiones HTTP
#include <Wire.h>    // Para comunicación I2C con la OLED
#include <Adafruit_GFX.h> // Librería gráfica para la OLED
#include <Adafruit_SH110X.h> // Librería específica para el controlador SH110X

// --- Definiciones para la pantalla OLED ---
#define I2C_ADDRESS 0x3C   // Dirección I2C típica para OLED (0x3C o 0x3D)
#define SCREEN_WIDTH 128   // Ancho de la pantalla OLED en píxeles
#define SCREEN_HEIGHT 64   // Alto de la pantalla OLED en píxeles
#define OLED_RESET -1      // No usar pin de reset (para I2C)

// Objeto de la pantalla OLED (¡GLOBAL!)
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Logo de Wi-Fi (Ejemplo 16x16 pixels - **REEMPLAZAR CON TU PROPIO BITMAP GENERADO**) ---
// Puedes usar herramientas online para generar un bitmap 16x16 de un icono de Wi-Fi.
// Este es un placeholder.
// array size is 32
static const byte Ionic_Ionicons_Wifi_1024[] PROGMEM  = {
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00
};
#define WIFI_LOGO_WIDTH 16
#define WIFI_LOGO_HEIGHT 16

// --- Definición de pines y tipo de sensor DHT ---
const int DHTPIN = 4;
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- Credenciales de la red WiFi ---
const char* ssid = ""; //nombre de la red wifi
const char* pass = ""; // ¡Revisa bien la contraseña!

// --- Definiciones para la conexión a Termux ---
const char* serverIp = ""; // IP de tu teléfono con Termux
const int serverPort = 5000;

// --- Configuración NTP para la hora en tiempo real ---
const char* ntpServer = "pool.ntp.org"; // Servidor NTP global
const long gmtOffset_sec = -5 * 3600; // GMT-5 para Colombia (5 horas * 3600 segundos/hora)
const int daylightOffset_sec = 0; // No hay horario de verano en Colombia

// --- Variables para el control de tiempo (sin usar delay() para el ciclo principal) ---
unsigned long previousMillis_DHT = 0;
const long interval_DHT = 5000; // Intervalo para lectura DHT y envío (5 segundos)
unsigned long previousMillis_NTP = 0;
const long interval_NTP = 30000; // Intervalo para sincronizar NTP (30 segundos, NTP no necesita ser cada segundo)


void setup() {
  Serial.begin(115200);
  Serial.println("------------------------------------");
  Serial.println("Iniciando ESP32...");
  Serial.println("------------------------------------");

  // --- Inicialización de la pantalla OLED ---
  if (!display.begin(I2C_ADDRESS, true)) {
    Serial.println(F("Error al iniciar la pantalla OLED. Verifica conexiones y dirección I2C."));
    while (true); // Detiene el programa si la OLED no inicia
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 0);
  display.println("Iniciando ESP32...");
  display.println("Conectando WiFi...");
  display.display();

  dht.begin();

  // --- Conexión WiFi ---
  Serial.print("Intentando conectar a la red WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 60) {
    delay(500);
    Serial.print(".");
    // Puedes actualizar la OLED aquí para mostrar puntos si quieres
    intentos++;
  }

  Serial.println("");

  display.clearDisplay();
  display.setCursor(0, 0);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("¡WiFi conectado exitosamente!");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());

    display.println("WiFi Conectado!");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.drawBitmap(SCREEN_WIDTH - WIFI_LOGO_WIDTH - 2, 0, Ionic_Ionicons_Wifi_1024, WIFI_LOGO_WIDTH, WIFI_LOGO_HEIGHT, SH110X_WHITE); // Dibuja el logo de WiFi en la esquina superior derecha
    display.display();
    delay(2000);

    // --- Configurar NTP una vez conectado a WiFi ---
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Hora NTP configurada.");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Sincronizando hora...");
    display.display();
    delay(2000); // Dar tiempo para la sincronización inicial
  } else {
    Serial.println("¡Error al conectar a la red WiFi!");
    Serial.println("Verifica el SSID y la contraseña.");

    display.println("Error WiFi!");
    display.println("Verifica cred.");
    display.display();
    // No hay NTP si no hay WiFi
  }

  Serial.println("------------------------------------");
  Serial.println("Iniciando lectura de sensor DHT...");
  Serial.print("Conectado DHT DATA a GPIO ");
  Serial.println(DHTPIN);
  Serial.print("Tipo de sensor: ");
  Serial.println(DHTTYPE == DHT11 ? "DHT11" : "DHT22");
  Serial.println("------------------------------------");

  display.clearDisplay();
  display.display();
}

void loop() {
  unsigned long currentMillis = millis(); // Obtiene el tiempo actual

  // --- Manejo de la reconexión WiFi ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Intentando reconectar...");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Reconectando WiFi...");
    display.display();

    WiFi.begin(ssid, pass);
    int reconect_intentos = 0;
    while (WiFi.status() != WL_CONNECTED && reconect_intentos < 40) {
      delay(500);
      Serial.print("*");
      // Puedes mostrar * en la OLED también si quieres
      reconect_intentos++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nReconectado!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("WiFi Reconectado!");
      display.setCursor(0, 1);
      display.println(WiFi.localIP());
      
      display.display();
      delay(2000);
      // Reconfigurar NTP al reconectar
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    } else {
      Serial.println("\nFallo en la reconexión. Esperando...");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Recon. Fallida!");
      display.setCursor(0, 1);
      display.println("No hay datos.");
      display.display();
      delay(1);
      return; // Salir del loop si no hay conexión
    }
  }

  // --- Actualizar y mostrar la hora NTP ---
  if (currentMillis - previousMillis_NTP >= interval_NTP) {
    previousMillis_NTP = currentMillis;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.print("Hora actual: ");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S"); // Formato largo para Serial

      // Esto actualiza la información de tiempo para el próximo ciclo.
      // No imprimimos en la OLED aquí directamente, se hace en la sección principal de actualización de la OLED.
    } else {
      Serial.println("Error al obtener la hora NTP.");
    }
  }

  // --- Lectura DHT y Envío de Datos (cada 5 segundos) ---
  if (currentMillis - previousMillis_DHT >= interval_DHT) {
    previousMillis_DHT = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("¡Error al leer del sensor DHT!");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Error DHT!");
      display.setCursor(0, 1);
      display.println("No hay datos.");
      display.display();
      return;
    }

    // --- Mostrar datos en el Monitor Serial ---
    Serial.print("Humedad: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperatura: ");
    Serial.print(t);
    Serial.println(" *C");

    // --- Enviar datos a Termux ---
    HTTPClient http;
    String serverPath = "http://" + String(serverIp) + ":" + String(serverPort) + "/dht_data";
    Serial.print("Enviando datos a: ");
    Serial.println(serverPath);

    http.begin(serverPath);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{";
    jsonPayload += "\"temperatura\":" + String(t) + ",";
    jsonPayload += "\"humedad\":" + String(h);
    jsonPayload += "}";

    Serial.print("Payload JSON: ");
    Serial.println(jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.printf("Código de Respuesta HTTP: %d\n", httpResponseCode);
      String responsePayload = http.getString();
      Serial.println("Respuesta del servidor Termux:");
      Serial.println(responsePayload);
    } else {
      Serial.printf("Error en la petición HTTP: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();

    // --- Mostrar datos y hora en la pantalla OLED ---
    display.clearDisplay();
    display.setCursor(0, 0);

    // Mostrar el logo de WiFi si está conectado
    if (WiFi.status() == WL_CONNECTED) {
      display.drawBitmap(SCREEN_WIDTH - WIFI_LOGO_WIDTH - 2, 0, Ionic_Ionicons_Wifi_1024, WIFI_LOGO_WIDTH, WIFI_LOGO_HEIGHT, SH110X_WHITE);
    }

    // Mostrar hora (tamaño de texto 1, debajo del logo o a la izquierda)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      display.setTextSize(1);
      display.setCursor(0, 0); // O donde quieras que empiece la hora
      char timeBuffer[9]; // Para HH:MM:SS
      strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
      display.print(timeBuffer);

      // Mostrar fecha si hay espacio
      // char dateBuffer[11]; // Para DD/MM/YYYY
      // strftime(dateBuffer, sizeof(dateBuffer), "%d/%m/%Y", &timeinfo);
      // display.setCursor(0, 10); // Otra posición
      // display.println(dateBuffer);
    } else {
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Sinc. hora...");
    }

    // Mostrar temperatura (tamaño de texto 2 para que sea más grande)
    display.setTextSize(1); // Texto más grande para los datos principales
    display.setCursor(0, 20); // Ajusta la posición Y para que no se superponga con la hora/logo
    display.print(t, 1);
    display.print((char)247); // Carácter de grado (°)
    display.println("C");

    // Mostrar humedad
    display.setCursor(0, 45); // Ajusta la posición Y
    display.print(h, 1);
    display.println(" %RH"); // %RH para humedad relativa

    display.display(); // ¡Actualiza la pantalla OLED!
  }
}

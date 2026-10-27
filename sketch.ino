/* LABORATORIO 2 - ESP32 + DHT22 + OLED + API Flask

   LEA PRIMERO LA SECCION "RECETA" EN setup().
   Las funciones tecnicas estan entregadas al final del archivo.
   No necesita editar su implementacion.
*/

#include <Arduino.h>
#include <math.h>
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// CONFIGURACION: cambie solo API_URL por la URL de su servicio Render.
const int PIN_DHT = 15;
const int PIN_SDA = 21;
const int PIN_SCL = 22;
const char* DISPOSITIVO = "doctorado-01";
const char* API_URL = "https://iot-lab2.onrender.com/mediciones";

// Objetos y tipo de datos preparados para el ejercicio.
DHTesp sensor;
Adafruit_SSD1306 pantalla(128, 64, &Wire, -1);

struct Medicion {
  int hora;
  float temperatura;
  float humedad;
};

// Declaraciones de las funciones entregadas al final del archivo.
Medicion simularHora(int hora, float temperaturaBase, float humedadBase);
void mostrarEnPantalla(int hora, float temperatura, float humedad);
void iniciarLote();
void agregarMedicion(int hora, float temperatura, float humedad);
int enviarLote();

// ================================================================
// RECETA PRINCIPAL: TRABAJE AQUI
// ================================================================

void setup() {
  // PREPARACION ENTREGADA: iniciar sensor, pantalla y WiFi.
  Serial.begin(115200);
  sensor.setup(PIN_DHT, DHTesp::DHT22);
  Wire.begin(PIN_SDA, PIN_SCL);
  pantalla.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  WiFi.begin("Wokwi-GUEST", "", 6);

  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi conectado");
  delay(2200); // Dar tiempo al DHT22 para entregar la primera lectura.

  // PASO 1. LEER EL SENSOR DHT22 (UNA SOLA VEZ).
  TempAndHumidity lectura = sensor.getTempAndHumidity();
  if (isnan(lectura.temperature) || isnan(lectura.humidity)) {
    Serial.println("Lectura invalida del DHT22");
    return;
  }
  float temperaturaBase = lectura.temperature;
  float humedadBase = lectura.humidity;
  Serial.print("Temperatura inicial: ");
  Serial.println(temperaturaBase);
  Serial.print("Humedad inicial: ");
  Serial.println(humedadBase);

  // PASO 2. CREAR UN LOTE VACIO ANTES DEL CICLO.
  iniciarLote();

  // PASO 3. PRODUCIR LAS 24 HORAS SIMULADAS (0, 1, ..., 23).
  for (int hora = 0; hora < 24; hora++) {
    Medicion dato = simularHora(hora, temperaturaBase, humedadBase);
    mostrarEnPantalla(dato.hora, dato.temperatura, dato.humedad);
    agregarMedicion(dato.hora, dato.temperatura, dato.humedad);
    delay(200);
  }

  // PASO 4. ENVIAR EL LOTE COMPLETO UNA SOLA VEZ.
  int codigo = enviarLote();
  Serial.print("Codigo HTTP final: ");
  Serial.println(codigo);
}

// setup() se ejecuta una vez. loop() no necesita repetir el envio.
void loop() {
  delay(1000);
}

// ================================================================
// FUNCIONES DE APOYO ENTREGADAS - NO ES NECESARIO MODIFICARLAS
// ================================================================

// Calcula una hora sintetica a partir de la lectura inicial del DHT22.
// Devuelve una Medicion con hora, temperatura y humedad.
Medicion simularHora(int hora, float temperaturaBase, float humedadBase) {
  float ciclo = sinf(2.0f * PI * (hora - 8) / 24.0f);
  Medicion dato;
  dato.hora = hora;
  dato.temperatura = temperaturaBase + 5.0f * ciclo;
  dato.humedad = constrain(humedadBase - 18.0f * ciclo, 0.0f, 100.0f);
  return dato;
}

// Presenta en pantalla los valores de una hora.
void mostrarEnPantalla(int hora, float temperatura, float humedad) {
  pantalla.clearDisplay();
  pantalla.setTextSize(1);
  pantalla.setTextColor(SSD1306_WHITE);
  pantalla.setCursor(0, 0);
  pantalla.println("DIA SIMULADO");
  pantalla.print("Hora: ");
  if (hora < 10) pantalla.print("0");
  pantalla.print(hora);
  pantalla.println(":00");
  pantalla.print("Temperatura: ");
  pantalla.print(temperatura, 1);
  pantalla.println(" C");
  pantalla.print("Humedad: ");
  pantalla.print(humedad, 1);
  pantalla.println(" %");
  pantalla.display();
}

// La variable cuerpoJSON va acumulando las mediciones antes del envio.
String cuerpoJSON = "";
int totalEnLote = 0;

// Abre un NUEVO lote y lo deja listo para recibir mediciones.
// Si se invoca dentro del for, borraria las mediciones anteriores.
void iniciarLote() {
  cuerpoJSON = String("{\"dispositivo\":\"") + DISPOSITIVO +
              "\",\"origen\":\"simulacion_dia\",\"mediciones\":[";
  totalEnLote = 0;
}

// Convierte UNA medicion a texto JSON. Se usa dentro de agregarMedicion().
String convertirAJSON(int hora, float temperatura, float humedad) {
  char textoHora[6];
  snprintf(textoHora, sizeof(textoHora), "%02d:00", hora);
  return String("{\"hora_simulada\":\"") + textoHora +
         "\",\"temperatura\":" + String(temperatura, 2) +
         ",\"humedad\":" + String(humedad, 2) + "}";
}

// Anade UNA medicion al lote abierto; conserva las anteriores.
void agregarMedicion(int hora, float temperatura, float humedad) {
  if (totalEnLote > 0) cuerpoJSON += ",";
  cuerpoJSON += convertirAJSON(hora, temperatura, humedad);
  totalEnLote++;
}

// Cierra el lote y lo envia mediante UN HTTP POST a la URL de Render.
// En esta demostracion se omite la validacion del certificado HTTPS.
// No use esta configuracion con datos sensibles o en produccion.
int enviarLote() {
  cuerpoJSON += "]}";
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sin conexion WiFi");
    return -1;
  }

  WiFiClientSecure cliente;
  cliente.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(20000);
  http.setTimeout(30000);
  if (!http.begin(cliente, API_URL)) return -2;

  http.addHeader("Content-Type", "application/json");
  Serial.print("Registros enviados: ");
  Serial.println(totalEnLote);
  int codigo = http.POST(cuerpoJSON);
  Serial.print("HTTP: ");
  Serial.println(codigo);
  if (codigo < 0) Serial.println(http.errorToString(codigo));
  else Serial.println(http.getString());
  http.end();
  return codigo;
}

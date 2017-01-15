/*
 * Ejemplo que intenta extablecer tanto una conexión WIFI como MQTT, realizando autoreconexiones 
 * cuando es necesario.
 * 
 * Tiene la problematica de utilizar delay para temporizacion de la monitorizacion del estado de la WIFI.
 * 
 * Continuamente escanea las redes WIFI cada WIFI_RECONNECT_TIME milisegunos y cuando encuentra el 
 * SSID que espera se conecta a él. Si se desconecta por algún motivo vuelve a scanear.
 * Mientras estemos conectados a la WIFI se mantiene una conexión con un servidor MQTT, que se auto 
 * reconecta ante perdidas de conexión.
 * Por otro lado creo dos timers de prueba. Uno se utiliza para indicar mediante el ritmo del parpadeo 
 * del led si estamos conectados a la WIFI o estamos scaneando, variando el tiempo del timer. 
 * El segundo timer no lo utilizo
 */
 
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <PubSubClient.h>

#define WIFI_RECONNECT_TIME 30000
#define MQTT_RECONNECT_TIME 30000
//#define WIFI_BEGIN_TIME 500
//#define MAX_WIFI_BEGIN_TIME 500 * 20

const char* ssid     = "WLAN_41E0";
const char* password = "JOWP1IJEV96NLA83RGFQ";
IPAddress MQTTserver(192, 168, 1, 38);
int MQTTport = 1883;

Ticker Ticker1;
Ticker Ticker2;

boolean led_state = false;

int lastWIFIStatus = 0;
long lastMQTTReconnectAttempt = 0;
long lastWIFIReconnectAttempt = 0;
long lastWIFIbeginAttempt = 0;
long firstWIFIbeginAttempt = 0;

WiFiClient espClient;
PubSubClient client(espClient);



void setup() {
  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  lastWIFIStatus = WiFi.status();

  client.setServer(MQTTserver, MQTTport);
  client.setCallback(MQTTcallback);

  lastMQTTReconnectAttempt = 0;
  lastWIFIReconnectAttempt = 0;


  Ticker1.attach(0.05, timer1);
  Ticker2.attach(5.0, timer2);

  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  Serial.println("Setup done");
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

void timer2()
{

}

void timer1()
{        
    if(led_state == true){
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
     led_state = false; 
    }else{
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      led_state = true;
    }

}


void loop() {
  static boolean first = true;
  static boolean firstMQTT = true;
  if(WiFi.status() != WL_CONNECTED){
      long now = millis();
      if ((now - lastWIFIReconnectAttempt > WIFI_RECONNECT_TIME) || first == true) {
        first=false;
        if(lastWIFIStatus == WL_CONNECTED){
          Serial.print("Perdida la conexion WIFI con ");
          Serial.println(ssid);
          Ticker1.attach(0.05, timer1);
          lastWIFIStatus = 0;
        }
        lastWIFIReconnectAttempt = now;
        // Attempt to reconnect
        if (WIFIreconnect()) {
          lastWIFIReconnectAttempt = 0;
          lastWIFIStatus = WL_CONNECTED;
        }
      }
  }else{
    if(!client.connected()){
      long now = millis();
      if (now - lastMQTTReconnectAttempt > MQTT_RECONNECT_TIME || firstMQTT == true) {
        firstMQTT = false;
        lastMQTTReconnectAttempt = now;
        // Attempt to reconnect
        if (MQTTreconnect()) {
          lastMQTTReconnectAttempt = 0;
        }
      }
    }else{
        //Client connected
        client.loop();
    }
  }
}

boolean WIFIreconnect() {
  //Serial.println("WIFIreconnect");
  WifiConnect();
  return (WiFi.status() == WL_CONNECTED);
}

boolean WifiConnect() {
  boolean conectado = false;
    Ticker1.attach(0.05, timer1);
  
    Serial.print("Buscando Redes WIFI Para conectar a ");
    Serial.println(ssid);
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    if(n==0)
      Serial.println("No existen Redes WIFI proximas");

    for (int i = 0; i < n; ++i)
    {
      if(strcmp(WiFi.SSID(i).c_str(), ssid) == 0){
        Serial.print(ssid);
        Serial.println(" Encontrada.");
        WiFi.begin(ssid, password);
        int count =0;
        while (WiFi.status() != WL_CONNECTED && count < 20) {
          int status = WiFi.status();
          Serial.print("Wifi Status ");
          Serial.println(status);
          count++;
          delay(500);
        }
        if(WiFi.status() != WL_CONNECTED){
          WiFi.disconnect();
          conectado = false;
          Serial.print("Error al intental conectar a ");
          Serial.println(ssid);
        }else{
          conectado= true;
          Ticker1.attach(2.0, timer1);
          Serial.print("Conectado a ");
          Serial.println(ssid);
        }
        
      }else{
        Serial.print(ssid);
        Serial.println(" No Encontrada.");
      }
    }
      
    return conectado;
}


boolean MQTTreconnect() {
  Serial.println("Intentando conectar MQTT");
  if (client.connect("arduinoClient")) {
    // Once connected, publish an announcement...
    client.publish("outTopic","hello world");
    // ... and resubscribe
    client.subscribe("inTopic");
  }
  
  boolean state = client.connected();
  if(state){
    Serial.println("Conectado con MQTT");
  }else{
    Serial.println("NO Conectado con MQTT");
  }
  
  return state;
}




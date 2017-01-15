/*
 * Ejemplo que intenta extablecer tanto una conexión WIFI como MQTT, realizando autoreconexiones 
 * cuando es necesario.
 * 
 * No existe ningún delay dentro del loop para poder relizar tareas criticas dentro del loop sin 
 * perjuicio de los delays
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
#define WIFI_BEGIN_TIME 500
#define MAX_WIFI_BEGIN_TIME 500 * 20


const char* ssid     = "WLAN_41E0";
const char* password = "JOWP1IJEV96NLA83RGFQ";
IPAddress MQTTserver(192, 168, 1, 38);
int MQTTport = 1883;


Ticker Ticker1;
Ticker Ticker2;

boolean led_state = false;

int lastWIFIStatus = 0;
int WifiConnect_state = 0;

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
  if(WiFi.status() != WL_CONNECTED && WifiConnect_state == 0){
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
        
        WifiConnect_state = _WifiConnect(WifiConnect_state);
        if(WifiConnect_state == 3){
          lastWIFIReconnectAttempt = 0;
          lastWIFIStatus = WL_CONNECTED;
          WifiConnect_state = 0;
        }else if(WifiConnect_state == -1){
          WifiConnect_state = 0;
          lastWIFIStatus = 0;
        }
      }
  }else if(WifiConnect_state == 1 || WifiConnect_state == 2 ){
    WifiConnect_state = _WifiConnect(WifiConnect_state);
    if(WifiConnect_state == 3){
      lastWIFIReconnectAttempt = 0;
      lastWIFIStatus = WL_CONNECTED;
      WifiConnect_state = 0;
    }else if(WifiConnect_state == -1){
      WifiConnect_state = 0;
      lastWIFIStatus = 0;
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


int _WifiConnect(int state) {
  static int n = 0;
  int ret=-1;

    switch (state) {
      case 0:
      {
            Serial.print("Buscando Redes WIFI Para conectar a ");
            Serial.println(ssid);
            Ticker1.attach(0.05, timer1);            
            n = WiFi.scanNetworks();
            if(n > 0)
              ret=1;
            else{
              Serial.println("No existen Redes WIFI proximas");
              ret=-1;
            }
        break;
      }
      case 1:
      {
            if(n != 0){
              if(strcmp(WiFi.SSID(n-1).c_str(), ssid) == 0){
                Serial.print(ssid);
                Serial.println(" Encontrada.");
                n = 0;
                WiFi.begin(ssid, password);
                firstWIFIbeginAttempt = millis();
                ret = 2;
              }else{
                --n;
                if(n > 0)
                  ret = 1;
                else{
                  ret = -1;
                  Serial.print(ssid);
                  Serial.println(" No Encontrada.");
                }
              }
            }
        break;
      }
      case 2:
      {
            long now = millis();
            if (now - lastWIFIbeginAttempt > WIFI_BEGIN_TIME) {
              lastWIFIbeginAttempt = now;
              // Attempt to check WIFI Status
              
              int status = WiFi.status();
              Serial.print("Wifi Status ");
              Serial.println(status);
              if(status == WL_CONNECTED){
                lastWIFIbeginAttempt = 0;
                firstWIFIbeginAttempt = 0;
                Ticker1.attach(2.0, timer1);
                ret = 3;
                Serial.print("Conectado a ");
                Serial.println(ssid);
              }else{
                if(now - firstWIFIbeginAttempt > MAX_WIFI_BEGIN_TIME){
                  lastWIFIbeginAttempt = 0;
                  firstWIFIbeginAttempt = 0;
                  WiFi.disconnect();
                  ret = -1;
                  Serial.print("Error al intental conectar a ");
                  Serial.println(ssid);
                }else{
                  ret = 2;
                }
              }
            }else{
              ret = 2;
            }
        break;    
      }
      default:
      {
            ret = -1;
      }
    }
    return ret;
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




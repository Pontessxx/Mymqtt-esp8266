#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid =  "HPMP-2G"; // ssid / nome da rede WI-FI que deseja se conectar
const char* password =  "97531HeMpe"; // Senha da rede WI-FI que deseja se conectar
const char* BROKER_MQTT = "46.17.108.113"; // URL do broker MQTT que deseja utilizar
const int BROKER_PORT = 1883; // Porta do Broker MQTT

const char* TOPICO_SUBSCRIBE = "/TEF/lamp108/cmd"; // tópico MQTT de escuta
const char* TOPICO_PUBLISH = "/TEF/lamp108/attrs"; // tópico MQTT de envio de informações para Broker
const char* TOPICO_PUBLISH_2 = "/TEF/lamp108/attrs/l"; // tópico MQTT de envio de informações para Broker
const char* ID_MQTT = "fiware_108"; // ID MQTT (para identificação de sessão)

//int D2 = 4; // Alterado para o pino D2 (GPIO 4) no ESP8266

WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
char EstadoSaida = '0'; // variável que armazena o estado atual da saída

void initSerial() {
  Serial.begin(115200);
}

void initWiFi() {
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(ssid);
  Serial.println("Aguarde");
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(ssid);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }
  
  Serial.print("- Mensagem recebida: ");
  Serial.println(msg);
  
  if (msg.equals("lamp108@on|")) {
    digitalWrite(D2, LOW);
    EstadoSaida = '1';
  }
  
  if (msg.equals("lamp108@off|")) {
    digitalWrite(D2, HIGH);
    EstadoSaida = '0';
  }
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPICO_SUBSCRIBE);
    } else {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Haverá nova tentativa de conexão em 2s");
      delay(2000);
    }
  }
}

void reconectWiFi() {
  if (WiFi.status() == WL_CONNECTED)
    return;
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(ssid);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void VerificaConexoesWiFIEMQTT() {
  if (!MQTT.connected())
    reconnectMQTT();
  
  reconectWiFi();
}

void EnviaEstadoOutputMQTT() {
  if (EstadoSaida == '1') {      
    MQTT.publish(TOPICO_PUBLISH, "s|on");
    Serial.println("- Led Ligado");
  }
  
  if (EstadoSaida == '0') {       
    MQTT.publish(TOPICO_PUBLISH, "s|off");
    Serial.println("- Led Desligado");
  }
  
  Serial.println("- Estado do LED onboard enviado ao broker!");
  delay(1000);
}

void InitOutput() {
  pinMode(D2, OUTPUT);
  digitalWrite(D2, HIGH);
  boolean toggle = false;
  
  for (int i = 0; i <= 10; i++) {
    toggle = !toggle;
    digitalWrite(D2, toggle);
    delay(200);
  }
  
  digitalWrite(D2, LOW);
}

void setup() {
  InitOutput();
  initSerial();
  initWiFi();
  initMQTT();
  delay(5000);
  MQTT.publish(TOPICO_PUBLISH, "s|off");
}

void loop() {
  const int potPin = A0;
  char msgBuffer[4];
  
  VerificaConexoesWiFIEMQTT();
  EnviaEstadoOutputMQTT();
  
  int sensorValue = analogRead(potPin);
  float voltage = sensorValue * (3.3 / 1024.0);
  float luminosity = map(sensorValue, 0, 1023, 0, 100);
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print("V - ");
  Serial.print("Luminosity: ");
  Serial.print(luminosity);
  Serial.println("%");
  dtostrf(luminosity, 4, 2, msgBuffer);
  MQTT.publish(TOPICO_PUBLISH_2, msgBuffer);
  
  MQTT.loop();
}

#define SSID "OnePlus 7T"
#define PASSWORD "qwertasdf"

void Wifi_init(){
  WiFi.begin(SSID, PASSWORD);  //Initialize WiFi Object

  Serial.print("Connecting to ");
  Serial.println(SSID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address : ");
  Serial.print(WiFi.localIP());
}
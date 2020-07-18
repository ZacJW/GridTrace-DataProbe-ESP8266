#include <ESP8266WiFi.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include "hardware_adapter_ABC.h"
#include "config.h"

const char* ssid = STASSID;
const char* password = STAPSK;

#ifdef HTTPS_SERVER
#include "cert_and_key.h"
BearSSL::ESP8266WebServerSecure server(443);
Hardware_Adapter_ABC<WiFiServerSecure> *hw_adapter =  new Hardware_Adapter<WiFiServerSecure>(&server);
#else
ESP8266WebServer server(80);
Hardware_Adapter_ABC<WiFiServer> *hw_adapter = new Hardware_Adapter<WiFiServer>(&server);
#endif
void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){delay(1);}
  hw_adapter->init();
  #ifdef HTTPS_SERVER
  server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
  #endif
  server.on("/description.xml", HTTP_GET, []() {
    SSDP.schema(server.client());
  });
  hw_adapter->register_routes();
  server.begin();
  
  SSDP.setSchemaURL("description.xml");
  #ifdef HTTPS_SERVER
  SSDP.setHTTPPort(443);
  #else
  SSDP.setHTTPPort(80);
  #endif
  SSDP.setName("GridSense DataProbe");
  SSDP.setURL(DATA_ROUTE);
  SSDP.setModelName("GridSense DataProbe V0.1");
  SSDP.setDeviceType("urn:grid-sense:service:data-probe:1");
  SSDP.begin();
}

void loop() {
  server.handleClient();
  hw_adapter->loop();
  delay(1);
}


// All hardware adapters must define a class template that is dervived from
// the Hardware_Adapter_ABC class template. The class must define a constructor
// that takes an ESP8266WebServerTemplate<T> pointer to pass to
// Hardware_Adapter_ABC<T> through the initializer list.
// The class template must be either named or aliased Hardware_Adapter
// The hardware adapter header should also define DATA_ROUTE to be the URL
// to request to get json format data from the connected hardware.

#ifndef _HARDWARE_ADAPTER_ABC_H_
#define _HARDWARE_ADAPTER_ABC_H_

template <class T>
class Hardware_Adapter_ABC{
  protected:
    esp8266webserver::ESP8266WebServerTemplate<T> *server;
  public:
    Hardware_Adapter_ABC(esp8266webserver::ESP8266WebServerTemplate<T> *server) : server(server){}

    // An optional method that will be called once during setup to initialise
    // the hardware adapter. URL routes should not be registered here.
    virtual void init(){}

    // The method called once expected to register hander functions on the
    // server for all URLs the server should respond to.
    virtual void register_routes() = 0;

    // An optional method that will be called repeatedly between handling
    // server requests. To be used if the hardware requires continous
    // communication beyond just when the ESP receives a request.
    virtual void loop(){} // Optional overload
};

#endif

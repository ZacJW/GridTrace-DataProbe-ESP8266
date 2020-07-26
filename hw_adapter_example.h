// ----- Hardware Adapter Example ----- \\
//
// All hardware adapters must define a class template that is dervived from
// the Hardware_Adapter_ABC class template. The class must define a constructor
// that takes an ESP8266WebServerTemplate<T> pointer to pass to
// Hardware_Adapter_ABC<T> through the initializer list.
// The class template must be either named or aliased Hardware_Adapter
// The hardware adapter header should also define DATA_ROUTE to be the URL
// to request to get json format data from the connected hardware.


#ifndef _HW_AD_EXAMPLE_H_
#define _HW_AD_EXAMPLE_H_

#define DATA_ROUTE "data.json"

template <class T>
class Hardware_Adapter_Example : public Hardware_Adapter_ABC<T>{
  public:
    Hardware_Adapter_Example(esp8266webserver::ESP8266WebServerTemplate<T> *server) : Hardware_Adapter_ABC<T>(server){
      
    }

    // This data request handler just generates some random cell voltage data.
    void get_data(){
      char message[256] = {0};
      snprintf(message, 255, "{"
                                 "\"cell_voltage\" : {"
                                     "\"columns\" : [{\"name\" : \"cell_id\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"millivolts\", \"type\" : \"integer\"}],"
                                     "\"values\" : [[0, %d], [1, %d], [2, %d]]"
                                 "}"
                             "}",
      random(3200,4200), random(3200,4200), random(3200,4200));
      
      this->server->send(200, "application/json", message);
    }
  
    void register_routes(){
      this->server->on("/"DATA_ROUTE, HTTP_GET, std::bind(&Hardware_Adapter_Example::get_data, this));
    }
};

// Alias Hardware_Adapter_Example to be accessible as the name Hardware_Adapter.
template <class T>
using Hardware_Adapter = Hardware_Adapter_Example<T>;

#endif

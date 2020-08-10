#ifndef _HW_AD_BMS_H_
#define _HW_AD_BMS_H_

#define DATA_ROUTE "data.json"

uint8_t request_basic[] = {0xdd, 0xa5, 0x03, 0x00, 0xff, 0xfd, 0x77};
uint8_t request_voltage[] = {0xdd, 0xa5, 0x04, 0x00, 0xff, 0xfc, 0x77};

template <class T>
class Hardware_Adapter_BMS : public Hardware_Adapter_ABC<T>{
  private:
    static void purge_serial(){
      delay(2);
      while(Serial.available()){
        Serial.read();
        delay(2);
      }
    }

    static int16_t calculate_checksum(uint8_t *response_pre, uint8_t *response_data){
      int16_t checksum = - response_pre[2] - response_pre[3];
      for (int i = 0; i < response_pre[3]; i++){
        checksum -= response_data[i];
      }
      return checksum;
    }

    static bool make_request(uint8_t* request, size_t request_length, 
                             uint8_t* response_pre, uint8_t*& response_data, uint8_t* response_post){
      int tries_remaining;
      for (tries_remaining = 5; tries_remaining; tries_remaining--){
        Serial.write(request, request_length);
        Serial.flush();
        size_t readCount = Serial.readBytes(response_pre, 4);
        if (readCount < 4 || response_pre[2] == 0x80  || response_pre[3] < 23 || response_pre[3] > 27){
          // BMS timed out or returned error status or data content too small/big, so purge serial buffer and retry
          purge_serial();
          continue;
        }
        response_data = new uint8_t[response_pre[3]];
        readCount = Serial.readBytes(response_data, response_pre[3]);
        if (readCount < response_pre[3]){
          // BMS timed out so purge serial buffer, deallocate response_data and retry
          purge_serial();
          delete[] response_data;
          continue;
        }

        readCount = Serial.readBytes(response_post, 3);
        if (readCount < 3){
          // BMS timed out so purge serial buffer, deallocate response_data and retry
          purge_serial();
          delete[] response_data;
          continue;
        }
        int16_t checksum_received = ((int16_t) response_post[0]) << 8 + ((int16_t) response_post[1]);
        
        if (checksum_received == calculate_checksum(response_pre, response_data)){
          break;
        }
        delete[] response_data;
        response_data = nullptr;
        purge_serial();
      }
      return tries_remaining;
    }
    
  public:
    Hardware_Adapter_BMS(esp8266webserver::ESP8266WebServerTemplate<T> *server) : Hardware_Adapter_ABC<T>(server){
      
    }

    void init(){
      Serial.begin(9600);
      Serial.setTimeout(10);
    }

    void get_data(){
      uint8_t response_pre[4] = {0};
      uint8_t *response_data;
      uint8_t response_post[3] = {0};
      if (!make_request(request_basic, sizeof(request_basic), response_pre, response_data, response_post)){
        this->server->send(502, "text/plain", "BMS response error");
        return;
      }
      uint16_t battery_voltage = (((uint16_t) response_data[0]) << 8) + (uint16_t) response_data[1];
      uint16_t battery_current = (((uint16_t) response_data[2]) << 8) + (uint16_t) response_data[3];
      uint16_t residual_cap  = (((uint16_t) response_data[4]) << 8) + (uint16_t) response_data[5];
      uint16_t nominal_cap = (((uint16_t) response_data[6]) << 8) + (uint16_t) response_data[7];
      uint16_t cycle_count = (((uint16_t) response_data[8]) << 8) + (uint16_t) response_data[9];
      
      uint16_t balance_state_low = (((uint16_t) response_data[12]) << 8) + (uint16_t) response_data[13];
      uint16_t balance_state_high = (((uint16_t) response_data[14]) << 8) + (uint16_t) response_data[15];
      uint16_t protection_state = (((uint16_t) response_data[16]) << 8) + (uint16_t) response_data[17];

      uint8_t rsoc = response_data[19];
      uint8_t mosfet_status = response_data[20];
      uint8_t cell_count = response_data[21];
      uint8_t ntc_count = response_data[22];
      int16_t ntc_readings[2] = {0};
      for (int i = 0; i < ntc_count && (24 + 2*i < response_pre[3]); i++){
        ntc_readings[i] = (((int16_t) response_data[23 + 2*i]) << 8) + (int16_t) response_data[24 + 2*i];
      }
      delete[] response_data;
      
      char ntc_string[16] = {0};
      char *ntc_end = ntc_string;
      if (ntc_readings[0] == 0){
        strcpy(ntc_end, "null, ");
        ntc_end += 6;
      }else{
        ntc_end += snprintf(ntc_end, 15 - (ntc_end - ntc_string), "%d, ", ntc_readings[0]);
      }
      if (ntc_readings[1] == 0){
        strcpy(ntc_end, "null");
      }else{
        snprintf(ntc_end, 15 - (ntc_end - ntc_string), "%d", ntc_readings[1]);
      }

      memset(response_pre, 0, 4);
      memset(response_post, 0, 3);
      if (!make_request(request_voltage, sizeof(request_voltage), response_pre, response_data, response_post) || (response_pre[3] % 2)){
        this->server->send(502, "text/plain", "BMS response error");
        return;
      }
      char voltages[512] = {0};
      uint16_t voltages_offset = 0;
      for (int i = 0; i*2  < response_pre[3]; i++){
        if (i){
          strcpy(voltages+voltages_offset, ", ");
          voltages_offset += 2;
        }
        uint16_t v = (((uint16_t) response_data[2*i]) << 8) + (uint16_t) response_data[2*i + 1];
        voltages_offset += snprintf(voltages + voltages_offset, 511 - voltages_offset, "[%d, %d]", i, v);
      }
      char message[2048] = {0};
      snprintf(message, 2047, "{"
                                 "\"battery\" : {"
                                     "\"columns\" : [{\"name\" : \"millivolts\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"milliamps\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"residual_milliamphours\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"nominal_milliamphours\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"cycle_count\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"balance_state_low\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"balance_state_high\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"protection_state\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"rsoc\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"mosfet_status\", \"type\" : \"integer\"}],"
                                                    "{\"name\" : \"bms_temperature\", \"type\" : \"integer\"}],"
                                                    "{\"name\" : \"battery_temperature\", \"type\" : \"integer\"}],"
                                     "\"values\" : [[%d0, %d0, %d0, %d0, %d, %d, %d, %d, %d, %d, %s]]"
                                 "},"
                                 "\"cell_voltage\" : {"
                                     "\"columns\" : [{\"name\" : \"cell_id\", \"type\" : \"integer\"},"
                                                    "{\"name\" : \"millivolts\", \"type\" : \"integer\"}],"
                                     "\"values\" : [%s]"
                                 "}"
                             "}",
      battery_voltage, battery_current, residual_cap, 
      nominal_cap, cycle_count, balance_state_low,
      balance_state_high, protection_state, rsoc,
      mosfet_status, ntc_string, voltages);
      
      this->server->send(200, "application/json", message);
    }
  
    void register_routes(){
      this->server->on("/"DATA_ROUTE, HTTP_GET, std::bind(&Hardware_Adapter_BMS::get_data, this));
    }
};

// Alias Hardware_Adapter_Example to be accessible as the name Hardware_Adapter.
template <class T>
using Hardware_Adapter = Hardware_Adapter_BMS<T>;


#endif

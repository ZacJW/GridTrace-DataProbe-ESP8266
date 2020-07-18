#ifndef _CONFIG_H_
#define _CONFIG_H_

// Set your WiFi access point's SSID and PSK in wifi_details.h
#include "wifi_details.h"

// If you would like the ESP to communicate using HTTPS uncomment the following 
// define and add your certificate and private key to cert_and_key.h

// #define HTTPS_SERVER


// ----- Hardware Adapters ----- \\

// Uncommenting one of the following hardware adapters configures the ESP to communicate
// with and pull data from various pieces of equipment.
// Only uncomment one hardware adapter include as more than one will cause compile errors.

// An example hardware adapter demonstarating how one should be written.
#include "hw_adapter_example.h"

#endif


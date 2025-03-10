#include <iostream>
#include <string>
#include <vector>
#include <ifaddrs.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
// #define _DEFAULT_SOURCE

std::vector<std::string> get_interface_list() {
  struct ifaddrs *interfaces;
  std::vector<std::string> interfaceList;

  if(getifaddrs(&interfaces) == 0) {
    // Loop through linked list of interfaces
    struct ifaddrs *interface; 
    for(interface = interfaces; interface != nullptr; interface = interface->ifa_next) {
      interfaceList.push_back(interface->ifa_name);
    }
  }

  return interfaceList;
}

bool is_wifi(std::string interface_name) {
  return interface_name.find("wlan") != std::string::npos 
         || interface_name.find("wlp1s0") != std::string::npos;
}

std::string get_interface_address(std::string interface) {
  struct ifaddrs *interfaces; 
  getifaddrs(&interfaces);

  for(struct ifaddrs *ifa=interfaces; ifa!=NULL; ifa=ifa->ifa_next) {
    if(ifa->ifa_addr && interface == ifa->ifa_name) {
        // struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
        // return inet_ntoa(sin->sin_addr);
        // ifa->ifa_addr
        printf("INET_NTOA");
        return inet_ntoa(((struct sockaddr_in*)ifa->ifa_addr)->sin_addr);  
        }
    }
    return "";
}

int main() {

  std::vector<std::string> interfaces = get_interface_list();

  std::string wifiInterface;

  for(std::string interface : interfaces) {
    if(is_wifi(interface)) {
      wifiInterface = interface;
      break;
    }
  }

  std::string address = get_interface_address(wifiInterface);

  std::cout << "WiFi IP Address: " << address << std::endl;

  return 0;
}

// #include <string> 
// #include <ifaddrs.h>
// #include <net/if.h>
// #include "iostream"
// #include "vector"

// std::vector<std::string> get_interface_list() {
//   struct ifaddrs *interfaces;
//   std::vector<std::string> interfaceList;

//   if(getifaddrs(&interfaces) == 0) {
//     // Loop through linked list of interfaces
//     struct ifaddrs *interface; 
//     for(interface = interfaces; interface != nullptr; interface = interface->ifa_next) {
//       interfaceList.push_back(interface->ifa_name);
//     }
//   }
//   return interfaceList;
// }
 
// bool is_wifi(std::string interface_name) {
//   return interface_name.find("wlan") != std::string::npos 
//          || interface_name.find("wif") != std::string::npos;
// }

// std::string get_wifi_ip() {
//   auto interfaces = get_interface_list();

//   for(auto& interface : interfaces) {
//     if(is_wifi(interface)) {
//         return interface;
//     } 
//   }
// }

// int main(){
//     std::cout << get_wifi_ip();
// }
// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
 typedef std::map<extent_protocol::extentid_t,std::string> Content_Map;
 typedef std::map<extent_protocol::extentid_t,extent_protocol::attr> Att_Map;
class extent_server {

 public:
  extent_server();

  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
 private:
  Content_Map content_map;
  Att_Map att_map;
  pthread_mutex_t map_m;
};

#endif








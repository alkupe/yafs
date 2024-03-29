// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"


struct extent_data {
    extent_data();
    bool dirty;
    bool removed;
    bool extent_cached;
    extent_protocol::attr attr;
    std::string buf;
};

typedef std::map<extent_protocol::extentid_t,extent_data> Extent_Map;

class extent_client {
 private:
  rpcc *cl;
  Extent_Map extent_map;
  pthread_mutex_t _extent_mutex;
 public:
  extent_client(std::string dst);
  ~extent_client();
  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status flush(extent_protocol::extentid_t eid);
};

#endif 


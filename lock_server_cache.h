#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <queue>
#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"
#include "handle.h"

struct client_data {

  enum lock_status {FREE,LOCKED,PROMISED};
  lock_status state;
  std::string owner;
  std::string next;
  std::queue< std::string > waiting_list;
  pthread_mutex_t cond_mutex;
  pthread_cond_t cv_;
  client_data(std::string);
};

typedef std::map<lock_protocol::lockid_t,client_data> LockMap;

class lock_server_cache {
 private:
  int nacquire;
  LockMap lock_map;
  pthread_mutex_t lookup_mutex;

 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif

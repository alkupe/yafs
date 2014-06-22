// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc/rpc.h"
#include <map>

class lock_server {

 protected:
  int nacquire;

 public:
  lock_server();
  lock_protocol::status grant(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
 private:
   std::map<lock_protocol::lockid_t,bool> request_map;
   pthread_mutex_t request_mutex;
   pthread_cond_t cv_;

};

#endif








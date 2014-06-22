// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
class extent_client;
// Classes that inherit lock_release_user can override dorelease so that
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
  private:
    extent_client* _ec;
 public:
  lock_release_user(extent_client *ec);
  virtual void dorelease(lock_protocol::lockid_t);
  virtual ~lock_release_user() {};
};

struct lock_state {

  enum lock_status {NONE,FREE,LOCKED,ACQUIRING,RELEASING};
  lock_status state;
  lock_state();
  pthread_cond_t cv_;
  pthread_mutex_t cond_mutex;
};



class lock_client_cache : public lock_client {
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;
  pthread_mutex_t lookup_mutex;
  struct timespec timeout;
  std::map<lock_protocol::lockid_t,lock_state> lock_map;
  rpcs *rlsrpc;
 public:
  lock_client_cache(std::string xdst, lock_release_user *l = 0);
  virtual ~lock_client_cache();
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t,
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t,
                                       int &);
};


#endif

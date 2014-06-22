// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"




client_data::client_data(std::string dst)
: state(FREE),owner(dst),next(),waiting_list(),cond_mutex(),cv_()
{}

lock_server_cache::lock_server_cache()
:lock_map(),lookup_mutex()
{
}



int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id,
                               int &)
{
  lock_protocol::status ret = lock_protocol::OK;
  rpcc *cl;
  rpcc *clw;
  pthread_mutex_lock(&lookup_mutex);
  LockMap::iterator iter = lock_map.find(lid);

  int state;
  if (iter == lock_map.end()) {
    client_data cd(id);

    lock_map.insert(std::make_pair(lid,cd));
    iter = lock_map.find(lid);
  }
  pthread_mutex_unlock(&lookup_mutex);
  ScopedLock sl(&iter->second.cond_mutex);

  if (iter->second.state == client_data::FREE) {
    iter->second.state = client_data::LOCKED;
    iter->second.owner = id;
    return ret;
  } else if (iter->second.state == client_data::LOCKED && (iter->second.next.empty() || iter->second.next == id)) {
    iter->second.next = id;
    pthread_mutex_unlock(&iter->second.cond_mutex);
    handle hc(iter->second.owner);
    cl = hc.safebind();
    if (!cl) {
      return lock_protocol::IOERR;
    }
    cl->call(rlock_protocol::revoke,lid,state);
    pthread_mutex_lock(&iter->second.cond_mutex);
    while (iter->second.state != client_data::PROMISED) {
      pthread_cond_wait(&iter->second.cv_,&iter->second.cond_mutex);
    }
    iter->second.state = client_data::LOCKED;
    iter->second.owner = id;

    if (iter->second.waiting_list.empty()) {
      iter->second.next.clear();
    } else {
      iter->second.next = iter->second.waiting_list.front();
      iter->second.waiting_list.pop();
      handle hw(iter->second.next);
      clw = hw.safebind();
      if (!clw) {
        return lock_protocol::IOERR;
      }
      clw->call(rlock_protocol::retry,lid,state);

    }
    return lock_protocol::OK;
  }
  if ((iter->second.state == client_data::PROMISED && iter->second.next != id) ||
      (iter->second.state == client_data::LOCKED && !(iter->second.next.empty()) )) {
      iter->second.waiting_list.push(id);

  }
  return lock_protocol::RETRY;
}

int
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id,
         int &r)
{
  pthread_mutex_lock(&lookup_mutex);
  LockMap::iterator iter = lock_map.find(lid);
  pthread_mutex_unlock(&lookup_mutex);
  ScopedLock sl(&iter->second.cond_mutex);
  if (!iter->second.next.empty()) {
    iter->second.state = client_data::PROMISED;
    pthread_cond_signal(&iter->second.cv_);
  } else {
    iter->second.state = client_data::FREE;
  }

  lock_protocol::status ret = lock_protocol::OK;
  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}


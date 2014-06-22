// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
lock_server::lock_server():
  nacquire (0),request_map(),request_mutex(), cv_()
{
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::grant(int clt, lock_protocol::lockid_t lid, int & r) {
  printf("grant request from clt %d\n", clt);
  ScopedLock sl(&request_mutex);
  lock_protocol::status ret = lock_protocol::OK;

  std::map<lock_protocol::lockid_t,bool>::iterator iter = request_map.find(lid);

  if (iter == request_map.end()) {
    //new lid
    request_map.insert(std::make_pair(lid,true));

  } else {

    while(iter->second) {
      VERIFY(pthread_cond_wait(&cv_,&request_mutex) == 0);
    }
    //if here, the lock is free, so thread can take it
    iter->second = true;
  }
  r = nacquire;
  return ret;
}


lock_protocol::status lock_server::release(int clt, lock_protocol::lockid_t lid, int& r) {
  printf("release request from clt %d\n", clt);
  ScopedLock sl(&request_mutex);
  std::map<lock_protocol::lockid_t,bool>::iterator iter = request_map.find(lid);


  //we can trust that the client is only going to release locks it actually has:
  VERIFY(iter != request_map.end());
  VERIFY(iter->second);
  //release the lock:
  iter->second = false;
  VERIFY(pthread_cond_broadcast(&cv_) == 0);
  r = nacquire;
  return lock_protocol::OK;
}



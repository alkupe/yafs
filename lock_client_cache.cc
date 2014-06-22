// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "extent_client.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"




  lock_state::lock_state()
  :state(NONE),cv_(),cond_mutex()
  {}


  lock_release_user::lock_release_user(extent_client *ec)
  :_ec(ec)
  {}

  void lock_release_user::dorelease(lock_protocol::lockid_t lid) {
    _ec->flush(lid);
  }

lock_client_cache::lock_client_cache(std::string xdst,
				     lock_release_user *_lu)
  : lock_client(xdst), lu(_lu),lookup_mutex(),lock_map()

{
  rlsrpc = new rpcs(0);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  const char *hname;
  hname = "127.0.0.1";

  std::ostringstream host;
  host << hname << ":" << rlsrpc->port();
  id = host.str();
}

lock_client_cache::~lock_client_cache() {
  delete rlsrpc;
  delete lu;

}


lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{


  int state;
  lock_protocol::status ret;
  pthread_mutex_lock(&lookup_mutex);
  std::map<lock_protocol::lockid_t,lock_state>::iterator it = lock_map.find(lid);

  if (it == lock_map.end()) {
    lock_state s;
    lock_map.insert(std::make_pair(lid,s));
    it = lock_map.find(lid);
  }
  pthread_mutex_unlock(&lookup_mutex);
  ScopedLock sl(&it->second.cond_mutex );
  if (it->second.state == lock_state::FREE) {
    ret = lock_protocol::OK;
    it->second.state = lock_state::LOCKED;
    return ret;

  }

  if (it->second.state == lock_state::ACQUIRING || it->second.state == lock_state::LOCKED) {
    while (it->second.state != lock_state::FREE ) {
      clock_gettime( CLOCK_REALTIME, & timeout );
      timeout.tv_sec += 1;
      pthread_cond_timedwait(&(it->second.cv_),&it->second.cond_mutex ,&timeout);
    }
    ret = lock_protocol::OK;
    it->second.state = lock_state::LOCKED;
    return ret;
  }
  while (it->second.state == lock_state::RELEASING) {
    clock_gettime( CLOCK_REALTIME, & timeout );
    timeout.tv_sec += 1;
    pthread_cond_timedwait(&(it->second.cv_),&it->second.cond_mutex ,&timeout);
  }

  while (it->second.state == lock_state::NONE) {
    it->second.state = lock_state::ACQUIRING;
    lock_protocol::status r = cl->call(lock_protocol::acquire, lid,id,state);
    if (r == lock_protocol::OK) {
      it->second.state = lock_state::LOCKED;
      return lock_protocol::OK;
    } else {
      while (it->second.state != lock_state::FREE && it->second.state != lock_state::NONE) {
        clock_gettime( CLOCK_REALTIME, & timeout );
        timeout.tv_sec += 1;
        pthread_cond_timedwait(&(it->second.cv_),&it->second.cond_mutex ,&timeout);
      }
      if (it->second.state == lock_state::FREE) {
        it->second.state = lock_state::LOCKED;
        return lock_protocol::OK;
      }
    }
  }
  return lock_protocol::RPCERR;

}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{

  pthread_mutex_lock(&lookup_mutex);
  std::map<lock_protocol::lockid_t,lock_state>::iterator it = lock_map.find(lid);
  pthread_mutex_unlock(&lookup_mutex);

  ScopedLock sl(&it->second.cond_mutex );
  int state;

  if (it->second.state == lock_state::RELEASING) {
    lu->dorelease(lid);
    lock_protocol::status r = cl->call(lock_protocol::release, lid,id,state);
    if (r != lock_protocol::OK) {
      return r;
    }
    it->second.state = lock_state::NONE;
  } else if (it->second.state == lock_state::LOCKED) {
    it->second.state = lock_state::FREE;

  }
  pthread_cond_signal(&(it->second.cv_));

  return lock_protocol::OK;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid,
                                  int &)
{
  int state;
  pthread_mutex_lock(&lookup_mutex);

  std::map<lock_protocol::lockid_t,lock_state>::iterator it = lock_map.find(lid);
  pthread_mutex_unlock(&lookup_mutex);
  ScopedLock sl(&it->second.cond_mutex );
  if (it->second.state == lock_state::FREE) {
    lu->dorelease(lid);
    lock_protocol::status r = cl->call(lock_protocol::release, lid,id,state);
    if (r != lock_protocol::OK) {
      return r;
    }
    it->second.state = lock_state::NONE;
  } else {
    it->second.state = lock_state::RELEASING;
  }

  int ret = rlock_protocol::OK;
  return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  pthread_mutex_lock(&lookup_mutex);
  std::map<lock_protocol::lockid_t,lock_state>::iterator it = lock_map.find(lid);
  pthread_mutex_unlock(&lookup_mutex);


  ScopedLock sl(&it->second.cond_mutex );
  it->second.state = lock_state::NONE;
  pthread_cond_signal(&(it->second.cv_));
  int ret = rlock_protocol::OK;
  return ret;
}




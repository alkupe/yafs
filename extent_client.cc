// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent
extent_data::extent_data()
:dirty(false),removed(false),extent_cached(false),buf()
{

}

extent_client::extent_client(std::string dst)
:extent_map(),_extent_mutex()
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

extent_client::~extent_client() {

  delete cl;
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  ScopedLock sl(&_extent_mutex);
  extent_protocol::status ret = extent_protocol::OK;
  time_t now = time(0);
  Extent_Map::iterator iter = extent_map.find(eid);
  if (iter == extent_map.end()) {
    ret = cl->call(extent_protocol::get, eid, buf);
    if (ret == extent_protocol::NOENT) {
      return ret;
    }
    extent_data newextent;

    ret = cl->call(extent_protocol::getattr, eid, newextent.attr);
    newextent.attr.size = buf.size();

    newextent.buf = buf;
    newextent.extent_cached = true;

    newextent.attr.mtime = now;
    newextent.attr.ctime = now;
    extent_map.insert(std::make_pair(eid,newextent));
    return ret;
  } else if (iter->second.removed) {
      ret = extent_protocol::NOENT;
      return ret;

  } else if (!iter->second.extent_cached) {
    ret = cl->call(extent_protocol::get, eid, buf);
    iter->second.extent_cached = true;
    iter->second.buf = buf;
    if (ret == extent_protocol::NOENT) {
      return ret;
    }
  } else {
    buf = iter->second.buf;
  }
  iter->second.attr.mtime = now;
  iter->second.attr.ctime = now;
  iter->second.attr.size = buf.size();
  return ret;
}




extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  ScopedLock sl(&_extent_mutex);
  extent_protocol::status ret = extent_protocol::OK;
  Extent_Map::iterator iter = extent_map.find(eid);
  if (iter == extent_map.end()) {

    cl->call(extent_protocol::getattr, eid, attr);
    if (ret == extent_protocol::NOENT) {
      return ret;
    }

    extent_data newextent;
    newextent.attr = attr;
    extent_map.insert(std::make_pair(eid,newextent));
    return ret;
  }
  if (iter->second.removed) {
    ret = extent_protocol::NOENT;
    return ret;

    }
  else {
     attr = iter->second.attr;
     return ret;
   }

}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  ScopedLock sl(&_extent_mutex);
  extent_protocol::status ret = extent_protocol::OK;

  time_t now = time(0);
  Extent_Map::iterator iter = extent_map.find(eid);
  if (iter == extent_map.end()) {
    extent_data newextent;
    newextent.buf = buf;
    newextent.attr.atime  = now;
    newextent.attr.mtime = now;
    newextent.attr.ctime = now;
    newextent.attr.size = buf.size();
    newextent.dirty =  true;
    newextent.extent_cached = true;
    extent_map.insert(std::make_pair(eid,newextent));
  } else if (iter->second.removed)
  {
    ret = extent_protocol::NOENT;
    return ret;

  } else  {
    iter->second.extent_cached = true;
    iter->second.buf = buf;
    iter->second.attr.mtime = now;
    iter->second.attr.ctime = now;
    iter->second.attr.size = buf.size();
    iter->second.dirty = true;
  }
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  ScopedLock sl(&_extent_mutex);
  extent_protocol::status ret = extent_protocol::OK;

  Extent_Map::iterator iter = extent_map.find(eid);
  if (iter == extent_map.end()) {
    extent_data newextent;
    newextent.removed = true;
    newextent.dirty = true;
    iter->second.extent_cached=true;
    extent_map.insert(std::make_pair(eid,newextent));
  } else {
    iter->second.removed = true;
    iter->second.extent_cached=true;
    iter->second.dirty = true;
  }
   return ret;

}

extent_protocol::status extent_client::flush(extent_protocol::extentid_t eid)
{
  ScopedLock sl(&_extent_mutex);
  extent_protocol::status ret =  extent_protocol::OK;
  int r;

  Extent_Map::iterator iter = extent_map.find(eid);
  if (iter->second.dirty) {
    if (iter->second.removed) {

      ret = cl->call(extent_protocol::remove, eid,r);
    } else {

      ret = cl->call(extent_protocol::put, eid, iter->second.buf, r);
    }

  }
  extent_map.erase(iter);
  return ret;
}



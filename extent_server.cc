// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rpc/slock.h"
extent_server::extent_server() :
content_map(),att_map(),map_m()
{}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  ScopedLock lock(&map_m);
  time_t now = time(0);
  std::pair<Content_Map::iterator,bool> m = content_map.insert(std::make_pair(id,buf));
  if (m.second) {
    extent_protocol::attr a;
    a.atime  = now;
    a.mtime = now;
    a.ctime = now;
    a.size = buf.size();
    std::pair<Att_Map::iterator,bool> check = att_map.insert(std::make_pair(id,a));
    if (!check.second) {
      return extent_protocol::IOERR;
    }
    return extent_protocol::OK;
  } else {
    content_map.find(id)->second = buf;
    Att_Map::iterator atit = att_map.find(id);
    if (atit == att_map.end()) {
      return extent_protocol::IOERR;
    }
    atit->second.mtime = now;
    atit->second.ctime = now;
    atit->second.size = buf.size();
    return extent_protocol::OK;
  }
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  ScopedLock lock(&map_m);
  Content_Map::iterator it = content_map.find(id);
  if (it == content_map.end()) {
    return extent_protocol::NOENT;
  } else {
    buf = it->second;
    Att_Map::iterator atit = att_map.find(id);
    atit->second.atime = time(0);
    return extent_protocol::OK;
  }
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{

  ScopedLock lock(&map_m);
  Att_Map::iterator it = att_map.find(id);
  if (it == att_map.end()) {
    return extent_protocol::NOENT;
  } else {
    a.size = it->second.size;
    a.atime  = it->second.atime;
    a.mtime = it->second.mtime;
    a.ctime = it->second.ctime;
    return extent_protocol::OK;
  }
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  ScopedLock lock(&map_m);
  Content_Map::iterator it = content_map.find(id);
  if (it == content_map.end()) {
    return extent_protocol::NOENT;
  } else {
    content_map.erase(id);
    att_map.erase(id);
    return extent_protocol::OK;
  }
}


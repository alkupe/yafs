// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client_cache.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst) :
  ec(new extent_client(extent_dst)), lc(new lock_client_cache(lock_dst, new lock_release_user(ec))) {
  srandom(getpid());
  inum myinum = 0x00000001;
  lc->acquire(myinum);
  std::string s;
  if (ec->get(myinum,s) == extent_protocol::NOENT) {
    if (ec->put(myinum, "") != extent_protocol::OK) {
      fprintf(stderr, "failed to create root\n");
      lc->release(myinum);
      exit(1);
    }
  }
  lc->release(myinum);
}

yfs_client::~yfs_client() {
  delete ec;
  delete lc;
}

yfs_client::inum yfs_client::n2i(std::string n) {
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string yfs_client::filename(inum inum) {
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool yfs_client::isfile(inum inum) {
  if (inum & 0x80000000) return true;
  return false;
}

bool yfs_client::isdir(inum inum) {
  return !isfile(inum);
}

int yfs_client::getfileatt(inum inum, fileinfo &fin) {
  int r = OK;
  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  lc->acquire(inum);
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

  release: lc->release(inum);
  return r;
}

int yfs_client::getdiratt(inum inum, dirinfo &din) {
  int r = OK;

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  lc->acquire(inum);
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

  release: lc->release(inum);
  return r;
}

int yfs_client::yfs_client::find_in_dir(inum myinum, std::string name,
                                        inum& found) {
  lc->acquire(myinum);

  int r = find_in_dir_helper(myinum, name, found);

  lc->release(myinum);
  return r;

}


int yfs_client::yfs_client::find_in_dir_helper(inum myinum, std::string name,
                                        inum& found) {
  std::string buf;

  if (ec->get(myinum, buf) != extent_protocol::OK) {

    return IOERR;
  }
  std::stringstream ss(buf);
  std::string record;
  while (getline(ss, record, '|')) {
    size_t dollar = record.find('$');
    std::string id(record.begin(), record.begin() + dollar);
    std::string file(record.begin() + dollar + 1, record.end());
    std::stringstream getid(id);
    inum item;
    getid >> item;
    if (name.compare(file) == 0) {
      found = item;
      return OK;
    }
  }
  return NOENT;
}




int yfs_client::yfs_client::remove_from_dir(inum myinum, std::string name) {
  lc->acquire(myinum);
  std::string buf;
  if (ec->get(myinum, buf) != extent_protocol::OK) {
    lc->release(myinum);
    return IOERR;
  }
  int r = NOENT;
  std::stringstream ss(buf);
  std::string record;
  std::string newbuf;
  inum found;
  while (getline(ss, record, '|')) {
    size_t dollar = record.find('$');
    std::string id(record.begin(), record.begin() + dollar);
    std::string file(record.begin() + dollar + 1, record.end());
    std::stringstream getid(id);
    inum item;
    getid >> item;
    if (name.compare(file) != 0) {
      newbuf.append(record);
      newbuf.append("|");
    } else {
      found = item;
      r = OK;
    }
  }
  if (r == OK) {
    lc->acquire(found);
    if (ec->put(myinum, newbuf) != extent_protocol::OK) {
      lc->release(myinum);
      lc->release(found);
      return IOERR;
    }
    if (ec->remove(found) != extent_protocol::OK) {
      lc->release(myinum);
      lc->release(found);
      return IOERR;
    }
    lc->release(found);
  }
  lc->release(myinum);
  return r;
}

int yfs_client::put_file_in_dir(inum parentinum, std::string file,
                                inum& newinum) {
  lc->acquire(parentinum);
  std::string buf;
  if (ec->get(parentinum, buf) != extent_protocol::OK) {
    lc->release(parentinum);
    return IOERR;
  }
  inum found;
  int r = find_in_dir_helper(parentinum, file, found);
  if (r != NOENT) {
    lc->release(parentinum);
    return EXIST;
  }
  newinum = random();
  newinum = newinum | 0x80000000;
  lc->acquire(newinum);
  r = put_in_dir(parentinum, newinum, file, buf);
  lc->release(newinum);
  lc->release(parentinum);
  return r;
}

int yfs_client::put_dir_in_dir(inum parentinum, std::string file, inum& newinum) {
  lc->acquire(parentinum);
  std::string buf;
  if (ec->get(parentinum, buf) != extent_protocol::OK) {
    lc->release(parentinum);
    return IOERR;
  }
  inum found;
  int r = find_in_dir_helper(parentinum, file, found);
  if (r != NOENT) {
    lc->release(parentinum);
    return EXIST;
  }
  newinum = random();
  newinum = newinum & ~(1 << 31);
  lc->acquire(newinum);
  r = put_in_dir(parentinum, newinum, file, buf);
  lc->release(newinum);
  lc->release(parentinum);
  return r;
}

int yfs_client::put_in_dir(inum parentinum, inum fileinum, std::string file,
                           std::string buf) {
  std::stringstream ss;

  ss << fileinum;
  ss << "$";
  ss << file;
  ss << "|";

  buf.append(ss.str());

  if (ec->put(parentinum, buf) != extent_protocol::OK) {
    return IOERR;
  }

  if (ec->put(fileinum, "") != extent_protocol::OK) {
    return IOERR;
  }
  return OK;
}

int yfs_client::readdir(inum parentinum,
                        std::vector<yfs_client::dirent>& filelist) {
  std::string buf;
  lc->acquire(parentinum);
  if (ec->get(parentinum, buf) != extent_protocol::OK) {
    lc->release(parentinum);
    return IOERR;
  }
  std::stringstream ss(buf);
  std::string record;
  while (getline(ss, record, '|')) {
    size_t found = record.find('$');
    std::string id(record.begin(), record.begin() + found);
    std::string file(record.begin() + found + 1, record.end());
    std::stringstream getid(id);

    inum item;
    getid >> item;
    dirent d;
    d.name = file;
    d.inum = item;
    filelist.push_back(d);
  }
  lc->release(parentinum);
  return OK;
}

int yfs_client::write_file_content(inum myinum, const char * mybuf,
                                   size_t size, off_t off) {
  lc->acquire(myinum);
  std::string oldbuf;
  extent_protocol::status r = ec->get(myinum, oldbuf);
  if (r != extent_protocol::OK) {
    lc->release(myinum);
    return NOENT;
  }
  if (off + size > oldbuf.size()) {
    oldbuf.resize(off + size);
  }
  oldbuf.replace(off, size, mybuf, size);
  r = ec->put(myinum, oldbuf);
  if (r != extent_protocol::OK) {
    lc->release(myinum);
    return IOERR;
  }
  lc->release(myinum);
  return OK;
}

int yfs_client::set_file_size(inum myinum, off_t size) {
  lc->acquire(myinum);
  std::string oldbuf;
  extent_protocol::status r = ec->get(myinum, oldbuf);
  if (r != extent_protocol::OK) {
    lc->release(myinum);
    return NOENT;
  }
  oldbuf.resize(size);
  r = ec->put(myinum, oldbuf);
  if (r != extent_protocol::OK) {
    lc->release(myinum);
    return IOERR;
  }
  lc->release(myinum);
  return OK;
}

int yfs_client::read_file_content(inum myinum, std::string& buf) {
  lc->acquire(myinum);
  extent_protocol::status r = ec->get(myinum, buf);
  lc->release(myinum);
  return r;
}


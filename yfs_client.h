#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

#include "lock_protocol.h"
#include "lock_client_cache.h"

class yfs_client {
  extent_client *ec;
  lock_client_cache *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
  int put_in_dir(inum ,inum , std::string, std::string);
  int find_in_dir_helper(inum myinum, std::string name,
                                          inum& found);
 public:

  yfs_client(std::string, std::string);
  ~yfs_client();
  int put_file_in_dir(inum, std::string , inum& );
  int put_dir_in_dir(inum, std::string , inum& );
  int find_in_dir(inum ,std::string buf, inum&);
  int remove_from_dir(inum ,std::string);
  bool isfile(inum);
  bool isdir(inum);
  int getfileatt(inum, fileinfo &);
  int getdiratt(inum, dirinfo &);
  int readdir(inum,std::vector<yfs_client::dirent>&);
  int read_file_content(inum, std::string&);
  int write_file_content(inum, const char*, size_t,off_t);
  int set_file_size(inum,off_t);
};

#endif

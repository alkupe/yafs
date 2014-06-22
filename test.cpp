/*
 * test.cpp
 *
 *  Created on: Sep 8, 2012
 *      Author: alex
 */

/*#include "lock_protocol.h"
#include "lock_client.h"
*/
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <sys/time.h>
using namespace std;
//#include "extent_protocol.h"
struct dum {
  int x;
  std::string y;
};


bool isfile(int inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}



//typedef std::map<extent_protocol::extentid_t,std::string> Content_Map;
//typedef std::map<extent_protocol::extentid_t,extent_protocol::attr> Att_Map;
int main(int argc, char *argv[])
{
  pthread_cond_t cv_;
  pthread_mutex_t m_;
cout << "hi\n";
struct timeval tv;
struct timespec ts;
  struct timespec timeout1;
  gettimeofday(&tv, NULL);
  ts.tv_sec = tv.tv_sec + 1;     /* 5 seconds in future */
  ts.tv_nsec = tv.tv_usec * 1000; /* microsec to nanosec */
  ( void ) pthread_mutex_init( & m_, NULL );
  ( void ) pthread_cond_init( & cv_, NULL );

  ( void ) pthread_mutex_lock( & m_ );
//( void ) clock_gettime( CLOCK_REALTIME, & timeout1 );
// timeout1.tv_nsec += 50000000000;
  /* One minute from the current system time */
  clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
  //abstime.tv_sec += 60;
    int rc = 0;
    struct timespec timeout;
  while (true){

    clock_gettime( CLOCK_REALTIME, & timeout );
    //ts.tv_sec = tv.tv_sec + 1;     /* 5 seconds in future */
    //ts.tv_nsec = tv.tv_usec * 1000; /* microsec to nanosec */
    //cout << tv.tv_sec << " " << ts.tv_sec << endl;
    //cout <<
     timeout.tv_nsec  += 750000000;
    rc = pthread_cond_timedwait(&cv_, &m_,&timeout);

    cout << rc << " hi\n";
  }
  //pthread_cond_timedwait(&cv_, &m_, &timeout);
  //printf("hi\n");
  return 0;
}

#define _GNU_SOURCE
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "libco.h"
#include "flb_libco/settings.h"


#ifdef MARK_EXEC
__attribute__((section(".custom-section")))
int test(int a1, int a2, int a3) {
  printf("I will never be called");
  printf("%d %d %d", a1, a2, a3);
  return 1;
}
#endif

static cothread_t t_main1;

static char *itimerspec_dump(struct itimerspec *ts);

static char *
itimerspec_dump(struct itimerspec *ts)
{
    static char buf[1024];

    snprintf(buf, sizeof(buf),
            "itimer: [ interval=%lu s %lu ns, next expire=%lu s %lu ns ]",
            ts->it_interval.tv_sec,
            ts->it_interval.tv_nsec,
            ts->it_value.tv_sec,
            ts->it_value.tv_nsec
           );

    return (buf);
}



void my_coroutine() {
  printf("called my_coroutine ");
  co_switch(t_main1);
}


int create_timer_fd() {  
  int msec = 2000;
  
  int tfd;
  struct itimerspec ts;
  
  tfd = timerfd_create(CLOCK_REALTIME, 0);
  if (tfd == -1) {
    printf("timerfd_create() failed: errno=%d\n", errno);
    return EXIT_FAILURE;
  }
  printf("created timerfd %d\n", tfd);
  
  ts.it_interval.tv_sec = msec / 1000;
  ts.it_interval.tv_nsec = (msec % 1000) * 1000000;
  ts.it_value.tv_sec = msec / 1000;
  ts.it_value.tv_nsec = (msec % 1000) * 1000000;
  
  if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &ts, NULL) < 0) {
    printf("timerfd_settime() failed: errno=%d\n", errno);
    close(tfd);
    return EXIT_FAILURE;
  }
  
  printf("set timerfd time=%s\n", itimerspec_dump(&ts));

  return tfd;
}

void *timer_loop(void *arg) {  
  cothread_t t_sub1;
  
  int tfd, epfd, ret;
  struct epoll_event ev;
  
  tfd = create_timer_fd();

  if (tfd == -1) {
    printf("creating timerfd failed: errno=%d\n", errno);
    close(tfd);
    return EXIT_FAILURE;
  }
  
  epfd = epoll_create(256);

  
  if (epfd == -1) {
    printf("epoll_create() failed: errno=%d\n", errno);
    close(tfd);
    return EXIT_FAILURE;
  }
  
  printf("created epollfd %d\n", epfd);
  
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
  
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
    printf("epoll_ctl(ADD) failed: errno=%d\n", errno);
    close(epfd);
    close(tfd);
    return EXIT_FAILURE;
  }
  
  printf("added timerfd to epoll set\n");

  uint64_t res;
  
  while(1) {
    memset(&ev, 0, sizeof(ev));
    
    ret = epoll_wait(epfd, &ev, 1, -1); // wait up to 500ms for timer
    //_loop(NULL);
    if (ret < 0) {
      printf("epoll_wait() failed: errno=%d\n", errno);
      close(epfd);
      close(tfd);
      return EXIT_FAILURE;
    }

    ret = read(tfd, &res, sizeof(res));

    printf("Current timestamp is %" PRIu64 "\n", res);

    size_t st;
    
    t_main1 = co_active();
    t_sub1 = co_create(65536, my_coroutine, &st);
    co_switch(t_sub1);
  }
}

int main()
{
  /* pthread_t t1; */
  
  /* if(pthread_create(&t1, NULL, marker2, 1)) { */
  /*   fprintf(stderr, "Error creatingC thread\n"); */
  /*   return 1; */
  /* } */

  timer_loop(1);
  sleep(10);
  sleep(10);

  return 0;
}

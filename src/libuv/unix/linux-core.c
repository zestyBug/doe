/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* We lean on the fact that POLL{IN,OUT,ERR,HUP} correspond with their
 * EPOLL* counterparts.  We use the POLL* variants in this file because that
 * is what libuv uses elsewhere.
 */

#include "uv.h"
#include "internal.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <net/if.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <sys/prctl.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define HAVE_IFADDRS_H 1

# if defined(__ANDROID_API__) && __ANDROID_API__ < 24
# undef HAVE_IFADDRS_H
#endif

#ifdef __UCLIBC__
# if __UCLIBC_MAJOR__ < 0 && __UCLIBC_MINOR__ < 9 && __UCLIBC_SUBLEVEL__ < 32
#  undef HAVE_IFADDRS_H
# endif
#endif

#ifdef HAVE_IFADDRS_H
# include <ifaddrs.h>
# include <sys/socket.h>
# include <net/ethernet.h>
# include <netpacket/packet.h>
#endif /* HAVE_IFADDRS_H */

/* Available from 2.6.32 onwards. */
#ifndef CLOCK_MONOTONIC_COARSE
# define CLOCK_MONOTONIC_COARSE 6
#endif

/* This is rather annoying: CLOCK_BOOTTIME lives in <linux/time.h> but we can't
 * include that file because it conflicts with <time.h>. We'll just have to
 * define it ourselves.
 */
#ifndef CLOCK_BOOTTIME
# define CLOCK_BOOTTIME 7
#endif

int uv__platform_loop_init(uv_loop_t* loop) {
  
  loop->inotify_fd = -1;
  loop->inotify_watchers = NULL;

  return uv__epoll_init(loop);
}


void uv__platform_loop_delete(uv_loop_t* loop) {
  if (loop->inotify_fd == -1) return;
  uv__io_stop(loop, &loop->inotify_read_watcher, POLLIN);
  uv__close(loop->inotify_fd);
  loop->inotify_fd = -1;
}



uint64_t uv__hrtime(uv_clocktype_t type) {
  static clock_t fast_clock_id = -1;
  struct timespec t;
  clock_t clock_id;

  /* Prefer CLOCK_MONOTONIC_COARSE if available but only when it has
   * millisecond granularity or better.  CLOCK_MONOTONIC_COARSE is
   * serviced entirely from the vDSO, whereas CLOCK_MONOTONIC may
   * decide to make a costly system call.
   */
  /* TODO(bnoordhuis) Use CLOCK_MONOTONIC_COARSE for UV_CLOCK_PRECISE
   * when it has microsecond granularity or better (unlikely).
   */
  clock_id = CLOCK_MONOTONIC;
  if (type != UV_CLOCK_FAST)
    goto done;

  clock_id = uv__load_relaxed(&fast_clock_id);
  if (clock_id != -1)
    goto done;

  clock_id = CLOCK_MONOTONIC;
  if (0 == clock_getres(CLOCK_MONOTONIC_COARSE, &t))
    if (t.tv_nsec <= 1 * 1000 * 1000)
      clock_id = CLOCK_MONOTONIC_COARSE;

  uv__store_relaxed(&fast_clock_id, clock_id);

done:

  if (clock_gettime(clock_id, &t))
    return 0;  /* Not really possible. */

  return t.tv_sec * (uint64_t) 1e9 + t.tv_nsec;
}


int uv_resident_set_memory(size_t* rss) {
  char buf[1024];
  const char* s;
  ssize_t n;
  long val;
  int fd;
  int i;

  do
    fd = open("/proc/self/stat", O_RDONLY);
  while (fd == -1 && errno == EINTR);

  if (fd == -1)
    return UV__ERR(errno);

  do
    n = read(fd, buf, sizeof(buf) - 1);
  while (n == -1 && errno == EINTR);

  uv__close(fd);
  if (n == -1)
    return UV__ERR(errno);
  buf[n] = '\0';

  s = strchr(buf, ' ');
  if (s == NULL)
    goto err;

  s += 1;
  if (*s != '(')
    goto err;

  s = strchr(s, ')');
  if (s == NULL)
    goto err;

  for (i = 1; i <= 22; i++) {
    s = strchr(s + 1, ' ');
    if (s == NULL)
      goto err;
  }

  errno = 0;
  val = strtol(s, NULL, 10);
  if (errno != 0)
    goto err;
  if (val < 0)
    goto err;

  *rss = val * getpagesize();
  return 0;

err:
  return UV_EINVAL;
}

int uv_uptime(double* uptime) {
  static volatile int no_clock_boottime;
  char buf[128];
  struct timespec now;
  int r;

  /* Try /proc/uptime first, then fallback to clock_gettime(). */

  if (0 == uv__slurp("/proc/uptime", buf, sizeof(buf)))
    if (1 == sscanf(buf, "%lf", uptime))
      return 0;

  /* Try CLOCK_BOOTTIME first, fall back to CLOCK_MONOTONIC if not available
   * (pre-2.6.39 kernels). CLOCK_MONOTONIC doesn't increase when the system
   * is suspended.
   */
  if (no_clock_boottime) {
    retry_clock_gettime: r = clock_gettime(CLOCK_MONOTONIC, &now);
  }
  else if ((r = clock_gettime(CLOCK_BOOTTIME, &now)) && errno == EINVAL) {
    no_clock_boottime = 1;
    goto retry_clock_gettime;
  }

  if (r)
    return UV__ERR(errno);

  *uptime = now.tv_sec;
  return 0;
}




#ifdef HAVE_IFADDRS_H
static int uv__ifaddr_exclude(struct ifaddrs *ent, int exclude_type) {
  if (!((ent->ifa_flags & IFF_UP) && (ent->ifa_flags & IFF_RUNNING)))
    return 1;
  if (ent->ifa_addr == NULL)
    return 1;
  /*
   * On Linux getifaddrs returns information related to the raw underlying
   * devices. We're not interested in this information yet.
   */
  if (ent->ifa_addr->sa_family == PF_PACKET)
    return exclude_type;
  return !exclude_type;
}
#endif

int uv_interface_addresses(uv_interface_address_t** addresses, int* count) {
#ifndef HAVE_IFADDRS_H
  *count = 0;
  *addresses = NULL;
  return UV_ENOSYS;
#else
  struct ifaddrs *addrs, *ent;
  uv_interface_address_t* address;
  int i;
  struct sockaddr_ll *sll;

  *count = 0;
  *addresses = NULL;

  if (getifaddrs(&addrs))
    return UV__ERR(errno);

  /* Count the number of interfaces */
  for (ent = addrs; ent != NULL; ent = ent->ifa_next) {
    if (uv__ifaddr_exclude(ent, UV__EXCLUDE_IFADDR))
      continue;

    (*count)++;
  }

  if (*count == 0) {
    freeifaddrs(addrs);
    return 0;
  }

  /* Make sure the memory is initiallized to zero using calloc() */
  *addresses = uv__calloc(*count, sizeof(**addresses));
  if (!(*addresses)) {
    freeifaddrs(addrs);
    return UV_ENOMEM;
  }

  address = *addresses;

  for (ent = addrs; ent != NULL; ent = ent->ifa_next) {
    if (uv__ifaddr_exclude(ent, UV__EXCLUDE_IFADDR))
      continue;

    address->name = uv__strdup(ent->ifa_name);

    if (ent->ifa_addr->sa_family == AF_INET6) {
      address->address.address6 = *((struct sockaddr_in6*) ent->ifa_addr);
    } else {
      address->address.address4 = *((struct sockaddr_in*) ent->ifa_addr);
    }

    if (ent->ifa_netmask->sa_family == AF_INET6) {
      address->netmask.netmask6 = *((struct sockaddr_in6*) ent->ifa_netmask);
    } else {
      address->netmask.netmask4 = *((struct sockaddr_in*) ent->ifa_netmask);
    }

    address->is_internal = !!(ent->ifa_flags & IFF_LOOPBACK);

    address++;
  }

  /* Fill in physical addresses for each interface */
  for (ent = addrs; ent != NULL; ent = ent->ifa_next) {
    if (uv__ifaddr_exclude(ent, UV__EXCLUDE_IFPHYS))
      continue;

    address = *addresses;

    for (i = 0; i < (*count); i++) {
      size_t namelen = strlen(ent->ifa_name);
      /* Alias interface share the same physical address */
      if (strncmp(address->name, ent->ifa_name, namelen) == 0 &&
          (address->name[namelen] == 0 || address->name[namelen] == ':')) {
        sll = (struct sockaddr_ll*)ent->ifa_addr;
        memcpy(address->phys_addr, sll->sll_addr, sizeof(address->phys_addr));
      }
      address++;
    }
  }

  freeifaddrs(addrs);

  return 0;
#endif
}


void uv_free_interface_addresses(uv_interface_address_t* addresses,
  int count) {
  int i;

  for (i = 0; i < count; i++) {
    uv__free(addresses[i].name);
  }

  uv__free(addresses);
}


void uv__set_process_title(const char* title) {
#if defined(PR_SET_NAME)
  prctl(PR_SET_NAME, title);  /* Only copies first 16 characters. */
#endif
}


static uint64_t uv__read_proc_meminfo(const char* what) {
  uint64_t rc;
  char* p;
  char buf[4096];  /* Large enough to hold all of /proc/meminfo. */

  if (uv__slurp("/proc/meminfo", buf, sizeof(buf)))
    return 0;

  p = strstr(buf, what);

  if (p == NULL)
    return 0;

  p += strlen(what);

  rc = 0;
  sscanf(p, "%" PRIu64 " kB", &rc);

  return rc * 1024;
}


uint64_t uv_get_free_memory(void) {
  struct sysinfo info;
  uint64_t rc;

  rc = uv__read_proc_meminfo("MemAvailable:");

  if (rc != 0)
    return rc;

  if (0 == sysinfo(&info))
    return (uint64_t) info.freeram * info.mem_unit;

  return 0;
}


uint64_t uv_get_total_memory(void) {
  struct sysinfo info;
  uint64_t rc;

  rc = uv__read_proc_meminfo("MemTotal:");

  if (rc != 0)
    return rc;

  if (0 == sysinfo(&info))
    return (uint64_t) info.totalram * info.mem_unit;

  return 0;
}


static uint64_t uv__read_cgroups_uint64(const char* cgroup, const char* param) {
  char filename[256];
  char buf[32];  /* Large enough to hold an encoded uint64_t. */
  uint64_t rc;

  rc = 0;
  snprintf(filename, sizeof(filename), "/sys/fs/cgroup/%s/%s", cgroup, param);
  if (0 == uv__slurp(filename, buf, sizeof(buf)))
    sscanf(buf, "%" PRIu64, &rc);

  return rc;
}


uint64_t uv_get_constrained_memory(void) {
  /*
   * This might return 0 if there was a problem getting the memory limit from
   * cgroups. This is OK because a return value of 0 signifies that the memory
   * limit is unknown.
   */
  return uv__read_cgroups_uint64("memory", "memory.limit_in_bytes");
}


void uv_loadavg(double avg[3]) {
  struct sysinfo info;
  char buf[128];  /* Large enough to hold all of /proc/loadavg. */

  if (0 == uv__slurp("/proc/loadavg", buf, sizeof(buf)))
    if (3 == sscanf(buf, "%lf %lf %lf", &avg[0], &avg[1], &avg[2]))
      return;

  if (sysinfo(&info) < 0)
    return;

  avg[0] = (double) info.loads[0] / 65536.0;
  avg[1] = (double) info.loads[1] / 65536.0;
  avg[2] = (double) info.loads[2] / 65536.0;
}

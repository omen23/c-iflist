#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

int ethernet_interface(const char *const name,
                       int *const index, int *const speed, int *const duplex)
{
  struct ifreq        ifr;
  struct ethtool_cmd  cmd;
  int                 fd, result;
  
  if (!name || !*name) {
    fprintf(stderr, "Error: NULL interface name.\n");
    fflush(stderr);
    return errno = EINVAL;
  }
  
  if (index)  *index = -1;
  if (speed)  *speed = -1;
  if (duplex) *duplex = -1;
  
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    const int err = errno;
    fprintf(stderr, "%s: Cannot create AF_INET socket: %s.\n", name, strerror(err));
    fflush(stderr);
    return errno = err;
  }
  
  strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
  ifr.ifr_data = (void *)&cmd;
  cmd.cmd = ETHTOOL_GSET;
  if (ioctl(fd, SIOCETHTOOL, &ifr) < 0) {
    const int err = errno;
    do {
      result = close(fd);
    } while (result == -1 && errno == EINTR);
    fprintf(stderr, "%s: SIOCETHTOOL ioctl: %s.\n", name, strerror(err));
    return errno = err;
  }
  
  if (speed)
    *speed = ethtool_cmd_speed(&cmd);
  
  if (duplex) {
    switch (cmd.duplex) {
      case DUPLEX_HALF: *duplex = 0; break;
      case DUPLEX_FULL: *duplex = 1; break;
      default:
        fprintf(stderr, "%s: Unknown mode (0x%x).\n", name, cmd.duplex);
        fflush(stderr);
        *duplex = -1;
    }
  }
    
    if (index && ioctl(fd, SIOCGIFINDEX, &ifr) >= 0)
      *index = ifr.ifr_ifindex;
    
    do {
      result = close(fd);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
      const int err = errno;
      fprintf(stderr, "%s: Error closing socket: %s.\n", name, strerror(err));
      return errno = err;
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
  int  arg, speed, index, duplex;
  
  if (argc < 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [ -h | --help ]\n", argv[0]);
    fprintf(stderr, "       %s INTERFACE ...\n", argv[0]);
    fprintf(stderr, "\n");
    return 1;
  }
  
  for (arg = 1; arg < argc; arg++) {
    if (ethernet_interface(argv[arg], &index, &speed, &duplex))
      return 1;
    
    if (index == -1)
      printf("%s: (no interface index)", argv[arg]);
    else
      printf("%s: interface %d", argv[arg], index);
    
    if (speed == -1)
      printf(", unknown bandwidth");
    else
      printf(", %d Mbps bandwidth", speed);
    
    if (duplex == 0)
      printf(", half duplex.\n");
    else if (duplex == 1)
      printf(", full duplex.\n");
    else
      printf(", unknown mode.\n");
  }
  
  return 0;
}

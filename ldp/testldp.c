#include "ldp.h"
#include "linkcommon.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  struct ldp_interface *intf;
  if (argc != 2)
  {
    printf("Usage: testldp netmap:eth0\n");
    return 1;
  }
  intf = ldp_interface_open(argv[1], 1, 1);
  if (intf == NULL)
  {
    perror("Can't open interface");
    return 1;
  }
  if (ldp_interface_link_wait(intf) != 0)
  {
    printf("Can't wait for link\n");
    return 1;
  }
  ldp_interface_close(intf);
  return 0;
}

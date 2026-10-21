#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  unsigned int i = 0x00646c72;
  unsigned char *bytes = (unsigned char *)&i;

  printf("H%x Wo%s\n", 57616U, (char *)&i);
  printf("bytes low to high: %x %x %x %x\n",
         (unsigned int)bytes[0], (unsigned int)bytes[1],
         (unsigned int)bytes[2], (unsigned int)bytes[3]);

  exit(0);
}
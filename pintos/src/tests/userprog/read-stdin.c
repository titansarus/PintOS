/* Try writing from fd 0 (stdin) and print it in stdout, */

#include <stdio.h>
#include <syscall.h>
#include "tests/main.h"

void
test_main (void)
{
  char buf[64];
  read (STDIN_FILENO, &buf, 64);
  printf ("stdin: %s", buf);
}

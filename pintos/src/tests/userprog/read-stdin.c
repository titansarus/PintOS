/* Try writing from fd 0 (stdin) and print it in stdout, */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  char buf[11];
  size_t rsize= read (0, &buf, 11);
  msg  ("%s" ,buf);
}

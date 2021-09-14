#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

static char buffer[1];

void
test_main (void)
{
  int file;
  
  CHECK (create ("a", 0), "create \"a\"");
  CHECK ((file = open ("a")) > 1, "open \"a\" for writing");

  cache_invalidate();
  int init_write = cache_spec (CACHE_W_CNT);

  msg ("writing 64kB to \"a\"");
  
  int num_write_bytes = 1<<16;   /* 2^16 */

  for(int i=0;i<num_write_bytes;i++) {
    random_init (0);
    random_bytes (buffer, sizeof buffer);
    if (write (file, buffer, 1) != 1)
      fail ("invalid number of written bytes");
  }
  
  msg("close \"a\" after writing");
  close(file);

  CHECK ((file = open ("a")) > 1, "open \"a\" for read");
  for(int i=0;i<num_write_bytes;i++)
    if(read (file, buffer, 1)!= 1)
      fail ("invalid number of read bytes");

  int num_writes = cache_spec (CACHE_W_CNT)-init_write;

  if (num_writes < 132 && num_writes > 124) 
    msg("total number of writes is close to 128");
  
  close (file);
  remove("a");

}

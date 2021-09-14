/* Measuring Cache Statistics */
#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BLOCK_SIZE 512
#define NUM_BLOCKS 50
static char buffer[BLOCK_SIZE];

void read_byte_util(int fd)
{
  for (int i = 0; i < NUM_BLOCKS; i++)
    {
      int bytes_read = read (fd, buffer, BLOCK_SIZE);
      if (bytes_read != BLOCK_SIZE)
          fail ("read %zu bytes in \"a\" returned %zu",
                BLOCK_SIZE, bytes_read);
    }
}


void
test_main (void)
{
  int fd;
  random_init (0);
  random_bytes (buffer, sizeof buffer);
  CHECK (create ("a", 0), "create \"a\"");
  CHECK ((fd = open ("a")) > 1, "open \"a\"");

  msg("creating a");
  
  for(int i = 0; i < NUM_BLOCKS; i++)
  {
    int bytes_written = write (fd, buffer, BLOCK_SIZE);
    if (bytes_written != BLOCK_SIZE)
      fail ("write %zu bytes in \"a\" returned %zu",
            BLOCK_SIZE, bytes_written);
  }
  msg ("close \"tmp\"");
  close (fd);

  msg("invalidating cache");
  cache_invalidate ();

  CHECK ((fd = open ("a")) > 1, "open \"a\"");
  msg ("read tmp");
  read_byte_util (fd);
  close (fd);
  msg("close \"a\"");

  int old_hit =  cache_spec (CACHE_HIT);
  int old_total = cache_spec (CACHE_HIT) + cache_spec (CACHE_MISS);
  int old_hit_rate = (100 * cache_spec (CACHE_HIT)) / (old_total);

  CHECK ((fd = open ("a")) > 1, "open \"a\"");
  msg ("read \"a\"");
  read_byte_util (fd);
  close (fd);
  msg("close \"a\"");

  remove ("a");

  int new_total = cache_spec (CACHE_MISS) + cache_spec (CACHE_HIT);
  int new_hit_rate = 100 * (cache_spec (CACHE_HIT) - old_hit) / (new_total - old_total);

  if (new_hit_rate>old_hit_rate){
    msg("new hit rate > old hit rate");
  }
}

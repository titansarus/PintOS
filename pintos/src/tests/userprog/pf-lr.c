#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  wait (exec ("pf-lr-util"));
  open ("sample.txt");
}
/* Wait for a process that will be killed for bad behavior. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
    char test[15]="seek tell test"; 
    int fd;

    CHECK ((create ("st.txt", sizeof test -1)), "create \"st.txt\" success");
    CHECK ((fd = open ("st.txt")) > 1, "open \"st.txt\" success");

    int size_write = write (fd, test, sizeof test - 1);
    if (size_write != sizeof test - 1)
        fail ("write() returned %d instead of %zu", size_write, sizeof test - 1);

    msg ("preforming seek \"st.txt\"");
    seek (fd, sizeof test - 5);
    CHECK ((tell (fd) == sizeof test - 5), "tell \"st.txt\" success");
}

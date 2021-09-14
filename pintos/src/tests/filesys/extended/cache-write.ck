# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(cache-write) begin
(cache-write) create "cache"
(cache-write) open "cache" for writing
(cache-write) writing 64kB to "cache"
(cache-write) close "cache" after writing
(cache-write) open "cache" for read
(cache-write) total number of writes is close to 128
(cache-write) end
cache-write: exit(0)
EOF
pass;

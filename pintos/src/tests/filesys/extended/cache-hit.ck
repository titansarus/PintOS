# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(cache-hit) begin
(cache-hit) create "a"
(cache-hit) open "a"
(cache-hit) creating a
(cache-hit) close "tmp"
(cache-hit) invalidating cache
(cache-hit) open "a"
(cache-hit) read tmp
(cache-hit) close "a"
(cache-hit) open "a"
(cache-hit) read "a"
(cache-hit) close "a"
(cache-hit) new hit rate > old hit rate
(cache-hit) end
cache-hit: exit(0)
EOF
pass;

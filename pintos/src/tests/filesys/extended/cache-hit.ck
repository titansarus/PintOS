# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(cache-hit) begin
(cache-hit) create "cache"
(cache-hit) open "cache"
(cache-hit) creating cache
(cache-hit) close "tmp"
(cache-hit) invalidating cache
(cache-hit) open "cache"
(cache-hit) read tmp
(cache-hit) close "cache"
(cache-hit) open "cache"
(cache-hit) read "cache"
(cache-hit) close "cache"
(cache-hit) new hit rate > old hit rate
(cache-hit) end
cache-hit: exit(0)
EOF
pass;

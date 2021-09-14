# -​*- perl -*​-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(pf-lr) begin
(pf-lr-util) begin
pf-lr-util: exit(-1)
(pf-lr) end
pf-lr: exit(0)
EOF
pass;
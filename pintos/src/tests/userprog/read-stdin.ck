# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(read-stdin) begin
stdin: BE ARVAHE HADDAM, NISTI DAR JADDAM!
(read-stdin) end
read-stdin: exit(0)
EOF
pass;

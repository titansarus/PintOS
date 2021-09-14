# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(read-stdin) begin
(read-stdin) Works Fine!
(read-stdin) end
read-stdin: exit(0)
EOF
pass;

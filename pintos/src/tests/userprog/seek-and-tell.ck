# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(seek-and-tell) begin
(seek-and-tell) create "st.txt" success
(seek-and-tell) open "st.txt" success
(seek-and-tell) preforming seek "st.txt"
(seek-and-tell) tell "st.txt" success
(seek-and-tell) end
seek-and-tell: exit(0)
EOF
pass;
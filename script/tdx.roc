
# take data (without cal)
# sm
# modcaldel 2 2

dac  254  50  WBC
flush

pgstop
pgset 0 b010000  16  pg_rest (reset TBM)
pgset 1 b100000  56  pg_sync (stretch after sync)
pgset 2 b000010   0  pg_trg (delay zero = end of pgset)

# stretch 3 1 999
allon
modtd 4000 (trigger f = 40 MHz / period)

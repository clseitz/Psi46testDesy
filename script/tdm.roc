
# take source data with module
# sm

dac  254  50  # WBC

pgstop
pgset 0 b010000  16  # pg_rest (reset TBM)
pgset 1 b100000  56  # pg_sync (stretch after sync)
pgset 2 b000010   0  # pg_trg (delay zero = end of pgset)
flush

# stretch 1 8 999 # does not work for modules

allon
flush
modtd 400  # trigger f = 40 MHz / period

mask

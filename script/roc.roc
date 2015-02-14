
-- 13.8.2014 at FEC: single ROC test
-- sfec

optia 30

ctl 4
caldel 22 33
effmap 100

# check global threshold:

ctl 0
phdac 22 33 25


ctl 4
tune 22 33

# PH vs Vcal:

ctl 0
dacscanroc 25 16 1 127

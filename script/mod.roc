
# module procedure 2015

smr
hvon
vb 150
getib

# set analog current per ROC [mA]:
optiamod 24

# set timing:
ctl 4
modcaldel 22 33

# response map:
modmap 20

# set global treshold to Vcal target
modvthrcomp 44

# set timing:
modcaldel 22 33

# set Vtrim and trim bits to Vcal target:
modtrim 32

# set timing:
modcaldel 22 33

# adjust noisy pixels:
modtrimbits

# response map:
modmap 20

# threshold map:
modthrmap 1 28

# PH tuning:
ctl 4
modtune 22 33

# save:
wdac ia24-trim32-chiller15
wtrim ia24-trim32-chiller15

# gaincal:
ctl 4
dacscanmod 25 16

ctl 0
dacscanmod 25 16

# BBtest:
ctl 4
optiamod 30  # better separation
modcaldel 22 33
modvthrcomp 32
modcaldel 22 33
dacscanmod 12 -16 2

# bump height:
#ctl 4
#dacscanmod 25 -4

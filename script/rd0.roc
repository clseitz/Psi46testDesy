# DP 1/2014 read module channel 0 with deser400

#dopen 32000 0
dsel 400
dreset 3
udelay(1000)
dstart 0
udelay(1000)
pgsingle
udelay(1000)
dstop 0
dread400 0
#dreadm 0
#dclose 0

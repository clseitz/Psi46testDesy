# DP 1/2014 read module channel 1 with deser400

dreset 3
#dopen 32000 1
dsel 400
dstart 1
pgsingle
udelay(500)
dstop 1
#dread400 1
dreadm 1
#dclose 1

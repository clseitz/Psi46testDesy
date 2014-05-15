
# psi46digV2.1

log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  100 mA
ia  100 mA

- timing & levels:
clk  4
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl 10
ctrlvl 10
sdalvl 10
tinlvl 10

--- pattern generator: set redout timing --------------------
pgstop
pgset 0 b101000  25  pg_resr pg_sync
pgset 1 b000100 106  pg_cal, set WBC = pg_cal - 7
pgset 2 b000010  46  pg_trg  
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 160  # delay in trigger loop [BC], 160 = 4 us

# d1 1 (40 MHz clk on D1)
# d1 4 (token on D1, see pixel_dtb.h)
  d1 5 (trigg on D1, see pixel_dtb.h)
  d2 6 (cal   on D2, see pixel_dtb.h)
# d1 7 (reset on D1, see pixel_dtb.h)
# d1 9 (sync  on D1, see pixel_dtb.h)

select 0	  [set roclist, I2C]
rocaddr 0	  [set ROC]

dopen 40100100 0 [daq_open]

--- power on --------------------------------
pon
hvon

mdelay 500

--- set divgV2.1 chip 404 DACs-----------------------------

dac   1   10  Vdig  needed for large events
dac   2   99  Vana  optia 25
dac   3   33  Vsf   linearity
dac   4    6  Vcomp better multi-pulse eff (td52)

dac   7  140  VwllPr (for effmap 98)
dac   9  140  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   140 VthrComp

dac  13   30  VIBias_Bus
dac  22   99  VIColOr

dac  17  210  VoffsetRO

dac  19   10  Vcomp_ADC
dac  20   99  VIref_ADC

dac  25  222  Vcal
dac  26  123  CalDel

dac 253    4  CtrlReg
dac 254   99  WBC (159 to get 79 pixel/DC, but not 80 = erase)

flush

show

mask

getvd
getva
getid
getia

fire 2 2

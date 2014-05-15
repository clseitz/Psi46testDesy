
# dig mod with clock stretch. does not work

log === startdigmod ===

--- set voltages and current limits

vd 2600 mV
va 1800 mV
id  800 mA
ia  800 mA

--- setup timing & levels -------------------
clk  3  (5 is bad in modcaldel, 3 is best in modtd
ctr  3  (CLK +  0)
sda 18  (CLK + 15)
tin  8  (CLK +  5)

clklvl 10
ctrlvl 10
sdalvl 10
tinlvl 10

--- power on --------------------------------
pon
mdelay 400
hvon
mdelay 400
resoff
mdelay 200

--- setup TBM08: 2 cores at E and F

modsel b11111

tbmset $E4 $F0    Init TBM, Reset ROC
tbmset $F4 $F0

tbmset $E0 $01    Disable PKAM Counter
tbmset $F0 $01

tbmset $E2 $C0    Mode = Calibration
tbmset $F2 $C0

tbmset $E8 $10    Set PKAM Counter
tbmset $F8 $10

tbmset $EA b00000000 Delays
tbmset $FA b00000000

tbmset $EC $00    Temp measurement control
tbmset $FC $00

mdelay 100

--- setup all ROCs --------------------------

select :

dac   1    8  Vdig 
dac   2  140  Vana
dac   3   40  Vsf
dac   4   12  Vcomp

dac   7   30  VwllPr
dac   9   30  VwllPr
dac  10  117  VhldDel
dac  11    1  Vtrim
dac  12   40  VthrComp

dac  13   30  VIBias_Bus
dac  14    6  Vbias_sf
dac  22   99  VIColOr

dac  15   60  VoffsetOp
dac  17  150  VoffsetRO
dac  18   45  VIon

dac  19   50  Vcomp_ADC
dac  20   70  VIref_ADC

dac  25  170  Vcal
dac  26  122  CalDel

dac  253   4  CtrlReg
dac  254  50  WBC    // tct - 6
dac  255  12  RBreg

flush

mask

getid
getia

--- setup probe outputs ---------------------
d1 9  sync
a1 1  sdata

--- setup readout timing --------------------
- pixel_dtb.h:	#define PG_RESR  0x0800 // ROC reset
- pixel_dtb.h:	#define PG_REST  0x1000 // TBM reset

pgstop
pgset 0 b100000  20  pg_sync (stretch after sync)
pgset 1 b010000  16  pg_rest (reset TBM)
pgset 2 b000100  56  pg_cal (WBC + 6 for digV2)
pgset 3 b000010   0  pg_trg (delay zero = end of pgset)
pgsingle

dopen   8500100  0  [daq_open]
dopen   8500100  1  [daq_open]

flush

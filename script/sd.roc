
dtb

--- set divgV2 ROC DACs-----------------------------

dac   1   15  Vdig  needed for large events
dac   2  130  Vana  optia 25
dac   3  240  Vsf   linearity
dac   4   12  Vcomp

dac   7   30  VwllPr
dac   9   30  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   40  VthrComp

dac  13    1  VIBias_Bus # against DCF
dac  14   14  Vbias_sf
dac  22   20  VIColOr    # against DCF

dac  15   50  VoffsetOp
dac  17  140  VoffsetRO
dac  18   45  VIon

dac  19   40  Vcomp_ADC
dac  20   70  VIref_ADC

dac  25  222  Vcal
dac  26  133  CalDel

dac 253    4  CtrlReg
dac 254  139  WBC (above 139: only one response per trig in caldel test)

flush

show

mask

getvd
getva
getid
getia

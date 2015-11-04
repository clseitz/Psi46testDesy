
# bootstrap ROC: set DACs

optia 24  # set Vana for target Ia [mA]

ctl 4
caldel 22 33  # set CalDel (from one pixel)

ctl 0
effdac 22 33 25  # one pix threshold, shows start value

vthrcomp 36 55  # set global threshold to min 36, start 55

caldelroc

trim 30 36      # trim to 30, start guess 36

ctl 4
caldelroc

effmap 100     #

-- check trim for low eff pixels:

trimbits

-- pixi col row
-- pixt col row trim

thrmap 30

ctl 4
tune 22 33 # set gain and offset

phmap 100

show
ctl 4
wdac ia25-trim30
wtrim ia25-trim30

-- gain cal:

ctl 4
dacscanroc 25 16

ctl 0
dacscanroc 25 16

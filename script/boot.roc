
# bootstrap ROC: set DACs

optia 25 # set Vana for target Ia [mA]

allon
vthrcomp # Id vs VthrComp: noise peak
mask

ctl 4

caldel 2 2 # set CalDel (from one pixel)

# noisemap # set VthrComp below noise

tune  # set gain and offset

#ctl 0
#thrmap

show

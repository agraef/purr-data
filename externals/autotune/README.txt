An auto-tuning PD External, based on 
autotalent an auto-tuning LADSPA plugin and an older port by Maxus Germanus

Autotalent v0.2
Free software (GPLv2) by Thomas A. Baran.
http://tombaran.info/autotalent.html

Ivica Ico Bukvic <ico.bukvic.net>
VERSION 0.9.1
changes:
    *added ability to specify FFT size
    *added ability to specify hop (window) size--requires further refinement
     to allow for more nimble pitch readjustment
    *added pull 2 value where pitch pull is relative to the closest pitch
    *circumvented segfaults due to NANs--may require a better fix

VERSION 0.9
changes:
    *compatible with the latest version of autotalent

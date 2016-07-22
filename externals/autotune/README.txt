An auto-tuning PD External, based on 
autotalent an auto-tuning LADSPA plugin and an older port by Maxus Germanus

Free software by Thomas A. Baran.
http://web.mit.edu/tbaran/www/autotune.html
VERSION 0.2

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

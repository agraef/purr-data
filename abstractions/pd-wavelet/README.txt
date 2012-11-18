::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:::                                                            :::
:::         MANIPULATION OF AUDIO IN THE WAVELET DOMAIN        :::		
:::            PROCESSING A WAVELET STREAM UING PD             :::
:::                                                            :::
:::		Raul Diaz Poblete [IEM] 2005-2006              :::
:::                                                            ::: 
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



That's a software for audio modification in real time with Miller 
Puckette's Pure Data (PD) 
(http://crca.ucsd.edu/~msp/software.html),
based on wavelet analysis-resynthesis using externals dwt~ and 
idwt~ from Tom Schouten's creb library 
(http://zwizwa.fartit.com/pd/creb/).


This patch was created at IEM (Institut für Elektronische Musik) 
in Graz (Austria) in 2005-2006 as part of a work for Toningenieur
Diplomarbeit supervised by Winfried Ritsch.



USE INSTRUCIONTS:
...................................................................

In order to run this software, you will need PD (for w32-platforms 
this is already included in this package).


WINDOWS:

 - Run the bat file start_pd.bat


LINUX:

 - Edit the skript start_pd.sh to specify your pd path.

 - Run the skript start_pd.sh



MORE INFORMATION:
...................................................................

Look at the documentation of my Diplomarbeit at IEM:

 - http://iem.at/projekte/dsp/wavelet/project_view

or at my home on PD Community Site:

 - https://puredata.org/Members/raul/project_patch/view



LICENCE
....................................................................

Copyright (c) 2005-2006 Raul Diaz.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by 
the Free Software Foundation; either version 2 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful,but 
WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
General Public License for more details.


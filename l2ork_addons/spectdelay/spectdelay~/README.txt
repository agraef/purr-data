spectdelay~ is a PD port of John Gibson's jg.spectdelay~ v.0.9 a granulation object for Max5 (John Gibson <johgibso@gmail.com> www.john-gibson.com).

Port by Ivica Ico Bukvic (<ico@vt.edu> ico.bukvic.net).

The source code is licensed under the GNU General Public License
(see "COPYING").

December 30, 2010: initial release.

INSTALL INSTRUCTIONS

1) edit linux-install.sh include paths so that they point to the right place where Pd includes are stored, namely: 
	-I/usr/local/include/pdl2ork part

2) run ./linux-install.sh

3) copy

	spectdelay~.pd_linux
	spectdelay~-help.pd
	array2list.pd
	arrayreset.pd

   to your externals folder (or any other path visible by Pd).

Cheers!


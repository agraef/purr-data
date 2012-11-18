syntax match pdLine0 /^#/
syntax match pdLine1 /;$/

syntax match pd_obj /X\>/
syntax match pd_win /N\>/
syntax match pd_arr /^#A\>.*/

set iskeyword+=~
set iskeyword+=+
set iskeyword+=-
set iskeyword+=*
set iskeyword+=/
set iskeyword+=%
set iskeyword+==
set iskeyword+=!
set iskeyword+=&
set iskeyword+=|
set iskeyword+=<
set iskeyword+=>
" syntax match pd_sigsym /\~/

"syntax match pd_xy /\(obj \|floatatom \|symbolatom \|msg \|pd \)\zs\(\d\+\)\{2}/
syntax match pd_xy /\zs\<\(\d\+\)\>/

syntax keyword pd_elem canvas obj pd
syntax keyword pd_conn connect restore coords

syntax keyword pd_build namecanvas msg text array table floatatom symbolatom inlet outlet inlet~ outlet outlet~ empty
syntax keyword pd_gui cnv nbx tgl bng vsl hsl vradio hradio vu
syntax keyword pd_ds pointer get set element getsize append sublist template struct plot drawnumber drawcurve filledcurve drawpolugon filledpolygon
syntax keyword pd_tab tabread tabwrite soundfiler tabread4 tabread~ tabwrite~ tabread4~ tabplay~ tabrecieve~ tabsend~ tabosc4~
syntax keyword pd_misc declare loadbang textfile qlist key keyup keyname openpanel savepanel
syntax keyword pd_timnum delay metro line timer cputime realtime pipe bag poly
syntax keyword pd_msgop f i s b float int symbol t trigger symbol send receive sel select route pack unpack spigot moses until print change swap value makefilename bang

syntax keyword pd_sig osc~ phasor~ noise~ cos~ delwrite~ delread~ vd~ vcf~ lop~ bp~ hip~

syntax keyword pd_amp line line~ vline~ snapshot~ vsnapshot~ env~ treshhold~
syntax keyword pd_raw biquad~ rpole~ rzero~ rzero_rev~ cpole~ czero~ czero_rev~ samphold~
syntax keyword pd_func clip max min random atan atan2 sqrt log exp abs mod div sin cos tan pow
syntax keyword pd_sigfunc +~ -~ *~ /~ max~ min~ clip~ q8_rsqrt~ q8_sqrt~ wrap~ fft~ ifft~ rfft~ rifft~ framp~
syntax keyword pd_sigop switch~ block~ adc~ dac~ samplerate~ sig~ bang~  send~ receive~ trow~ catch~ readsf~ writesf~ print~
syntax keyword pd_amp line line~ vline~ snapshot~ vsnapshot~ env~ treshhold~
syntax keyword pd_raw biquad~ rpole~ rzero~ rzero_rev~ cpole~ czero~ czero_rev~ samphold~ expr~ fexpr~
syntax keyword pd_math expr + - * / == != > < >= <=
syntax match pd_bitw /\( &&\| &\| ||\| |\| %\)\( \|;\)/
syntax keyword pd_midi notein ctlin pgmin bendin touchin polytouchin midiin sysexin noteout ctlout pgmout bandout touchout polytouchout midiout mknote stripnote
syntax keyword pd_conv mtof mtof~ ftom~ ftom~ powtodb~ powtodb~ rmstodb~ rmstodb~ dbtopow~ dbtopow~ dbtorms~ dbtorms~ rmstopow~ rmstopow~ powtorms~ powtorms~






hi link pdLine0 Special
hi link pdLine1 Special
hi link pd_win Error
hi link pd_obj PreProc
hi link pd_arr Constant
hi link pd_xy Identifier

hi link pd_elem Underlined
hi link pd_conn Special
hi link pd_build Todo
hi link pd_gui Constant
hi link pd_ds Type
hi link pd_tab Type
hi link pd_misc Macro
hi link pd_timnum Special

hi link pd_func Special
hi link pd_sigfunc Statement
hi link pd_sigop Error
hi link pd_sig Error
hi link pd_amp Type
hi link pd_raw Type
hi link pd_math Statement
hi link pd_bitw Debug
hi link pd_midi Identifier
hi link pd_conv Statement

" hi link pd_sigsym Debug

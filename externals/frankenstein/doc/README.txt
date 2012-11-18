***README***

Vatic, davide, please add notes and thoughts here for now...

****Ruby/Gridflow instructions***

In order to use some of the objects in the test patches, for now just [random_choice] within [manager] in the chord_melo_test patches, you will need GridFlow and Ruby installed.

Please find instructions here:

http://gridflow.ca/latest/doc/install.html

Once you have Ruby 1.8.0 or above installed, and GridFlow is happily starting in Puredata, you should see the following at PD startup:

setting up Ruby-for-PureData...
we are using Ruby version 1.8.1
[gf] This is GridFlow 0.8.1 within Ruby version 1.8.1

Or, something similar.

Now, you will need to copy ruby/.gridflow_startup to your home directory. If it is somewhere else than /home/you/ or /Users/you, you can always load it from pd by using the Ruby console input (see pd's console window, and under IN - OUT), by doing:

load "/path/to/.gridflow_startup

You're now able to write puredata objects in Ruby!



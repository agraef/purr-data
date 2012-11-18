example 2: a more complex use of ann_mlp

GEM and pmpd are needed to run this example

start example2.pd

start GEM rendering. 
see how the cursor is locked to the great red ball 
and how you can push the 6 small balls.
each small ball has associated a sine wave
its volume = its velocity

if you want to train the net using different meanings from mine
you can write a testfile or train the net on the fly or both 
(train on file then refine on the fly)

training on the fly is simpler

training is a bit tricky but might be more efficient (less error in the nn)

to write a testfile:
1)toggle ON in [pd write trainfile]
2)record a list of patterns
3)toggle OFF when you are ready
4)write to a file
5)edit the file adding a line with 3 integers
6)change training parameters (from the defaul to more flexible ones)
I now suggest to lower pd's process priority to normal 
because the training process blocks pd and may slow down your computer
7) start trainign from file. this will take a long time, 
wait for the message in console for the training to be completed
8) now you can activate the metro and run the net and see how it responds.

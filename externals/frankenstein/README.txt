frankenstein set of externals

authors:
dmorelli: www.davidemorelli.it
pland: www.davidcasal.com
with the help of vatic and using soundspotter code (copyright 2006 Michael Casey)

copyright (c) 2006 Davide Morelli and David Plans Casal

last update: 
14-10-2005

------------------- what is this?

this is an attempt to build an intelligent system for realtime 
improvisation, a system that follows a human player and proposes
rhythm, melody, chords sequences, formal structure.
it learns the style in realtime.
it uses various AI techniques: GA, searchs, ANN (maybe)

description of files:

chord_melo.c
a GA melody generator. it takes a melody as input and evolves it over time.
uses co-evolutionary techniques (Todd)

chords_memory.c
it is a directional graph which implements a memory of the played 
chords sequences, you can train it...
and once trained you can ask questions like:
in C major, from a D minor 7h where did I go most if the times?
or
in C major, from a D minor 7h build a chord sequence to bring me
in D major in 4 steps using the chords sequences I used most

harmonizer.c
a GA external that build choir voicing for 5 voices: 
you pass it the midi value of each voice, starting chord, next chord
and it outputs a list with the midi values of each voice.
avoids hidden 8ves and 5ths.


folders:

doc/
implementation documents, notes, logs, ideas, etc..
patches/
example patches, help patches, some used in performances, etc..
aima/
python patches not yes used but interesting for agents
ruby/
gridflow patches not yet used
old/
old not used code
test/
testing code, not to be used
backup/
previous versions


TODO:

gluer/solderer
an external that takes the input of two or more chord_melody,
a chord sequence and glues the melody statemets together
to build a long complete melody.

form_manager
an external that manages the form and structure of the piece:
decide which melody statement to use, the chord sequence to ask for, etc..

various objects to detect played notes

diatonic_melody.c
same as chord_melody but without chord reference, only scale used
(for post-tonal music non based on chords)

chromatic_melo.c
same as diatonic_melo but without any reference (for atonal music)

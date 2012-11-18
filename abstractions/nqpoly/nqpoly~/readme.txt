not-quite-poly

polyphonic synthesis for pd.

how do i use it?

first you need to make an instrument. you do this by making an abstraction (or an external, if you are so inclined) that follows these rules:

it has two inlets, and two outlets.

the first inlet, takes a list. these are the parameters that are used to trigger an instance of your "instrument". the parameters can be anything at all, but you would normally specify some things like duration of note, and frequency etc.

the second inlet is optional. it receives a bang when the instument has finished loading. yes that's right, just like loadbang. the reason for having this inlet is because there is a 'bug', that will stop loadbangs from working inside your polyphonic instrument. when i say optional, i mean you will get some errors at creation time of the nqpoly~ object if you don't have it, but everything else should work.

the first outlet is the signal output from your instrument. your instrument should only start generating a signal on this outlet (other than 0) in response to a trigger on it's first input.

the second outlet should send out a bang when your instrument has finished playing it's current note, and is ready to receive another trigger.

your instrument can also accept up to 5 creation arguments. they must all be symbols (not floats, unfortunately). the intended use of the creation arguments is as the prefix for receives, so that you can control all of your instantiated instruments as a group.

once you have an instrument, you can build a polyphonic version of it with the nqpoly object. for example

[nqpoly~ 5 myinst param]

this will make a version of myinst that has a maximum of 5 voices, with the creation parameter "param".

sending a list to the first input of nqpoly~ will start one of the voices, and the signal will appear on the first output. sending another list while the first voice is still playing will start the next voice, and it's output will be summed with the first voice and both signals will be heard on the first output. if all of the voices are exausted, the list you supplied will appear on the second output.

there is an example instrument called grain~ which takes a two element list specifying the frequency (in hertz) and the duration (in milliseconds) of the note. on my machine i can just manage to use about 100 voices of this instrument before i start getting pops and clicks. i think that's pretty damned impressive :) it also means that this can be used for granular synthesis as well as simple polyphonic synthesis. 

the nqpoly~-test.pd patch shows an example using grain~. for the brave, you can look at the other test patches, and the innards of nqpoly~ and nqwrap~ to see how it works. it's mostly white magic tho :)

oh one last thing... some of the test patches use some the "tgl" object from iemlib... it's just to toggle the metro so it should be easy to change if you don't have/use iemlib. also some of the layout might be strange, i use hacked fonts in my pd.tk...

pix.test.at

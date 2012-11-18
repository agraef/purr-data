VST~

reference: http://iem.kug.ac.at/~jams/
this plugin uses a tiny amount of code from the plugin~
object mentioned above. It also tries to replicate the 
syntax

setup

Set environment var to locate your plugins: 

set VST_PATH=C:\Program Files\Steinberg\Vstplugins\Plugins archive;C:\Program Files\Steinberg\Vstplugins\VSTPlugs;C:\audio\Cubase5\Vstplugins\VstPlugins;C:\Program Files\Steinberg\Vstplugins

add the following line to your PD startup (editing the path to 
suit) 

-lib \VST\Debug\vst

There are three example patches - using Waldorf D-pole, Pro-52
(helpfully called neontest) and Crazy Diamond http://rumpelrausch.de.vu/

notes:

In order to support plugins with spaces in the name it takes all 
the parameters it is given and assumes they are parts of the 
name with spaces in between. Thus 

	vst~ waldorf d-pole 

loads the d-pole plugin because it looks for "waldorf d-pole". The 
VST_PATH environment variable is use just like for "Plugin~". 
The two right most inputs are for midi note and velocity and 
work just like the "noteout" object. 

There is not currently any support for: 

	programs
	other midi messages 
	help file

 I am in the process of adding all of these. The editor does 
actually work but the window it is in breaks PD so it is disabled 
at present. There are no plans for a graphical interface for 
plugins with no supplied editor - what do you think PD is for ;-)

I have discovered that not all VST plugins behave properly 
- for example I found one called VST_Chopper that crashes 
PD if you print its parameters. 

New Stuff: 

There are a number of "plugins" that come with Cubase (truetape, double delay etc).
Despite being in a folder called VSTPLugins these are NOT VST plugins but 
Cubase extentsions. Steinberg themselves told me this. 

There is now support for the built in editor. Send an Edit message to the 
plugin to see the graphical editor. 

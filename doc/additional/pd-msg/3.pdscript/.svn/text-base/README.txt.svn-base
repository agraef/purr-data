This is a short example that shows how to "script" a Pd patch with pd commands.
You will need to have 'pdsend' installed, which is a command line 
tool for sending messages to Pd.  There is one included in this
directory which might work for you.

First start Pd with the patch "lispg.pd".  It has a [netreceive 3005]
object in it which allows the patch to receive messages on port 3005.

# pd -open lisp.pd

Then either start the automated shell script:

# sh ./test.sh

or pipe the Pd commands to 'pdsend' directly:

# pdsend 3005 < test.txt

Read through test.txt for further explanations.

Guenter

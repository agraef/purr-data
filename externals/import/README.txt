This object loads libraries and libdirs from within a patch. It is the first
small step towards a patch-specific namespace.  It is a reimplementation of a
similar/same idea from Guenter Geiger's [using] object.

Aiming to provide a simplified Python-style import for Pure Data, this [import]
object loads libraries as part of a patch.  It will load anything that Pd
considers a library, including libraries that are defined by Pd loaders like
'libdir'.

[import] allows you to manipulate load library into the patch-local path from
within a Pd patch itself.  This is a very simple version that does not need
any modifications to the core of Pd.  Therefore, you should make [import] the
first object you create in your patch.

To install, copy the "import" folder into your user Pd folder.  You
can find out more here:
http://puredata.info/docs/faq/how-do-i-install-externals-and-help-files-with-pd-extended

For more info on the structure of libdirs, see this webpage:
http://puredata.org/docs/developer/Libdir


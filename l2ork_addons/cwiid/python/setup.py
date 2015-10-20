from distutils.core import setup, Extension

setup(name='cwiid',
	version='0.7.00',
	ext_modules=[Extension('cwiid', ['cwiidmodule.c', 'Wiimote.c'])]
	)

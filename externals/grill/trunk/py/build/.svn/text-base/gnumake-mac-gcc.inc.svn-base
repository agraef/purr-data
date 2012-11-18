# don't use -framework Python, since this will stick to the default system version

_LOCAL_FRAMEWORK := /Library/Frameworks/Python.framework/Versions/$(PY_MAJOR_VERSION).$(PY_MINOR_VERSION)
_SYSTEM_FRAMEWORK := /System/Library/Frameworks/Python.framework/Versions/$(PY_MAJOR_VERSION).$(PY_MINOR_VERSION)
_LOCAL_LIBRARY := /Library/Python/$(PY_MAJOR_VERSION).$(PY_MINOR_VERSION)
_SYSTEM_LIBRARY := /System/Library/Python/$(PY_MAJOR_VERSION).$(PY_MINOR_VERSION)

DEFS += -DPY_EXPORTS
INCPATH += -F/Library/Frameworks -framework Python

ifeq ($(PY_DEFAULT),1)
LIBS += $(_SYSTEM_FRAMEWORK)/Python
INCPATH += -I$(_SYSTEM_FRAMEWORK)/Headers
else
LIBS += $(_LOCAL_FRAMEWORK)/Python
INCPATH += -I$(_LOCAL_FRAMEWORK)/Headers
endif

ifdef PY_NUMARRAY
DEFS += -DPY_NUMARRAY
endif
ifdef PY_NUMPY
DEFS += -DPY_NUMPY
INCPATH += -I$(_LOCAL_LIBRARY)/python$(PY_MAJOR_VERSION).$(PY_MINOR_VERSION)/site-packages/numpy/core/include
ifeq ($(PY_DEFAULT),1)
INCPATH += -I$(_SYSTEM_FRAMEWORK)/lib/python$(PY_MAJOR_VERSION).$(PY_MINOR_VERSION)/site-packages/numpy/core/include
INCPATH += -I$(_SYSTEM_FRAMEWORK)/Extras/lib/python/numpy/core/include
else
INCPATH += -I$(_LOCAL_FRAMEWORK)/lib/python$(PY_MAJOR_VERSION).$(PY_MINOR_VERSION)/site-packages/numpy/core/include
endif
endif
ifdef PY_NUMERIC
DEFS += -DPY_NUMERIC
endif

ifdef PY_USE_GIL
DEFS += -DPY_USE_GIL
endif

ifdef PY_USE_INOFFICIAL
DEFS += -DPY_USE_INOFFICIAL
endif

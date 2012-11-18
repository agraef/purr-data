a = Analysis(['../../support/_mountzlib.py',
              '../../support/useUnicode.py',
              'src/gripd.py'],
              pathex=[])
pyz = PYZ(a.pure)
exe = EXE(pyz,
          a.scripts + [('OO','','OPTION')] + [('f','','OPTION')],
          exclude_binaries=1,
          name='buildgripd/gripd',
          debug=0,
          strip=1,
          console=1 )
coll = COLLECT( exe,
               a.binaries,
               strip=1,
               name='distgripd')

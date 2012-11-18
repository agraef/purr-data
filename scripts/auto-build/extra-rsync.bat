REM uses http://setacl.sourceforge.net/

cd \msys\1.0\home\pd\

REM echo y|cacls c:\msys\1.0\home\pd\auto-build /C /T /G pd:F everyone:R 

setacl -on c:\msys\1.0\home\pd\auto-build -ot file -actn ace -ace "n:pd;p:full,write_owner;i:so,sc;m:set" -ace "n:everyone;p:read;i:so,sc;m:set"

REM Cygwin rsync seems to be unhappy with SVN's .svn file permissions, so
REM ignore SVN files first to get all the 'meat'
rsync -av --progress --whole-file --exclude='*inv\**' --cvs-exclude --timeout=60 rsync://128.238.56.50/distros/pd-extended/ /home/pd/auto-build/pd-extended/
rsync -av --progress --whole-file --exclude='*inv\**' --cvs-exclude --timeout=60 rsync://128.238.56.50/distros/pd-devel/ /home/pd/auto-build/pd-devel/

sleep 60

REM now get the SVN changes, this might fail a lot, especially on '.svn/entries'
rsync -av --progress --whole-file --delete-before --exclude='*inv\**' --timeout=60 rsync://128.238.56.50/distros/pd-extended/ /home/pd/auto-build/pd-extended/
rsync -av --progress --whole-file --delete-before --exclude='*inv\**' --timeout=60 rsync://128.238.56.50/distros/pd-devel/ /home/pd/auto-build/pd-devel/

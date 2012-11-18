#This is the xgui makefile.

current:
	cd xgui-lib ; make pd_linux ; cd ..

uninstall:

# bin
	rm -f /usr/local/bin/xgui
	rm -f /usr/local/bin/pdx
# main
	rm -f /usr/local/lib/pdx/main/*
	rmdir /usr/local/lib/pdx/main
# patch4pdx
	rm -f /usr/local/lib/pdx/patch4pdx/*
	rmdir /usr/local/lib/pdx/patch4pdx
# lib
	rm -f /usr/local/lib/pdx/*
	rmdir  /usr/local/lib/pdx
# doc
	rm -f /usr/local/lib/pd/doc/xgui/*
	rmdir /usr/local/lib/pd/doc/xgui



install:

# bin
	install ./bin/xgui.sh /usr/local/bin/xgui
	install ./bin/pdx.sh /usr/local/bin/pdx

# lib
	mkdir -p /usr/local/lib/pdx

# main
	mkdir -p /usr/local/lib/pdx/main
	install ./main/* /usr/local/lib/pdx/main/

# patch4pdx
	mkdir -p /usr/local/lib/pdx/patch4pdx

	install ./adapters_in/* /usr/local/lib/pdx/patch4pdx/
	install ./adapters_out/* /usr/local/lib/pdx/patch4pdx/
	install ./behaviors/* /usr/local/lib/pdx/patch4pdx/
	install ./filters/* /usr/local/lib/pdx/patch4pdx/
	install ./objects/* /usr/local/lib/pdx/patch4pdx/
	install ./utils/* /usr/local/lib/pdx/patch4pdx/
	install ./physics/* /usr/local/lib/pdx/patch4pdx/

# xgui-lib
	install ./bin/*.pd_linux /usr/local/lib/pd/extra/

# doc
	mkdir -p /usr/local/lib/pd/doc/xgui/
	install ./doc/* /usr/local/lib/pd/doc/xgui/

archive:
	cd ../;tar -cf xgui.tar xgui
	gzip ../xgui.tar

4linux:
	chmod 774 pdx.sh
	chmod 774 bin/pdx.sh
	chmod 774 bin/xgui.sh

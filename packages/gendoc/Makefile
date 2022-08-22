# These are just dummy values for testing purposes, the parent Makefile should
# override them.
version = 2.17.0
build_version = 20220809-rev.35c9bcab

targets = $(addprefix ReadMe-, Linux Darwin MINGW) \
$(addprefix Welcome-, Linux Darwin MINGW)
markdown = $(addsuffix .md, $(targets))
html = $(addsuffix .html, $(targets))

# python -m markdown setup. We may need to invoke python as python3 on systems
# which still have python2 as the default. You can also set this manually by
# passing python-markdown=python-markdown-command to make if python-markdown
# gets invoked in an unusual way.
python-markdown = $(shell python3 -m markdown 2>/dev/null < /dev/null && echo python3 -m markdown || echo python -m markdown)

# Note: This requires special tools (python-markdown, awk).
all: html

markdown: $(markdown)
html: $(html)

ReadMe-%.md: osname = $(patsubst ReadMe-%.md,%,$@)
ReadMe-%.md: ReadMe.md
	awk -f gendoc.awk osname="$(osname)" version="$(version)" build_version="$(build_version)" < $< > $@

ReadMe-%.html: osname = $(patsubst ReadMe-%.html,%,$@)
ReadMe-%.html: ReadMe-%.md pandoc-template.html
	$(python-markdown) $< > ReadMe-body.html
	sed -e "s/@version@/$(version) ($(osname))/g" -e '/--stylesheet--/r gh-pandoc.css' -e '/--body--/r ReadMe-body.html' pandoc-template.html > $@
	rm -f ReadMe-body.html

Welcome-%.md: osname = $(patsubst Welcome-%.md,%,$@)
Welcome-%.md: Welcome.md
	awk -f gendoc.awk osname="$(osname)" version="$(version)" build_version="$(build_version)" < $< > $@

Welcome-%.html: osname = $(patsubst Welcome-%.html,%,$@)
Welcome-%.html: Welcome-%.md pandoc-template.html
	$(python-markdown) $< > Welcome-body.html
	sed -e "s/@version@/$(version) ($(osname))/g" -e '/--stylesheet--/r gh-pandoc.css' -e '/--body--/r Welcome-body.html' pandoc-template.html > $@
	rm -f Welcome-body.html

clean:
	rm -f $(markdown) $(html)

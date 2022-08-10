# These are just dummy values for testing purposes, the parent Makefile should
# override them.
version = 2.17.0
build_version = 20220809-rev.35c9bcab

targets = $(addprefix ReadMe-, Linux Darwin MINGW) \
$(addprefix Welcome-, Linux Darwin MINGW)
markdown = $(addsuffix .md, $(targets))
html = $(addsuffix .html, $(targets))

# Note: This requires special tools (pandoc, awk).
all: html

# NOTE: In pandoc 2.19, --self-contained, which forces the css to be included
# in the html output, is deprecated in favour of --embed-resources. But the
# new option isn't supported in pandoc 2.18, which is still being used a
# lot. Brilliant. For now we just live with the deprecation warning. You can
# always set different options using the self-contained variable from the
# command line if needed.

# >=2.19:
#self-contained = --standalone --embed-resources

# <=2.18:
self-contained = --self-contained

markdown: $(markdown)
html: $(html)

ReadMe-%.md: osname = $(patsubst ReadMe-%.md,%,$@)
ReadMe-%.md: ReadMe.md
	awk -f gendoc.awk osname="$(osname)" version="$(version)" build_version="$(build_version)" < $< > $@

ReadMe-%.html: osname = $(patsubst ReadMe-%.html,%,$@)
ReadMe-%.html: ReadMe-%.md
	pandoc $< -o $@ -M pagetitle="Purr Data $(version) ($(osname))" $(self-contained) --css gh-pandoc.css

Welcome-%.md: osname = $(patsubst Welcome-%.md,%,$@)
Welcome-%.md: Welcome.md
	awk -f gendoc.awk osname="$(osname)" version="$(version)" build_version="$(build_version)" < $< > $@

Welcome-%.html: osname = $(patsubst Welcome-%.html,%,$@)
Welcome-%.html: Welcome-%.md
	pandoc $< -o $@ -M pagetitle="Purr Data $(version) ($(osname))" $(self-contained) --css gh-pandoc.css

clean:
	rm -f $(markdown) $(html)

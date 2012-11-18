REST = rradicalpd

all: html tex pdf 

tex:
	rest2latex --documentclass=scrartcl $(REST).rst $(REST).tex

pdf: tex
	pdflatex $(REST).tex

html:
	rest2html $(REST).rst $(REST).html


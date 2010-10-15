
include LaTeX.mk

all: pdf

e :
	evince main.pdf &

x xfig :
	cd fig; xfig &

$(patsubst %.svg,%.pdf,$(wildcard fig/*svg)): %.pdf : %.svg
	inkscape -z -D -A $@ $<
 

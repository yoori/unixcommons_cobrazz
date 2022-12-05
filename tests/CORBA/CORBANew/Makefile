.PHONY: all clean check


all clean check debug:
	@for d in `find . -mindepth 1 -maxdepth 1 -type d -a -name '[^.]*' | grep -v narrow`; do $(MAKE) -C $$d $(MAKECMDGOALS) || exit 1; done

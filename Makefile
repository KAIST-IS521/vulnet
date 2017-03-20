CFLAGS = -Wall
BUILDDIR = build

all: $(BUILDDIR) $(BUILDDIR)/vulnet

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/vulnet: vulnet.c $(BUILDDIR)
	$(CC) $(CFLAGS) -o$@ $<

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean

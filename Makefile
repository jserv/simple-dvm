VMS = jvm/jvm dvm/dvm

all: $(VMS)

jvm/jvm:
	$(MAKE) -C jvm

dvm/dvm:
	$(MAKE) -C dvm

clean:
	$(MAKE) -C jvm clean
	$(MAKE) -C dvm clean
	$(RM) .output-jvm .output-dvm

check: $(VMS)
	jvm/jvm tests/Foo1.class > .output-jvm
	dvm/dvm tests/Foo1.dex > .output-dvm
	@diff -u .output-jvm .output-dvm && echo "OK!" || echo "ERROR: different results"

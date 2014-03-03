simple_jvm/jvm:
	$(MAKE) -C simple_jvm

simple_dvm/dvm:
	$(MAKE) -C simple_dvm

clean:
	$(MAKE) -C simple_jvm clean
	$(MAKE) -C simple_dvm clean

check: simple_jvm/jvm simple_dvm/dvm
	simple_jvm/jvm tests/Foo1.class
	simple_dvm/dvm tests/Foo1.dex


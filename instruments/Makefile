all:: install

install::
	cd $(DIRECT_SYM_ROOT)/instruments/rename-exit && make install
	cd $(DIRECT_SYM_ROOT)/instruments/lower-intrinsic && make install
	cd $(DIRECT_SYM_ROOT)/instruments/dyn-intercept-calls && make install
	cd $(DIRECT_SYM_ROOT)/instruments/print-callgraph && make install
	cd $(DIRECT_SYM_ROOT)/instruments/track-branch && make install && make func

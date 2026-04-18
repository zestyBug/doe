all: main

test1:
	$(MAKE) -f ./makefile.mk $@ DEBUG=1
test2:
	$(MAKE) -f ./makefile.mk $@ DEBUG=1
test3:
	$(MAKE) -f ./makefile.mk $@ DEBUG=1
test4:
	$(MAKE) -f ./makefile.mk $@ DEBUG=1
test5:
	$(MAKE) -f ./makefile.mk $@ DEBUG=1
test6:
	$(MAKE) -f ./makefile.mk $@ DEBUG=1
test7:
	$(MAKE) -f ./makefile.mk $@ DEBUG=1
dmain:
	$(MAKE) -f ./makefile.mk main DEBUG=1
main:
	$(MAKE) -f ./makefile.mk $@



test-%:
	$(MAKE) -f ./test-v2/makefile.mk $@

clean:
	@echo "Cleaning up..."
	@$(RM) -rf obj bin

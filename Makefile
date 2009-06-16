
all: FvTest

clean depend generated realclean $(CUSTOM_TARGETS):
	@$(MAKE) -f Makefile.FvTest $(@)

.PHONY: FvTest
FvTest:
	@$(MAKE) -f Makefile.FvTest generated all

project_name_list:
	@echo FvTest

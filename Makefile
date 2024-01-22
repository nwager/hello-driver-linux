#!/usr/bin/make -f

MODULES := hello

all: $(MODULES)

$(MODULES):
	$(MAKE) -C "$@"

CLEANMODS := $(addprefix clean-,$(MODULES))
$(CLEANMODS):
	$(MAKE) -C "$(subst clean-,,$@)" clean
clean: $(CLEANMODS)

LOADMODS := $(addprefix load-,$(MODULES))
$(LOADMODS):
	$(MAKE) -C "$(subst load-,,$@)" load
load: $(LOADMODS)

UNLOADMODS := $(addprefix unload-,$(MODULES))
$(UNLOADMODS):
	$(MAKE) -C "$(subst unload-,,$@)" unload
unload: $(UNLOADMODS)

INSTALLMODS := $(addprefix install-,$(MODULES))
$(INSTALLMODS):
	$(MAKE) -C "$(subst install-,,$@)" install
install: $(INSTALLMODS)

UNINSTALLMODS := $(addprefix uninstall-,$(REVMODS))
$(UNINSTALLMODS):
	$(MAKE) -C "$(subst uninstall-,,$@)" uninstall
uninstall: $(UNINSTALLMODS)

.PHONY: all clean $(MODULES) $(CLEANMODS) $(LOADMODS) $(UNLOADMODS) $(INSTALLMODS) $(UNINSTALLMODS)


include scripts/Makefile

modules_make  = $(MAKE) -C $(1);
modules_clean = $(MAKE) clean -C $(1); 

.PHONY: all mm mc clean

all: mm

#mm:
#	@ $(foreach n,$(Modules),$(call modules_make,$(n)))
#mc:
#	@ $(foreach n,$(Modules),$(call modules_clean),$(n)))


mm:
	 $(foreach n,$(Modules),$(call modules_make,$(n)))

mc : 
	 $(foreach n,$(Modules),$(call modules_clean,$(n)))


clean : mc
	$(RM) bin/* lib/*

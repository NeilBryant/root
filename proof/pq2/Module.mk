# Module.mk for pq2 module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: G. Ganis, 2010

MODNAME      := pq2
MODDIR       := $(ROOT_SRCDIR)/proof/$(MODNAME)
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

PQ2DIR       := $(MODDIR)
PQ2DIRS      := $(PQ2DIR)/src
PQ2DIRI      := $(PQ2DIR)/inc

##### pq2 #####
PQ2H         := $(wildcard $(MODDIRI)/*.h)
PQ2S         := $(wildcard $(MODDIRS)/*.cxx)
PQ2O         := $(call stripsrc,$(PQ2S:.cxx=.o))
PQ2DEP       := $(PQ2O:.o=.d)
PQ2          := bin/pq2

##### Libraries needed #######
PQ2LIBS      := -lProof -lHist -lMatrix -lTree \
                -lRIO -lNet -lThread -lMathCore $(BOOTLIBS) 
PQ2LIBSDEP    = $(BOOTLIBSDEP) $(IOLIB) $(NETLIB) $(HISTLIB) $(TREELIB) \
                $(MATRIXLIB) $(MATHCORELIB) $(PROOFLIB) $(THREADLIB)

# used in the main Makefile
PQ2H_REL     := $(patsubst $(MODDIRI)/%.h,include/%.h,$(PQ2H))
ALLHDRS      += $(PQ2H_REL)
ALLEXECS     += $(PQ2)
ifeq ($(CXXMODULES),yes)
  CXXMODULES_HEADERS := $(patsubst include/%,header \"%\"\\n,$(PQ2H_REL))
  CXXMODULES_MODULEMAP_CONTENTS += module Proof_$(MODNAME) { \\n
  CXXMODULES_MODULEMAP_CONTENTS += $(CXXMODULES_HEADERS)
  CXXMODULES_MODULEMAP_CONTENTS += "export \* \\n"
  CXXMODULES_MODULEMAP_CONTENTS += // link no-library-created \\n
  CXXMODULES_MODULEMAP_CONTENTS += } \\n
endif

# include all dependency files
INCLUDEFILES += $(PQ2DEP)

##### local rules #####
.PHONY:         all-$(MODNAME) clean-$(MODNAME) distclean-$(MODNAME)

include/%.h:    $(PQ2DIRI)/%.h
		cp $< $@

$(PQ2):         $(PQ2O) $(PQ2LIBSDEP)
		$(LD) $(LDFLAGS) -o $@ $(PQ2O) $(RPATH) $(PQ2LIBS) $(SYSLIBS)

all-$(MODNAME): $(PQ2)

clean-$(MODNAME):
		@rm -f $(PQ2O)

clean::         clean-$(MODNAME)

distclean-$(MODNAME): clean-$(MODNAME)
		@rm -f $(PQ2DEP) $(PQ2)

distclean::     distclean-$(MODNAME)

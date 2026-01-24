#
# @(#)GNUmakefile	1.131 02/10/03 @(#)
#
# Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
# PROPRIETARY/CONFIDENTIAL
# Use is subject to license terms.
#
#########
#
# Makefile for building and packaging all of the MID Profile.
#
#
# Most frequently used targets:
#
#    all         -- build optimized MIDP executable and example.
#    example	 -- just compile the example Java source files
#    midp  	 -- build the midp executable
#    midp-dist    -- binary distribution bundle
#    midp-src     -- exchange src bundles with RI partners
#    midp-docs    -- combined javadoc bundle for MIDP and CLDC 
#
#  There are also some release engineering targets. See
#  $(MIDP_DIR)/build/share/makefiles/Release.gmk and 
#  $(MIDP_DIR)/build/share/makefiles/Docs.gmk
#  for their descriptions.
#
#

BUILD_DIR=$(shell pwd)
# Setup platform directory structure
#
include $(BUILD_DIR)/Platform.gmk

# Include generic defines
#
include $(MIDP_DIR)/build/share/makefiles/Defs.gmk

all :: midp example

# Modify the 'clean' target to clean-up any platform-specific files
#
clean clobber ::
	@echo ... $@
	-@rm -fr .sb *.rec *.rst *.db *.pdb

reallyclean :: clean
	@echo ... $@

# Use the 'insanelyclean' target with caution! This will remove the ssl_obj
# directory which provides the native methods used by the HTTPS protocol.
#
insanelyclean :: reallyclean
	@echo ... $@

# core MIDP targets
#
include $(MIDP_DIR)/build/share/makefiles/MIDP.gmk

# MIDP examples
#
include $(MIDP_DIR)/build/share/makefiles/Example.gmk

everything : clean all docs_html docs_mid docs_mif clean_tools all_tools
	@echo ... $@

midp_quick: obj$(g)/nativeGUI.o
	@echo "Linking ... $(BINDIR)/$(MIDP_CMD)$(g)$(EXE)"
	@$(LD) $(DEBUG_FLAG) $(MIDP_OBJ_FILES) $(KVM_OBJ_FILES) \
	   $(PLATFORM_OBJ_FILES) $(LINKER_OUTPUT)$(BINDIR)/$(MIDP_CMD)$(g)$(EXE) \
	   $(LIBS)

# Documentation targets 
#
include $(MIDP_DIR)/build/share/makefiles/Docs.gmk

# Release engineering targets.
#
include $(MIDP_DIR)/build/share/makefiles/Release.gmk

# Quality Assurance engineering targets.
#
include $(MIDP_DIR)/build/share/makefiles/Testbeans.gmk


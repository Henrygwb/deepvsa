SUBDIRS=src
dist_doc_DATA=README

TESTSUITES_DIR = testsuites
EXECUTABLE=$(SUBDIRS)/reverse

testcases = abc2mtex \
            aireplay-ng \
            clamav_5314 \
            corehttp_3586 \
            corehttp_4060 \
            dxfscope \
            gas \
            gdb-6.6 \
            gdb-7.5.1 \
            gif2png \
            html2hdml \
            htmldoc \
            join \
            jpegtoavi \
            latex2rtf \
            libpng \
            libsmi \
            mcrypt \
            mp3info \
            nasm \
            nginx \
            nullhttp \
            o3read \
            openjpeg \
            pcal \
            podofo \
            poppler \
            prozilla \
            psutils \
            putty \
            python-33251 \
            python-38616 \
            sort \
            sqlite \
            stftp \
            tiff_2285 \
            unalz \
            uniq \
            unrar \
            0verkill \
            unrtf

.PHONY: all $(testcases)

$(testcases):%:
	$(EXECUTABLE) $(TESTSUITES_DIR)/$@/

# core
# libs/ 
# inst.reverse 
# binary.reverse 
# regs.reverse 
# xmm.log
# valset_heuristic
# region_DL

#	$(EXECUTABLE) $(TESTSUITES_DIR)/$@/core $(TESTSUITES_DIR)/$@/libs/ $(TESTSUITES_DIR)/$@/inst.reverse $(TESTSUITES_DIR)/$@/binary.reverse $(TESTSUITES_DIR)/$@/regs.reverse $(TESTSUITES_DIR)/$@/xmm.log $(TESTSUITES_DIR)/$@/valset_heuristic $(TESTSUITES_DIR)/$@/memop $(TESTSUITES_DIR)/$@/region_DL

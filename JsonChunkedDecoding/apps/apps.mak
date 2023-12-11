
APPWEBPATH = JsonChunkedDecoding/apps

STARTHTMLS += \
	$(APPWEBPATH)/inno-jsonchunkeddecoding.htm

APPWEBSRC_NOZIP += \
	$(APPWEBPATH)/inno-jsonchunkeddecoding.png

APPWEBSRC_ZIP += \
	$(APPWEBPATH)/inno-jsonchunkeddecoding.js \
	$(APPWEBPATH)/inno-jsonchunkeddecoding.css \
	$(APPWEBPATH)/inno-jsonchunkeddecodingtexts.js


APPWEBSRC = $(APPWEBSRC_ZIP) $(MANAGERSRC) $(APPWEBSRC_NOZIP)

$(OUTDIR)/obj/apps_start.cpp: $(STARTHTMLS) $(OUTDIR)/obj/innovaphone.manifest
		$(IP_SRC)/exe/httpfiles -k -d $(OUTDIR)/obj -t $(OUTDIR) -o $(OUTDIR)/obj/apps_start.cpp \
	-s 0 $(subst $(APPWEBPATH)/,,$(STARTHTMLS))

$(OUTDIR)/obj/apps.cpp: $(APPWEBSRC)
		$(IP_SRC)/exe/httpfiles -k -d $(APPWEBPATH) -t $(OUTDIR) -o $(OUTDIR)/obj/apps.cpp \
	-s 0,HTTP_GZIP $(subst $(APPWEBPATH)/,,$(APPWEBSRC_ZIP) $(MANAGERSRC)) \
	-s 0 $(subst $(APPWEBPATH)/,,$(APPWEBSRC_NOZIP))

APP_OBJS += $(OUTDIR)/obj/apps_start.o
$(OUTDIR)/obj/apps_start.o: $(OUTDIR)/obj/apps_start.cpp
APP_OBJS += $(OUTDIR)/obj/apps.o
$(OUTDIR)/obj/apps.o: $(OUTDIR)/obj/apps.cpp

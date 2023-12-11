include JsonChunkedDecoding/apps/apps.mak

APP_OBJS += $(OUTDIR)/obj/JsonChunkedDecoding.o
$(OUTDIR)/obj/JsonChunkedDecoding.o: JsonChunkedDecoding/JsonChunkedDecoding.cpp $(OUTDIR)/JsonChunkedDecoding/JsonChunkedDecoding.png

APP_OBJS += $(OUTDIR)/obj/JsonChunkedDecoding_tasks.o
$(OUTDIR)/obj/JsonChunkedDecoding_tasks.o: JsonChunkedDecoding/JsonChunkedDecoding_tasks.cpp

$(OUTDIR)/JsonChunkedDecoding/JsonChunkedDecoding.png: JsonChunkedDecoding/JsonChunkedDecoding.png
	copy JsonChunkedDecoding\JsonChunkedDecoding.png $(OUTDIR)\JsonChunkedDecoding\JsonChunkedDecoding.png

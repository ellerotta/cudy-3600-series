
LzmaAlone=./lzma457/CPP/7zip/Compress/LZMA_Alone

mklzma_cmd:
	$(MAKE) -C $(LzmaAlone) -f makefile.gcc && \
	cp -f $(LzmaAlone)/lzma lzmacmd


clean:
	rm -f lzmacmd
	$(MAKE) -C $(LzmaAlone) -f makefile.gcc clean


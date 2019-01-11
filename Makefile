

all:
	make -C common
	make -C server
	make -C client_c all

clean:
	make -C common clean
	make -C server clean
	make -C client_c clean
	make -C python_sdk clean

.PHONY: all clean
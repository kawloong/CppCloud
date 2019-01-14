
DIR=".."

all:
	make -C $(DIR)/common
	make -C $(DIR)/server
	make -C $(DIR)/client_c all

clean:
	make -C $(DIR)/common clean
	make -C $(DIR)/server clean
	make -C $(DIR)/client_c clean
	make -C $(DIR)/python_sdk clean

.PHONY: all clean
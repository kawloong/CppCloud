
DIR=".."

all:
	make -C $(DIR)/common
	make -C $(DIR)/server
	make -C $(DIR)/cpp_sdk all

clean:
	make -C $(DIR)/common clean
	make -C $(DIR)/server clean
	make -C $(DIR)/cpp_sdk clean
	make -C $(DIR)/python_sdk clean
	make -C $(DIR)/web_py clean

.PHONY: all clean

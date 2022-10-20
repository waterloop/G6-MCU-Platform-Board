THIS_DIR := $(shell readlink -f .)
GENERAL_BUILD_DIR = $(THIS_DIR)/build
BUILD_DIR = $(GENERAL_BUILD_DIR)/$(DEV)



all:
	cd $(DEV) && make BUILD_DIR=$(BUILD_DIR)

.PHONY: clean
clean:
	rm -rf $(GENERAL_BUILD_DIR)

.PHONY: analyze
analyze:
	$(PREFIX)objdump -t $(BUILD_DIR)/standalone/main.elf

.PHONY: flash
flash:
	st-flash write $(BUILD_DIR)/standalone/main.bin 0x08000000
	st-flash reset


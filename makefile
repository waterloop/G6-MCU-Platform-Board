THIS_DIR := $(shell readlink -f .)
BUILD_DIR = $(THIS_DIR)/build


all:
	cd $(DEV) && make BUILD_DIR=$(BUILD_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: analyze
analyze:
	$(PREFIX)objdump -t $(BUILD_DIR)/standalone/main.elf

.PHONY: flash
flash:
	st-flash write $(BUILD_DIR)/standalone/main.bin 0x08000000
	st-flash reset


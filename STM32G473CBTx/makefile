######################################
# target
######################################
TARGET = stm32g473

######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og


#######################################
# paths
#######################################
# Build path

OBJECTS_DIR = $(BUILD_DIR)/objects
PLATFORM_LIB_DIR = $(BUILD_DIR)/platform
STANDALONE_DIR = $(BUILD_DIR)/standalone

######################################
# source
######################################
# C sources

MAIN_SOURCE := ./Core/Src/main.cpp

PLATFORM_C_SOURCES := $(shell find ../platform/src -name "*.c")
PLATFORM_CPP_SOURCES := $(shell find ../platform/src -name "*.cpp")

CORE_C_SOURCES := $(shell find ./Core/Src -name "*.c")
CORE_CPP_SOURCES := $(shell find ./Core/Src -name "*.cpp" | grep -v "main.cpp")

HAL_SOURCES := $(shell find ./Drivers/STM32G4xx_HAL_Driver -name "*.c")

RTOS_SOURCES = \
./Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
./Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
./Middlewares/Third_Party/FreeRTOS/Source/list.c \
./Middlewares/Third_Party/FreeRTOS/Source/queue.c \
./Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c \
./Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
./Middlewares/Third_Party/FreeRTOS/Source/timers.c \
./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c \
./Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c

C_SOURCES = $(PLATFORM_C_SOURCES) $(CORE_C_SOURCES) $(HAL_SOURCES) $(RTOS_SOURCES)
CPP_SOURCES = $(MAIN_SOURCE) $(CORE_CPP_SOURCES) $(PLATFORM_CPP_SOURCES)

# ASM sources
ASM_SOURCES = ./Core/Startup/startup_stm32g473cbtx.s

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
AR = $(GCC_PATH)/$(PREFIX)ar
CPP_CC = $(GCC_PATH)/$(PREFIX)g++ -std=c++17
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
AR = $(PREFIX)ar
CPP_CC = $(PREFIX)g++ -std=c++17
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m4

# fpu
FPU = -mfpu=fpv4-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS =

# C defines
C_DEFS =  \
-D USE_HAL_DRIVER \
-D STM32G473xx \
-D DEBUG


# AS includes
AS_INCLUDES =

# C includes
C_INCLUDES =  \
-I ./Core/Inc \
-I ./Drivers/STM32G4xx_HAL_Driver/Inc \
-I ./Drivers/STM32G4xx_HAL_Driver/Inc/Legacy \
-I ./Middlewares/Third_Party/FreeRTOS/Source/include \
-I ./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 \
-I ./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
-I ./Drivers/CMSIS/Device/ST/STM32G4xx/Include \
-I ./Drivers/CMSIS/Include \
-I ../platform/inc \
-I ../G6-CAN-Messages

C_INCLUDES += $(USER_INCLUDES)

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = ./STM32G473CBTx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(STANDALONE_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(STANDALONE_DIR)/$(TARGET).elf $(STANDALONE_DIR)/$(TARGET).hex $(STANDALONE_DIR)/$(TARGET).bin $(PLATFORM_LIB_DIR)/platform.a


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(OBJECTS_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# list of CPP program objects
OBJECTS += $(addprefix $(OBJECTS_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))

# list of ASM program objects
OBJECTS += $(addprefix $(OBJECTS_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

PLATFORM_OBJECTS = $(addprefix $(OBJECTS_DIR)/,$(notdir $(PLATFORM_C_SOURCES:.c=.o)))
PLATFORM_OBJECTS = $(addprefix $(OBJECTS_DIR)/,$(notdir $(PLATFORM_CPP_SOURCES:.cpp=.o)))

$(OBJECTS_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(OBJECTS_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
	@echo ""

$(OBJECTS_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CPP_CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(OBJECTS_DIR)/$(notdir $(<:.cpp=.lst)) $< -o $@
	@echo ""

$(OBJECTS_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@
	@echo ""

$(PLATFORM_LIB_DIR)/platform.a: $(PLATFORM_OBJECTS)
	@echo ""
	$(AR) rvs $@ $(PLATFORM_OBJECTS)
	@echo ""
	$(SZ) $@
	@echo ""

$(STANDALONE_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CPP_CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo ""
	$(SZ) $@
	@echo ""

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)
	mkdir -p $(OBJECTS_DIR)
	mkdir -p $(PLATFORM_LIB_DIR)
	mkdir -p $(STANDALONE_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***


LDSCRIPT = linker.ld
LIBS = -lc -lm -lnosys
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard

ifeq ($(PRINTER),DREAMER_NX)
# DreamNX ( dreamerNX_1.9.20191123 )
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x2000000A -DPRINTER_DBUFFER=0x20000B20 -DPRINTER_CORE_CLOCK=0x20000684
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x08011728 -DM107_CALL_ADDRESS=0x08011746 -DFW_NAME=\"dreamerNX_1.9.20191123_dec.bin\"
else ifeq ($(PRINTER),DREAMER)
# Dreamer ( dreamer_v2.15.20200917 ) same as DreamerNX o_O
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x2000000A -DPRINTER_DBUFFER=0x20000B20 -DPRINTER_CORE_CLOCK=0x20000684
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x08011728 -DM107_CALL_ADDRESS=0x08011746 -DFW_NAME=\"dreamer_v2.15.20200917_dec.bin\"
else ifeq ($(PRINTER),CREATOR_MAX)
# CreatorMax ( creatorMax_1.0.1.20200817 )
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x20000002 -DPRINTER_DBUFFER=0x20000AE8 -DPRINTER_CORE_CLOCK=0x00000000
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x08011592 -DM107_CALL_ADDRESS=0x080115B0 -DFW_NAME=\"creatorMax_1.0.1.20200817_dec.bin\"
endif

MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
OPT = -Os -g -gdwarf-2
CFLAGS  = $(OPT) $(MCU) $(PATCH_SETTINGS) -std=c11 -Wall -pedantic -fdata-sections -ffunction-sections
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=patch.map -Wl,--gc-sections

patch_tool:
	gcc $(INJECTOR_SETTINGS) ff_injector.c -o ff_injector
 
dreamer_nx:
	$(MAKE) ff_patch patch_tool PRINTER=DREAMER_NX

dreamer:
	$(MAKE) ff_patch patch_tool PRINTER=DREAMER

creator_max:
	$(MAKE) ff_patch patch_tool PRINTER=CREATOR_MAX

%.o: %.c
	arm-none-eabi-gcc -c $(CFLAGS) -o $@ $<

%.o: %.s
	arm-none-eabi-gcc -x assembler-with-cpp -c $(CFLAGS) -o $@ $<

ff_patch: startup.o patch.o
	arm-none-eabi-gcc $(LDFLAGS) -o patch.elf startup.o patch.o
	arm-none-eabi-size patch.elf
	arm-none-eabi-objcopy -O binary -S patch.elf patch.bin

clean:
	rm -rf *.o *.elf *.map


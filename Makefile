LDSCRIPT = linker.ld
LIBS = -lc -lm -lnosys
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard

ifeq ($(PRINTER),DREAMER_NX)
# DreamNX ( dreamerNX_1.9.20191123 ) CORE_CLOCK=0x20000684
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x2000000A -DPRINTER_DBUFFER=0x20000B20 -DREAR_CASE_ENABLE=0x2000028D
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x08011728 -DM107_CALL_ADDRESS=0x08011746 -DFW_NAME=\"dreamerNX_1.9.20191123_dec\"
else ifeq ($(PRINTER),DREAMER)
# Dreamer ( dreamer_v2.15.20200917 ) same as DreamerNX o_O
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x2000000A -DPRINTER_DBUFFER=0x20000B20 -DREAR_CASE_ENABLE=0x2000028D
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x08011728 -DM107_CALL_ADDRESS=0x08011746 -DFW_NAME=\"dreamer_v2.15.20200917_dec\"
else ifeq ($(PRINTER),CREATOR_MAX)
# CreatorMax ( creatorMax_1.0.1.20200817 )
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x20000002 -DPRINTER_DBUFFER=0x20000AE8
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x08011592 -DM107_CALL_ADDRESS=0x080115B0 -DFW_NAME=\"creatorMax_1.0.1.20200817_dec\"
else ifeq ($(PRINTER),GUIDER)
#!UNTESTED: Guider ( guider_1.1.20170224 )
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x20000002 -DPRINTER_DBUFFER=0x20000E48
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x080116C4 -DM107_CALL_ADDRESS=0x080116D4 -DFW_NAME=\"guider_1.1.20170224_dec\"
else ifeq ($(PRINTER),FINDER)
#!UNTESTED: Finder ( finder_1.5.20170419 )
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x20000002 -DPRINTER_DBUFFER=0x20000DE0
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x080116C4 -DM107_CALL_ADDRESS=0x080116D4 -DFW_NAME=\"finder_1.5.20170419_dec\"
else ifeq ($(PRINTER),INVENTOR)
#!UNTESTED: Inventor ( inventor_1.9.20191123 )
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x20000014 -DPRINTER_DBUFFER=0x20000B00
	INJECTOR_SETTINGS=-DM106_CALL_ADDRESS=0x080115B8 -DM107_CALL_ADDRESS=0x080115D0 -DFW_NAME=\"inventor_1.9.20191123_dec\"
else ifeq ($(PRINTER),CREATOR_PRO2)
#!UNTESTED: Creator Pro 2 ( creator-pro2_v1.8.20210221 ) max temp patch only, no encryption/decryption needed
	PATCH_SETTINGS=-DPRINTER_DOFFSET=0x2000003C -DPRINTER_DBUFFER=0x20000B9C
	INJECTOR_SETTINGS=-DFW_NAME=\"creator-pro2_v1.8.20210221\"
endif

MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
OPT = -Os -g -gdwarf-2
CFLAGS  = $(OPT) $(MCU) $(PATCH_SETTINGS) -D$(PRINTER)=1 -std=c11 -Wall -pedantic -fdata-sections -ffunction-sections
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=patch.map -Wl,--gc-sections

all:
	@echo "use:\n\tmake <printer>"

patch_tool:
	gcc $(INJECTOR_SETTINGS) ff_injector.c -o ff_injector
 
dreamer_nx:
	$(MAKE) ff_patch patch_tool PRINTER=DREAMER_NX

dreamer:
	$(MAKE) ff_patch patch_tool PRINTER=DREAMER

creator_max:
	$(MAKE) ff_patch patch_tool PRINTER=CREATOR_MAX

guider:
	$(MAKE) ff_patch patch_tool PRINTER=GUIDER

finder:
	$(MAKE) ff_patch patch_tool PRINTER=FINDER

inventor:
	$(MAKE) ff_patch patch_tool PRINTER=INVENTOR

creator_pro2:
	$(MAKE) ff_patch patch_tool PRINTER=CREATOR_PRO2

%.o: %.c
	arm-none-eabi-gcc -c $(CFLAGS) -o $@ $<

%.o: %.s
	arm-none-eabi-gcc -x assembler-with-cpp -c $(CFLAGS) -o $@ $<

ff_patch: startup.o patch.o
	arm-none-eabi-gcc $(LDFLAGS) -o patch.elf startup.o patch.o
	arm-none-eabi-size patch.elf
	arm-none-eabi-objcopy -O binary -S patch.elf patch.bin

clean:
	rm -rf *.o *.elf *.map *_patched.bin

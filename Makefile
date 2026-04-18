CC      := riscv64-unknown-elf-gcc
OBJCOPY := riscv64-unknown-elf-objcopy
OBJDUMP := riscv64-unknown-elf-objdump

# T-Head C906 is RV64IMAC (no F/D), lp64 ABI
CFLAGS  := -march=rv64imac -mabi=lp64 -mcmodel=medany \
           -Os -g \
           -nostdlib -nostartfiles -ffreestanding \
           -Wall -Wextra \
           -Isrc

SRCS    := src/startup.c src/uart.c src/main.c
TARGET  := hello
BIN     := bin

FIPTOOL  := python3 /fiptool/fiptool
DDR_PARAM := /fiptool/data/ddr_param.bin
RTOS     := /fiptool/data/cvirtos.bin

.PHONY: all clean disasm

all: $(BIN)/fip.bin

$(BIN):
	mkdir -p $(BIN)

# ── ELF ──────────────────────────────────────────────────────────────────────
$(BIN)/$(TARGET).elf: $(SRCS) linker.ld | $(BIN)
	$(CC) $(CFLAGS) -T linker.ld -o $@ $(SRCS)

# ── flat binary ──────────────────────────────────────────────────────────────
$(BIN)/$(TARGET).bin: $(BIN)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@echo "Binary size: $$(wc -c < $@) bytes"

# ── dummy blobs for FIP slots we don't use (opensbi / uboot) ─────────────────
$(BIN)/dummy.bin: | $(BIN)
	dd if=/dev/zero of=$@ bs=512 count=1 2>/dev/null

# ── FIP image ─────────────────────────────────────────────────────────────────
$(BIN)/fip.bin: $(BIN)/$(TARGET).bin $(BIN)/dummy.bin
	$(FIPTOOL) \
		--fsbl $(BIN)/$(TARGET).bin \
		--ddr_param $(DDR_PARAM) \
		--opensbi $(BIN)/dummy.bin \
		--uboot $(BIN)/dummy.bin \
		--rtos $(RTOS) \
		$@
	@echo "fip.bin ready — copy to FAT32 partition of microSD"

# ── helpers ──────────────────────────────────────────────────────────────────
disasm: $(BIN)/$(TARGET).elf
	$(OBJDUMP) -d $<

clean:
	rm -rf $(BIN)

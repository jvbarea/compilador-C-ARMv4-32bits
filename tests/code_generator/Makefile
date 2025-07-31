.SUFFIXES:

PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
MYCC    := ../../mycc
QEMU    ?= qemu-system-arm
GDB     ?= gdb-multiarch
LDS     := linker.ld

CFLAGS  := -mcpu=arm7tdmi -marm -nostdlib -Og
LDFLAGS := -T $(LDS)
QEMUFLAGS = -M virt -nographic -semihosting

# -------- pipeline -----------------------------------------
%.s   : %.c $(MYCC)          ; $(MYCC) -S $< > $@
%.elf : %.s $(LDS)           ; $(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
%.run : %.elf                ; $(QEMU) $(QEMUFLAGS) -kernel $< ; echo "exit=$$?"
%.dbg : %.elf                ; $(QEMU) $(QEMUFLAGS) -S -gdb tcp::1234 -kernel $< & \
                               sleep 0.5 ; $(GDB) -q $< -ex "target remote :1234" \
                               -ex "monitor system_reset" -ex "tui enable"

# -------- metas de conveniência -----------------------------
FILES ?= $(basename $(filter %.c,$(MAKECMDGOALS)))

run: $(addsuffix .run,$(FILES))
dbg: $(addsuffix .dbg,$(FILES))

clean: ; rm -f *.s *.elf

# Evita que foo.c seja tratado como alvo a construir
$(FILES): ;

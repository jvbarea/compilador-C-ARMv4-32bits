###############################################################################
.SUFFIXES:                         # desativa regras implícitas do GNU make

# — Ferramentas --------------------------------------------------------------
PREFIX   ?= arm-none-eabi-
CC       := $(PREFIX)gcc           # GCC cross-compiler
MYCC     := ../../mycc             # seu compilador  → .s
QEMU     ?= qemu-system-arm
GDB      ?= gdb-multiarch
LDS      := linker.ld

CFLAGS    = -mcpu=arm7tdmi -marm -nostdlib -Og
LDFLAGS   = -T $(LDS)
QEMUFLAGS = -M virt -nographic -semihosting-config enable=on,target=native        # captura SWI 0x123456

###############################################################################
## 1) Fluxo “MYCC”  (.c → .s → .elf → run/gdb)
###############################################################################

# — 1.1)  .c  →  .s  (via MYCC) ------------------------------------------------
%.s : %.c $(MYCC)
	@echo "🛠  [asm] $< → $@"
	$(MYCC) -S $< > $@

# — 1.2)  .s  →  .elf ----------------------------------------------------------
%.elf : %.s $(LDS)
	@echo "🛠  [link] $< → $@"
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# — 1.3)  executa --------------------------------------------------------------
%.run : %.elf
	@echo "▶️   Executando $< …"
	@$(QEMU) $(QEMUFLAGS) -kernel $< ; echo "📤 exit=$$?"

# — 1.4)  depuração (QEMU + GDB) ----------------------------------------------
%.gdb : %.elf
	@echo "🛠  Debug: abrindo QEMU + GDB"
	@$(QEMU) $(QEMUFLAGS) -S -gdb tcp::1234 -kernel $< & \
	  qpid=$$! ; sleep 0.5 ; \
	  $(GDB) -q $< \
	    -ex "target remote :1234" \
	    -ex "load" \
	    -ex "monitor system_reset" \
	    -ex "tui enable" ; \
	  kill $$qpid >/dev/null 2>&1 || true

###############################################################################
## 2) Fluxo “GCC”  (.c → .gcc.s → .gcc.elf → run/gdb)
###############################################################################

# — 2.1)  .c  →  .gcc.s  -------------------------------------------------------
%.gcc.s : %.c
	@echo "🛠  [asm-gcc] $< → $@"
	$(CC) $(CFLAGS) -S $< -o $@

# — 2.2)  .gcc.s  →  .gcc.elf  -------------------------------------------------
%.gcc.elf : %.gcc.s $(LDS)
	@echo "🛠  [link-gcc] $< → $@"
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# — 2.3)  executa --------------------------------------------------------------
%.gcc.run : %.gcc.elf
	@echo "▶️   Executando (GCC) $< …"
	@$(QEMU) $(QEMUFLAGS) -kernel $< ; echo "📤 exit=$$?"

# — 2.4)  depuração (QEMU + GDB) ----------------------------------------------
%.gcc.gdb : %.gcc.elf
	@echo "🛠  Debug (GCC): abrindo QEMU + GDB"
	@$(QEMU) $(QEMUFLAGS) -S -gdb tcp::1234 -kernel $< & \
	  qpid=$$! ; sleep 0.5 ; \
	  $(GDB) -q $< \
	    -ex "target remote :1234" \
	    -ex "load" \
	    -ex "monitor system_reset" \
	    -ex "tui enable" ; \
	  kill $$qpid >/dev/null 2>&1 || true

###############################################################################
## 3) Metas de conveniência
###############################################################################
# Arquivos .c passados na linha de comando + extras via FILES=
FILES := $(basename $(filter %.c,$(MAKECMDGOALS))) $(FILES)

.PHONY: run gdb run-gcc gdb-gcc clean

run:     $(addsuffix .run,$(FILES))        # executa saída do MYCC
gdb:     $(addsuffix .gdb,$(FILES))        # depura saída do MYCC
run-gcc: $(addsuffix .gcc.run,$(FILES))    # executa saída do GCC
gdb-gcc: $(addsuffix .gcc.gdb,$(FILES))    # depura saída do GCC

# Evita que foo.c vire “target” a construir
$(FILES:%=%.c):

clean:
	rm -f *.s *.elf *.gcc.s *.gcc.elf
###############################################################################

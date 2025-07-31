###############################################################################
.SUFFIXES:                         # desativa regras implícitas do GNU make

# — Ferramentas --------------------------------------------------------------
PREFIX   ?= arm-none-eabi-
CC       := $(PREFIX)gcc
MYCC     := ../../mycc              # seu compilador -> .s
QEMU     ?= qemu-system-arm
GDB      ?= gdb-multiarch
LDS      := linker.ld

CFLAGS    = -mcpu=arm7tdmi -marm -nostdlib -Og
LDFLAGS   = -T $(LDS)
QEMUFLAGS = -M virt -nographic -semihosting        # captura SWI 0x123456

# — 1)  .c  →  .s ------------------------------------------------------------
%.s : %.c $(MYCC)
	@echo "🛠  [asm] $< → $@"
	$(MYCC) -S $< > $@

# — 2)  .s  →  .elf ----------------------------------------------------------
%.elf : %.s $(LDS)
	@echo "🛠  [link] $< → $@"
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# — 3)  executa --------------------------------------------------------------
%.run : %.elf
	@echo "▶️   Executando $< …"
	@$(QEMU) $(QEMUFLAGS) -kernel $< ; echo "📤 exit=$$?"

# — 4)  depuração (QEMU + GDB) ----------------------------------------------
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

# —–––– metas de conveniência —–––––––––––––––––––––––––––––––––––––––––––––––
# Arquivos .c passados na linha de comando, + os vindos via FILES=
FILES := $(basename $(filter %.c,$(MAKECMDGOALS))) $(FILES)

.PHONY: run gdb clean

run: $(addsuffix .run,$(FILES))
gdb: $(addsuffix .gdb,$(FILES))

# Evita que foo.c vire “target” a construir
$(FILES:%=%.c):

clean:
	rm -f *.s *.elf
###############################################################################

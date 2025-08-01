/*---------------------------------------------------------------------------*/
/*  linker.ld — ARM32 bare-metal (QEMU virt)                                 */
/*---------------------------------------------------------------------------*/
ENTRY(_start)                 /* e_entry do ELF                    */

MEMORY
{
    /* RAM física da placa virt inicia em 0x4000_0000                */
    RAM (rwx) : ORIGIN = 0x40000000, LENGTH = 128M
}

/* Deixa 64 KiB livres: o QEMU grava o DTB logo no início da RAM     */
SECTIONS
{
    . = ORIGIN(RAM) + 0x10000;          /* ponto real de carga/execução */

    /*--------------------------------------------------------------------*/
    /*  Código e literais                                                 */
    /*--------------------------------------------------------------------*/
    _text_start = .;
    .text : ALIGN(4)
    {
        *(.text*)                      /* funções, .init, .fini …        */
    } > RAM
    _text_end = .;

    /*--------------------------------------------------------------------*/
    /*  Dados de somente-leitura                                          */
    /*--------------------------------------------------------------------*/
    . = ALIGN(8);
    _rodata_start = .;
    .rodata : { *(.rodata*) } > RAM
    _rodata_end = .;

    /*--------------------------------------------------------------------*/
    /*  Dados inicializados                                               */
    /*--------------------------------------------------------------------*/
    . = ALIGN(8);
    _data_start = .;
    .data : { *(.data*) } > RAM
    _data_end = .;

    /*--------------------------------------------------------------------*/
    /*  BSS (zerada no _start)                                            */
    /*--------------------------------------------------------------------*/
    . = ALIGN(8);
    _bss_start = .;
    .bss (NOLOAD) : { *(.bss*) *(COMMON) } > RAM
    _bss_end = .;

    /*--------------------------------------------------------------------*/
    /*  Heap (começa logo após a BSS)                                     */
    /*--------------------------------------------------------------------*/
    . = ALIGN(8);
    _end = .;                /* também conhecido como heap_start */

    /*--------------------------------------------------------------------*/
    /*  Pilha — full-descending, alinhada a 8 bytes (AAPCS)               */
    /*--------------------------------------------------------------------*/
    . = ALIGN(8);
    _stack_bottom = .;
    . = . + 8K;            /* 8 KiB para a pilha; ajuste à vontade */
    _stack_top = .;        /* carregar em SP no _start            */

    /* Fim das seções — o resto da RAM fica livre para heap ou buffers */
}

/*---------------------------------------------------------------------------*/

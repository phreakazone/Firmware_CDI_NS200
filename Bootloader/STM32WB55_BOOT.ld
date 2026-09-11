ENTRY(Reset_Handler)
_estack = 0x20030000;
_Min_Heap_Size = 0x100;
_Min_Stack_Size = 0x400;
MEMORY
{
 FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
 RAM (xrw)   : ORIGIN = 0x20000008, LENGTH = 0x2FFF8
}
SECTIONS {
 .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4); } >FLASH
 .text : { . = ALIGN(4); *(.text*) *(.rodata*) KEEP(*(.init)) KEEP(*(.fini)) . = ALIGN(4); _etext = .; } >FLASH
 .ARM.extab : { *(.ARM.extab*) } >FLASH
 .ARM : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } >FLASH
 _sidata = LOADADDR(.data);
 .data : { . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } >RAM AT> FLASH
 .bss : { . = ALIGN(4); _sbss = .; __bss_start__ = _sbss; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; __bss_end__ = _ebss; } >RAM
 .mb_dummy (NOLOAD) : { _sMB_MEM2 = .; _eMB_MEM2 = .; } >RAM
 ._user_heap_stack : { . = ALIGN(8); PROVIDE(end = .); PROVIDE(_end = .); . += _Min_Heap_Size; . += _Min_Stack_Size; . = ALIGN(8); } >RAM
 /DISCARD/ : { *(.note.GNU-stack) }
}

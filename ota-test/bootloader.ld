/*
 * Linker script for Bootloader
 * Location: 0x10000000 (start of flash)
 * Size: 256KB
 */

MEMORY
{
    /* Bootloader occupies first 256KB of flash */
    FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 256K
    
    /* RAM - shared by all partitions */
    RAM(rwx)  : ORIGIN = 0x20000000, LENGTH = 264K
    
    /* Scratch RAM for core 1 if needed */
    SCRATCH_X(rwx) : ORIGIN = 0x20040000, LENGTH = 4K
    SCRATCH_Y(rwx) : ORIGIN = 0x20041000, LENGTH = 4K
}

ENTRY(_entry_point)

SECTIONS
{
    /* Boot2 stage must be first - 256 bytes */
    .boot2 : {
        __boot2_start__ = .;
        KEEP(*(.boot2))
        __boot2_end__ = .;
    } > FLASH
    
    /* Code section */
    .text : {
        __logical_binary_start = .;
        KEEP(*(.vectors))
        KEEP(*(.binary_info_header))
        __binary_info_header_end = .;
        KEEP(*(.reset))
        
        /* Application code */
        *(.time_critical*)
        *(.text*)
        
        /* Read-only data */
        . = ALIGN(4);
        *(.rodata*)
        
        /* Constructor/destructor tables */
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
        
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
        
        . = ALIGN(4);
        __fini_array_start = .;
        *(SORT_BY_INIT_PRIORITY(.fini_array.*))
        *(.fini_array)
        __fini_array_end = .;
        
        *(.eh_frame)
    } > FLASH
    
    /* ARM exception unwinding */
    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH
    
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH
    
    /* End of flash code */
    . = ALIGN(256);
    __flash_binary_end = .;
    
    /* Initialized data - copied from flash to RAM at startup */
    .data : {
        __data_start__ = .;
        *(vtable)
        *(.data*)
        . = ALIGN(4);
        *(.after_data.*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT> FLASH
    
    /* Used by startup code to initialize .data */
    __etext = LOADADDR(.data);
    
    /* Uninitialized data (zeroed at startup) */
    .bss : {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM
    
    /* Heap */
    .heap (NOLOAD): {
        __end__ = .;
        end = __end__;
        __HeapBase = .;
        *(.heap*)
        . = ORIGIN(RAM) + LENGTH(RAM) - 0x800; /* Reserve 2KB for stack */
        __HeapLimit = .;
    } > RAM
    
    /* Stack (grows downward) */
    .stack (NOLOAD): {
        __StackLimit = .;
        *(.stack*)
        . = . + 0x800; /* 2KB stack */
        __StackTop = .;
    } > RAM
    PROVIDE(__stack = __StackTop);
    
    /* Scratch X and Y for core 1 */
    .scratch_x (NOLOAD): {
        __scratch_x_start__ = .;
        *(.scratch_x*)
        . = ALIGN(4);
        __scratch_x_end__ = .;
    } > SCRATCH_X
    
    .scratch_y (NOLOAD): {
        __scratch_y_start__ = .;
        *(.scratch_y*)
        . = ALIGN(4);
        __scratch_y_end__ = .;
    } > SCRATCH_Y
    
    /* Discard debug sections */
    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu.lto_*)
    }
}

/* Assert checks */
ASSERT(__flash_binary_end <= ORIGIN(FLASH) + LENGTH(FLASH), 
       "Bootloader exceeds 256KB flash allocation")
ASSERT(__StackTop <= ORIGIN(RAM) + LENGTH(RAM), 
       "Stack overflow into RAM boundary")
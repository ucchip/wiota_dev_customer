/* not needed, but we need separate linker scripts anyway */
OUTPUT_ARCH(riscv)
/* required to correctly link newlib */
GROUP( -lc -lgloss -lgcc -lsupc++ )

SEARCH_DIR(.)
__DYNAMIC  =  0;

MEMORY
{
	uboot       : ORIGIN = 0x00000000, LENGTH = 0X7000  /* 28K uboot length*/
	uc_rom      : ORIGIN = 0x00007000, LENGTH = 0x3000  /* 12K uncompressed code sys data, NO RODATA!!!!*/
	protocal_stack   : ORIGIN = 0x0000A000, LENGTH = 0x14000  /*80K*/
	instrram    : ORIGIN = 0x0001E000, LENGTH = 0x12000 /* 72K actually rom... */
	datarom     : ORIGIN = 0x00200000, LENGTH = 0x4FC00 /* 84K data rom*/
	dataram     : ORIGIN = 0x00308000, LENGTH = 0x7B00  /* 30K byte board.h also need change!!! */
	traceram    : ORIGIN = 0x0030FB00, LENGTH = 0x400   /* 1 KB */
	stack       : ORIGIN = 0x0030FF00, LENGTH = 0x100   /* 256 byte */
	ota_flag    : ORIGIN = 0x00400000, LENGTH = 1K
}



/* Stack information variables */
_min_stack     = 0x100;   /* 256 - minimum stack space to reserve */
_stack_len     = LENGTH(stack);
_stack_start   = ORIGIN(stack) + LENGTH(stack);
_rom_end       = ORIGIN(uc_rom) + LENGTH(uc_rom);
_rom_start     = ORIGIN(uc_rom);

/* We have to align each sector to word boundaries as our current s19->slm
 * conversion scripts are not able to handle non-word aligned sections. */

SECTIONS
{

	.uboot :
	{
		. += LENGTH(uboot) -4;
		LONG(0x00000000);
	} > uboot = 0x00000000

    .vectors :
    {
         KEEP(*(.vectors));
    } > uc_rom

    .boot : ALIGN(512)
    {
        *(.critical)
    } > uc_rom

    .fastfunc :
    {
        . = ALIGN(4);
        *(.reset)
        *(.crt0);
        *(.download);
        . += 80;
        . = ALIGN(4096);
        _stbl_end = .;
    } > uc_rom

/*functions called only once during startup */
    .init :
    {
        . = ALIGN(4);
        *(.init)
    } > instrram

	.protocal_stack :
	{
		*l1_*(.text*);
		*c_cce_firmware*(.text*);
		*cce*(.text*);
		*mac*(.text*);
		*adp*(.text*);
		*ctrl_cmd*(.text*);
		*trcKernelPort*(.text*);
		*trcStreamingPort*(.text*);
		*trcStreamingRecorder*(.text*);
		*vsi_trc*(.text*);
		*dll*(.text*);
		*uart_tool*(.text*);
		*global_task*(.text*);
		. = LENGTH(protocal_stack ) -4;
		LONG(0);
	} > protocal_stack

    .text :
    {
        . = ALIGN(4);
        _stext = .;
		*(.text*)
        *(.pvt)
        _etext  =  .;
        __CTOR_LIST__ = .;
        LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
        *(.ctors)
        LONG(0)
        __CTOR_END__ = .;
        __DTOR_LIST__ = .;
        LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
        *(.dtors)
        LONG(0)
        __DTOR_END__ = .;
        *(.lit)
        *(.shdata)
       _endtext = .;
	   . = LENGTH(instrram) -4;
	   LONG(0);

    }  > instrram

    /*--------------------------------------------------------------------*/
    /* Global constructor/destructor segement                             */
    /*--------------------------------------------------------------------*/

    .preinit_array     :
    {
      PROVIDE_HIDDEN (__preinit_array_start = .);
      KEEP (*(.preinit_array))
      PROVIDE_HIDDEN (__preinit_array_end = .);
    } > datarom

    .fini_array     :
    {
      PROVIDE_HIDDEN (__fini_array_start = .);
   /*   KEEP (*(SORT(.fini_array.*)))*/
   /*   KEEP (*(.fini_array ))*/
      PROVIDE_HIDDEN (__fini_array_end = .);
    } > datarom

    .rodata : {
        . = ALIGN(4);
    KEEP (*(.dsp_data));
        *(.rodata);
        *(.rodata.*);
        *(.srodata.*);
	*(.srodata*);

	*(FalPartTable);

	/* section information for finsh shell */
	. = ALIGN(4);
	__fsymtab_start = .;
	KEEP(*(SORT(FSymTab)))
	__fsymtab_end = .;
	. = ALIGN(4);
	__vsymtab_start = .;
	KEEP(*(SORT(VSymTab)))
	__vsymtab_end = .;
	. = ALIGN(4);

	. = ALIGN(4);
	__rt_init_start = .;
	KEEP(*(SORT(.rti_fn*)))
	__rt_init_end = .;
	. = ALIGN(4);

	/* section information for modules */
	. = ALIGN(4);
	__rtmsymtab_start = .;
	KEEP(*(SORT(RTMSymTab)))
	__rtmsymtab_end = .;


    } > datarom

    .at_array     :
    {
        /* section information for at server */
        . = ALIGN(4);
        __rtatcmdtab_start = .;
        KEEP(*(RtAtCmdTab))
        __rtatcmdtab_end = .;
        . = ALIGN(4);
    } > datarom


    .shbss :
    {
        . = ALIGN(4);
        *(.shbss)
    } > dataram

    .data : {
        . = ALIGN(4);
        sdata  =  .;
        _sdata  =  .;
        *(.data);
        *(.data.*);
        *(.sdata);
        *(.sdata*);
        edata  =  .;
        _edata  =  .;
    } > dataram

    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
        _bss_end = .;
    } > dataram

    .heap : {
       . = ALIGN(4);
    } > dataram

    .guard : {
       . = ALIGN(4);
    . = . + 16;
    } > dataram



    /* ensure there is enough room for stack */
    .stack (NOLOAD): {
        . = ALIGN(4);
        . = . + _min_stack ;
        . = ALIGN(4);
        stack = . ;
        _stack = . ;
        _sp = . ;
    } > stack

    .stab  0 (NOLOAD) :
    {
        [ .stab ]
    }

    .stabstr  0 (NOLOAD) :
    {
        [ .stabstr ]
    }

    .bss :
    {
        . = ALIGN(4);
        _end = .;
    } > dataram


    .trace_buff :
    {
        . = ALIGN(4);
        *(.trace_buff)
    } > traceram

	.ota_flag :
	{
		LONG(0x46054355);
		LONG(LENGTH(uboot));
		LONG(LENGTH(uc_rom));
		LONG(LENGTH(protocal_stack));
		LONG(LENGTH(instrram));
		LONG(LENGTH(datarom));
	} > ota_flag

}

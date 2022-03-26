#ifndef __SECTION_DEFINES_H__
#define __SECTION_DEFINES_H__

/*subsections can be put into large sections in link scripts */
#ifndef _FPGA_

#define __crt0
#define __dsp
#define __task
#define __reset
#define __reset0
#define __reset1
#define __reset2
#define __reset3
#define BOOT_RESET(name, index)
#define CRT0_SECTION(name, index)
//#define AUTO_SECTION(time, name, index)
#define __rom_func

#else

#define __crt0 __attribute__ ((section (".crt0")))
#define __dsp  __attribute__ ((section (".dsp_data")))
#define __task  __attribute__ ((section (".task_text")))
#define __reset __attribute__ ((section (".reset")))
#define __reset_64 __attribute__ ((section (".reset"), aligned(64)))
#define __reset0 __attribute__ ((section (".reset0")))
#define __reset1 __attribute__ ((section (".reset1")))
#define __reset2 __attribute__ ((section (".reset2")))
#define __reset3 __attribute__ ((section (".reset3")))
#define BOOT_RESET(index)  __attribute__ ((section (".boot_"#index)))
#define CRT0_SECTION(name, index)  __attribute__ ((section (".crt0_" #name "_" #index)))
#define DOWNLOADSECTION __attribute__((section(".download")))
#define UBOOTSHAREDATASECTION __attribute__((section(".share_data")))

//#define AUTO_SECTION(time, name, index)  __attribute__ ((section (".auto_section_" #time "_" #name "_" #index)))
//#define AUTO_SECTION(name, index)  __attribute__ ((section (".auto_section_"#name "_" #index)))
//#define AUTO_DATA_SECTION(name, index)  __attribute__ ((section (".data_section_"#name "_" #index)))
//#define __rom_func __attribute__ ((section (".fixfunc")))
#define __rom_func

//#define _ROMFUNC_

#endif

#define __init __attribute__ ((section (".init")))
#endif

#ifndef __SECTION_DEFINES_H__
#define __SECTION_DEFINES_H__

/*subsections can be put into large sections in link scripts */

#define __crt0 __attribute__ ((section (".crt0")))
#define __reset __attribute__ ((section (".reset")))
#define __init __attribute__ ((section (".init")))
#define __dsp  __attribute__ ((section (".dsp_data")))


#endif

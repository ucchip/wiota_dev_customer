#include "mem_share.h"

//int _share[1024 / sizeof(int)] __attribute__ ((section (".share")));
//int _share[2];
int get_program_type(void)
{
    //return _share[0];
    return 0;
}

void set_program_type(int type)
{
    //_share[0] = type;
}

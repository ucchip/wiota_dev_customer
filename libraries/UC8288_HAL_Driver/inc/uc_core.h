#ifndef _CORE_H
#define _CORE_H

//#ifdef __cplusplus
//    #define __I    volatile           /* Defines "read only" permissions */
//#else
//    #define __I    volatile const     /* Defines "read only" permissions */
//#endif

#define     __O    volatile           /* Defines "write only" permissions */

#define     __IO   volatile           /* Defines "write/read" permissions */

#define     __I    volatile const     /* Defines "read only" permissions */

#ifdef USE_FULL_ASSERT
#define    CHECK_PARAM(expr)   ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define    CHECK_PARAM(expr)   ((void)0)
#endif






#endif
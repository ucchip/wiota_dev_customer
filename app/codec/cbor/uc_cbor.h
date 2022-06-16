/**
 * file
 * brief
 * CBOR parsing
 */

#ifndef UC_CBOR_H
#define UC_CBOR_H

#include <rtthread.h>
#include "string.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define htons(ret) (((ret & 0xFF) << 8) | ((ret >> 8) & 0xFF))
#define htonl(ret) (((ret & 0xFF) << 24) | (((ret >> 8) & 0xFF) << 16) | (((ret >> 16) & 0xFF) << 8) | (((ret >> 24) & 0xFF)))

#define CBOR_NO_FLOAT

#define CBOR_ASSERT(arg)                               \
    if ((arg) == 0)                                    \
    {                                                  \
        rt_kprintf("have a assert failed: %s %s %d\n", \
                   __FILE__, __FUNCTION__, __LINE__);  \
        while (1)                                      \
        {                                              \
        }                                              \
    }

/* The 8 major types */
#define MT_UNSIGNED 0
#define MT_NEGATIVE 1
#define MT_BYTES 2
#define MT_TEXT 3
#define MT_ARRAY 4
#define MT_MAP 5
#define MT_TAG 6
#define MT_PRIM 7

/* The initial bytes resulting from those */
#define IB_UNSIGNED (MT_UNSIGNED << 5)
#define IB_NEGATIVE (MT_NEGATIVE << 5)
#define IB_BYTES (MT_BYTES << 5)
#define IB_TEXT (MT_TEXT << 5)
#define IB_ARRAY (MT_ARRAY << 5)
#define IB_MAP (MT_MAP << 5)
#define IB_TAG (MT_TAG << 5)
#define IB_PRIM (MT_PRIM << 5)

#define IB_NEGFLAG (IB_NEGATIVE - IB_UNSIGNED)
#define IB_NEGFLAG_AS_BIT(ib) ((ib) >> 5)
#define IB_TEXTFLAG (IB_TEXT - IB_BYTES)

#define IB_AI(ib) ((ib)&0x1F)
#define IB_MT(ib) ((ib) >> 5)

/* Tag numbers handled by this implementation */
#define TAG_TIME_EPOCH 1
#define TAG_BIGNUM 2
#define TAG_BIGNUM_NEG 3
#define TAG_URI 32
#define TAG_RE 35

/* Initial bytes of those tag numbers */
#define IB_TIME_EPOCH (IB_TAG | TAG_TIME_EPOCH)
#define IB_BIGNUM (IB_TAG | TAG_BIGNUM)
#define IB_BIGNUM_NEG (IB_TAG | TAG_BIGNUM_NEG)
/* TAG_URI and TAG_RE are non-immediate tags */

/* Simple values handled by this implementation */
#define VAL_FALSE 20
#define VAL_TRUE 21
#define VAL_NIL 22
#define VAL_UNDEF 23

/* Initial bytes of those simple values */
#define IB_FALSE (IB_PRIM | VAL_FALSE)
#define IB_TRUE (IB_PRIM | VAL_TRUE)
#define IB_NIL (IB_PRIM | VAL_NIL)
#define IB_UNDEF (IB_PRIM | VAL_UNDEF)

/* AI values with more data in head */
#define AI_1 24
#define AI_2 25
#define AI_4 26
#define AI_8 27
#define AI_INDEF 31
#define IB_BREAK (IB_PRIM | AI_INDEF)
/* For  */
#define IB_UNUSED (IB_TAG | AI_INDEF)

/* Floating point initial bytes */
#define IB_FLOAT2 (IB_PRIM | AI_2)
#define IB_FLOAT4 (IB_PRIM | AI_4)
#define IB_FLOAT8 (IB_PRIM | AI_8)

// These definitions are here because they aren't required for the public
// interface, and they were quite confusing in uc_cbor.h

#ifdef USE_CBOR_CONTEXT
/**
 * Allocate enough space for 1 `cn_cbor` structure.
 *
 * @param[in]  ctx  The allocation context, or NULL for calloc.
 * @return          A pointer to a `cn_cbor` or NULL on failure
 */
#define CN_CALLOC(ctx) ((ctx) && (ctx)->calloc_func) ? (ctx)->calloc_func(1, sizeof(cn_cbor), (ctx)->context) : calloc(1, sizeof(cn_cbor));

/**
 * Free a
 * @param  free_func [description]
 * @return           [description]
 */
#define CN_FREE(ptr, ctx) ((ctx) && (ctx)->free_func) ? (ctx)->free_func((ptr), (ctx)->context) : free((ptr));

#define CBOR_CONTEXT_PARAM , context
#define CN_CALLOC_CONTEXT() CN_CALLOC(context)
#define CN_CBOR_FREE_CONTEXT(p) CN_FREE(p, context)

#else

#define CBOR_CONTEXT_PARAM
#define CN_CALLOC_CONTEXT() CN_CALLOC
#define CN_CBOR_FREE_CONTEXT(p) CN_FREE(p)

#ifndef CN_CALLOC
//#define CN_CALLOC calloc(1, sizeof(cn_cbor))
#define CN_CALLOC rt_calloc(1, sizeof(cn_cbor))
#endif

#ifndef CN_FREE
//#define CN_FREE free
#define CN_FREE rt_free
#endif

#endif // USE_CBOR_CONTEXT

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(p) ((void)&(p))
#endif

/**
 * All of the different kinds of CBOR values.
 */
typedef enum cn_cbor_type
{
    /** false */
    CN_CBOR_FALSE,
    /** true */
    CN_CBOR_TRUE,
    /** null */
    CN_CBOR_NULL,
    /** undefined */
    CN_CBOR_UNDEF,
    /** Positive integers */
    CN_CBOR_UINT,
    /** Negative integers */
    CN_CBOR_INT,
    /** Byte string */
    CN_CBOR_BYTES,
    /** UTF-8 string */
    CN_CBOR_TEXT,
    /** Byte string, in chunks.  Each chunk is a child. */
    CN_CBOR_BYTES_CHUNKED,
    /** UTF-8 string, in chunks.  Each chunk is a child */
    CN_CBOR_TEXT_CHUNKED,
    /** Array of CBOR values.  Each array element is a child, in order */
    CN_CBOR_ARRAY,
    /** Map of key/value pairs.  Each key and value is a child, alternating. */
    CN_CBOR_MAP,
    /** Tag describing the next value.  The next value is the single child. */
    CN_CBOR_TAG,
    /** Simple value, other than the defined ones */
    CN_CBOR_SIMPLE,
    /** Doubles, floats, and half-floats */
    CN_CBOR_DOUBLE,
    /** Floats, and half-floats */
    CN_CBOR_FLOAT,
    /** An error has occurred */
    CN_CBOR_INVALID
} cn_cbor_type;

/**
 * Flags used during parsing.  Not useful for consumers of the
 * `cn_cbor` structure.
 */
typedef enum cn_cbor_flags
{
    /** The count field will be used for parsing */
    CN_CBOR_FL_COUNT = 1,
    /** An indefinite number of children */
    CN_CBOR_FL_INDEF = 2,
    /** Not used yet; the structure must free the v.str pointer when the
       structure is freed */
    CN_CBOR_FL_OWNER = 0x80, /* of str */
} cn_cbor_flags;

typedef struct codec_data_list_t
{
    unsigned int length;
    unsigned char* data_ptr;
//    struct codec_data_list_t* next;  /** The sibling after this one, or NULL if this is the last */
} codec_data_list_t;

/**
 * A CBOR value
 */
typedef struct cn_cbor
{
    /** The type of value */
    cn_cbor_type type;
    /** Flags used at parse time */
    cn_cbor_flags flags;
    /** Data associated with the value; different branches of the union are
        used depending on the `type` field. */
    union
    {
        /** CN_CBOR_BYTES */
        const uint8_t *bytes;
        /** CN_CBOR_TEXT */
        const char *str;
        /** CN_CBOR_INT */
        long sint;
        /** CN_CBOR_UINT */
        unsigned long uint;
        //#ifndef CBOR_NO_FLOAT
        /** CN_CBOR_DOUBLE */
        double dbl;
        /** CN_CBOR_FLOAT */
        float f;
        //#endif
        /** for use during parsing */
        unsigned long count;
    } v; /* TBD: optimize immediate */
    /** Number of children.
      * @note: for maps, this is 2x the number of entries */
    int length;
    /** The first child value */
    struct cn_cbor *first_child;
    /** The last child value */
    struct cn_cbor *last_child;
    /** The sibling after this one, or NULL if this is the last */
    struct cn_cbor *next;
    /** The parent of this value, or NULL if this is the root */
    struct cn_cbor *parent;
} cn_cbor;

/**
 * All of the different kinds of errors
 */
typedef enum cn_cbor_error
{
    /** No error has occurred */
    CN_CBOR_NO_ERROR,
    /** More data was expected while parsing */
    CN_CBOR_ERR_OUT_OF_DATA,
    /** Some extra data was left over at the end of parsing */
    CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED,
    /** A map should be alternating keys and values.  A break was found
        when a value was expected */
    CN_CBOR_ERR_ODD_SIZE_INDEF_MAP,
    /** A break was found where it wasn't expected */
    CN_CBOR_ERR_BREAK_OUTSIDE_INDEF,
    /** Indefinite encoding works for bstrs, strings, arrays, and maps.
        A different major type tried to use it. */
    CN_CBOR_ERR_MT_UNDEF_FOR_INDEF,
    /** Additional Information values 28-30 are reserved */
    CN_CBOR_ERR_RESERVED_AI,
    /** A chunked encoding was used for a string or bstr, and one of the elements
        wasn't the expected (string/bstr) type */
    CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING,
    /** An invalid parameter was passed to a function */
    CN_CBOR_ERR_INVALID_PARAMETER,
    /** Allocation failed */
    CN_CBOR_ERR_OUT_OF_MEMORY,
    /** A float was encountered during parse but the library was built without
        support for float types. */
    CN_CBOR_ERR_FLOAT_NOT_SUPPORTED
} cn_cbor_error;

/**
 * Strings matching the `cn_cbor_error` conditions.
 *
 * @todo: turn into a function to make the type safety more clear?
 */
extern const char *cn_cbor_error_str[];

/**
 * Errors
 */
typedef struct cn_cbor_errback
{
    /** The position in the input where the erorr happened */
    int pos;
    /** The error, or CN_CBOR_NO_ERROR if none */
    cn_cbor_error err;
} cn_cbor_errback;

#ifdef USE_CBOR_CONTEXT

/**
 * Allocate and zero out memory.  `count` elements of `size` are required,
 * as for `calloc(3)`.  The `context` is the `cn_cbor_context` passed in
 * earlier to the CBOR routine.
 *
 * @param[in] count   The number of items to allocate
 * @param[in] size    The size of each item
 * @param[in] context The allocation context
 */
typedef void *(*cn_calloc_func)(size_t count, size_t size, void *context);

/**
 * Free memory previously allocated with a context.  If using a pool allocator,
 * this function will often be a no-op, but it must be supplied in order to
 * prevent the CBOR library from calling `free(3)`.
 *
 * @note: it may be that this is never needed; if so, it will be removed for
 * clarity and speed.
 *
 * @param  context [description]
 * @return         [description]
 */
typedef void (*cn_free_func)(void *ptr, void *context);

/**
 * The allocation context.
 */
typedef struct cn_cbor_context
{
    /** The pool `calloc` routine.  Must allocate and zero. */
    cn_calloc_func calloc_func;
    /** The pool `free` routine.  Often a no-op, but required. */
    cn_free_func free_func;
    /** Typically, the pool object, to be used when calling `calloc_func`
      * and `free_func` */
    void *context;
} cn_cbor_context;

/** When USE_CBOR_CONTEXT is defined, many functions take an extra `context`
  * parameter */
#define CBOR_CONTEXT , cn_cbor_context *context
/** When USE_CBOR_CONTEXT is defined, some functions take an extra `context`
  * parameter at the beginning */
#define CBOR_CONTEXT_COMMA cn_cbor_context *context,

#else

#define CBOR_CONTEXT
#define CBOR_CONTEXT_COMMA

#endif

/**
 * Decode an array of CBOR bytes into structures.
 *
 * @param[in]  buf          The array of bytes to parse
 * @param[in]  len          The number of bytes in the array
 * @param[in]  CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out] errp         Error, if NULL is returned
 * @return                  The parsed CBOR structure, or NULL on error
 */
cn_cbor *cn_cbor_decode(const uint8_t *buf, size_t len CBOR_CONTEXT, cn_cbor_errback *errp);

/**
 * Get a value from a CBOR map that has the given string as a key.
 *
 * @param[in]  cb           The CBOR map
 * @param[in]  key          The string to look up in the map
 * @return                  The matching value, or NULL if the key is not found
 */
cn_cbor *cn_cbor_mapget_string(const cn_cbor *cb, const char *key);

/**
 * Get a value from a CBOR map that has the given integer as a key.
 *
 * @param[in]  cb           The CBOR map
 * @param[in]  key          The int to look up in the map
 * @return                  The matching value, or NULL if the key is not found
 */
cn_cbor *cn_cbor_mapget_int(const cn_cbor *cb, int key);

/**
 * Get the item with the given index from a CBOR array.
 *
 * @param[in]  cb           The CBOR map
 * @param[in]  idx          The array index
 * @return                  The matching value, or NULL if the index is invalid
 */
cn_cbor *cn_cbor_index(const cn_cbor *cb, unsigned int idx);

/**
 * Free the given CBOR structure.
 * You MUST NOT try to free a cn_cbor structure with a parent (i.e., one
 * that is not a root in the tree).
 *
 * @param[in]  cb           The CBOR value to free.  May be NULL, or a root object.
 * @param[in]  CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 */
void cn_cbor_free(cn_cbor *cb CBOR_CONTEXT);

/**
 * Write a CBOR value and all of the child values.
 *
 * @param[in]  buf        The buffer into which to write
 * @param[in]  buf_offset The offset (in bytes) from the beginning of the buffer
 *                        to start writing at
 * @param[in]  buf_size   The total length (in bytes) of the buffer
 * @param[in]  cb         [description]
 * @return                -1 on fail, or number of bytes written
 */
int cn_cbor_encoder_write(uint8_t *buf,
                          size_t buf_offset,
                          size_t buf_size,
                          const cn_cbor *cb);

/**
 * Create a CBOR map.
 *
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created map, or NULL on error
 */
cn_cbor *cn_cbor_map_create(CBOR_CONTEXT_COMMA cn_cbor_errback *errp);

/**
 * Create a CBOR byte string.  The data in the byte string is *not* owned
 * by the CBOR object, so it is not freed automatically.
 *
 * @param[in]   data         The data
 * @param[in]   len          The number of bytes of data
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor *cn_cbor_data_create(const uint8_t *data, int len CBOR_CONTEXT,
                             cn_cbor_errback *errp);

/**
 * Create a CBOR UTF-8 string.  The data is not checked for UTF-8 correctness.
 * The data being stored in the string is *not* owned the CBOR object, so it is
 * not freed automatically.
 *
 * @note: Do NOT use this function with untrusted data.  It calls strlen, and
 * relies on proper NULL-termination.
 *
 * @param[in]   data         NULL-terminated UTF-8 string
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor *cn_cbor_string_create(const char *data
                                   CBOR_CONTEXT,
                               cn_cbor_errback *errp);

/**
 * Create a CBOR integer (either positive or negative).
 *
 * @param[in]   value    the value of the integer
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor *cn_cbor_int_create(int64_t value
                                CBOR_CONTEXT,
                            cn_cbor_errback *errp);

#ifndef CBOR_NO_FLOAT
/**
 * Create a CBOR float.
 *
 * @param[in]   value    the value of the float
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor *cn_cbor_float_create(float value
                                  CBOR_CONTEXT,
                              cn_cbor_errback *errp);

/**
 * Create a CBOR double.
 *
 * @param[in]   value    the value of the double
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor *cn_cbor_double_create(double value
                                   CBOR_CONTEXT,
                               cn_cbor_errback *errp);
#endif /* CBOR_NO_FLOAT */

/**
 * Put a CBOR object into a map with a CBOR object key.  Duplicate checks are NOT
 * currently performed.
 *
 * @param[in]   cb_map       The map to insert into
 * @param[in]   key          The key
 * @param[in]   cb_value     The value
 * @param[out]  errp         Error
 * @return                   True on success
 */
unsigned char cn_cbor_map_put(cn_cbor *cb_map,
                              cn_cbor *cb_key, cn_cbor *cb_value,
                              cn_cbor_errback *errp);

/**
 * Put a CBOR object into a map with an integer key.  Duplicate checks are NOT
 * currently performed.
 *
 * @param[in]   cb_map       The map to insert into
 * @param[in]   key          The integer key
 * @param[in]   cb_value     The value
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error
 * @return                   True on success
 */
unsigned char cn_cbor_mapput_int(cn_cbor *cb_map,
                                 int64_t key, cn_cbor *cb_value CBOR_CONTEXT,
                                 cn_cbor_errback *errp);

/**
 * Put a CBOR object into a map with a string key.  Duplicate checks are NOT
 * currently performed.
 *
 * @note: do not call this routine with untrusted string data.  It calls
 * strlen, and requires a properly NULL-terminated key.
 *
 * @param[in]   cb_map       The map to insert into
 * @param[in]   key          The string key
 * @param[in]   cb_value     The value
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error
 * @return                   True on success
 */
unsigned char cn_cbor_mapput_string(cn_cbor *cb_map,
                                    const char *key, cn_cbor *cb_value CBOR_CONTEXT,
                                    cn_cbor_errback *errp);

/**
 * Create a CBOR array
 *
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor *cn_cbor_array_create(CBOR_CONTEXT_COMMA cn_cbor_errback *errp);

/**
 * Append an item to the end of a CBOR array.
 *
 * @param[in]   cb_array  The array into which to insert
 * @param[in]   cb_value  The value to insert
 * @param[out]  errp      Error
 * @return                True on success
 */
unsigned char cn_cbor_array_append(cn_cbor *cb_array,
                                   cn_cbor *cb_value,
                                   cn_cbor_errback *errp);

unsigned char* codec_demo_encode(int* size_out, unsigned short list_num_in, codec_data_list_t* data_in);

codec_data_list_t* codec_demo_decode(unsigned char* encoded_in, unsigned short len, unsigned short* list_num_out);

#endif /* UC_CBOR_H */

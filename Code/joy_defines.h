#ifndef JOY_DEFINES_H
#define JOY_DEFINES_H

#define Assert(cond) if(!(cond)){ *((int*)0) = 0;}
#define ASSERT(cond) if(!(cond)){ *((int*)0) = 0;}

#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))
#define InvalidCodePath Assert(!"Invalid code path!");

#define INVALID_CODE_PATH Assert(!"Invalid code path!");
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

#define Kilobytes(count) ((count) * 1000)
#define Megabytes(count) ((count) * 1000000)
#define Gigabytes(count) ((count) * 1000000000)

#define Kibibytes(count) ((count) * 1024)
#define Mibibytes(count) ((count) * 1024 * 1024)
#define Gibibytes(count) ((count) * 1024 * 1024 * 1024)

#define GlobalVariable static
#define InternalFunction static
#define LocalAsGlobal static

#define GLOBAL_VARIABLE static
#define INTERNAL_FUNCTION static
#define LOCAL_AS_GLOBAL static

#ifndef Min
#define Min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef Max
#define Max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef Abs
#define Abs(a) ((a) >= 0) ? (a) : -(a)
#endif

#define JOY_TRUE 1
#define JOY_FALSE 0

#define DLIST_REFLECT_PTRS(value, next, prev) {\
    (value).##next = &(value); \
    (value).##prev = &(value);}

#define DLIST_FREE_IS_EMPTY(free_value, next) ((free_value).##next == &(free_value))

#define DLIST_INSERT_AFTER_SENTINEL(entry_ptr, sent_value, next, prev) \
{\
    (entry_ptr)->##next = (sent_value).##next##; \
    (entry_ptr)->##prev = &(sent_value); \
    (entry_ptr)->##prev##->##next = (entry_ptr); \
    (entry_ptr)->##next##->##prev = (entry_ptr);}

#define DLIST_INSERT_BEFORE_SENTINEL(entry_ptr, sent_value, next, prev) \
{\
    (entry_ptr)->##next = &(sent_value);\
    (entry_ptr)->##prev = (sent_value).##prev##; \
    (entry_ptr)->##prev##->##next = (entry_ptr); \
    (entry_ptr)->##next##->##prev = (entry_ptr);}

#define DLIST_INSERT_AFTER(entry_ptr, after_ptr, next, prev) \
{\
    (entry_ptr)->##next = (after_ptr)->##next##; \
    (entry_ptr)->##prev = (after_ptr); \
    (entry_ptr)->##prev##->##next = (entry_ptr); \
    (entry_ptr)->##next##->##prev = (entry_ptr);}

#define DLIST_INSERT_BEFORE(entry_ptr, before_ptr, next, prev) \
{\
    (entry_ptr)->##next = (before_ptr); \
    (entry_ptr)->##prev = (before_ptr)->##prev##; \
    (entry_ptr)->##prev##->##next = (entry_ptr); \
    (entry_ptr)->##next##->##prev = (entry_ptr);}

#define DLIST_REMOVE_ENTRY(entry_ptr, next, prev) \
{\
    entry_ptr->##next##->##prev = entry_ptr->##prev##;\
    entry_ptr->##prev##->##next = entry_ptr->##next##;}


#endif
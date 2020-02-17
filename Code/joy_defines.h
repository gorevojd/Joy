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

#define JOY_INIT_SENTINELS_LINKS(name, next, prev) {name.##next = &name; name.##prev = &name;}

#endif
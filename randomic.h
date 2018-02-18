/*
randomic.h - Portable, single-file, atomic PRNG library, using the smallprng algorithm and standard atomics

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring
rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software.
If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

/*
randomic supports the following three configurations:
#define RANDOMIC_EXTERN
    Default, should be used when using randomic in multiple compilation units within the same project.
#define RANDOMIC_IMPLEMENTATION
    Must be defined in exactly one source file within a project for randomic to be found by the linker.
#define RANDOMIC_STATIC
    Defines all randomic functions as static, useful if randomic is only used in a single compilation unit.

randomic usage:
    The randomic type represents a PRNG context and should be initialized using randomicSeed before being used for anything.
    Usually you'll want either randomicFloat or randomicDouble, which return a random number in the given range (inclusive).
    If needed randomicNext can be used to get the raw uint32 output of the pseudo-random generator (from 0 to UINT32_MAX).

randomic algorithm:
    Randomic uses the smallprng algorithm from http://burtleburtle.net/bob/rand/smallprng.html as it is public domain, which the
    more well-known alternatives (such as Mersenne Twister) are not. As described on the linked page, it is non-cryptographic.
*/

//include only once
#ifndef RANDOMIC_H
#define RANDOMIC_H

//process configuration
#ifdef RANDOMIC_STATIC
    #define RANDOMIC_IMPLEMENTATION
    #define RADEF static
#else //RANDOMIC_EXTERN
    #define RADEF extern
#endif

//includes
#include <stdatomic.h>
#include <stdint.h>

//types
typedef _Atomic struct randomic {
    uint32_t a, b, c, d;
} randomic;

//function declarations
RADEF void randomicSeed(randomic*, uint32_t);
RADEF float randomicFloat(randomic*, float, float);
RADEF double randomicDouble(randomic*, double, double);
RADEF uint32_t randomicNext(randomic*);

//implementation section
#ifdef RANDOMIC_IMPLEMENTATION

//function declarations
static struct randomic randomicInternal(struct randomic);

//public functions
RADEF void randomicSeed (randomic* rdic, uint32_t seed) {
    struct randomic ctx;
    ctx.a = 0xf1ea5eed;
    ctx.b = ctx.c = ctx.d = seed;
    for (int i = 0; i < 20; i++)
        ctx = randomicInternal(ctx);
    atomic_store(rdic, ctx);
}
RADEF float randomicFloat (randomic* rdic, float a, float b) {
    return a + (b-a)*((float)randomicNext(rdic)/(float)UINT32_MAX);
}
RADEF double randomicDouble (randomic* rdic, double a, double b) {
    return a + (b-a)*((double)randomicNext(rdic)/(double)UINT32_MAX);
}
RADEF uint32_t randomicNext (randomic* rdic) {
    struct randomic ctx = atomic_load(rdic), ntx;
    while (!atomic_compare_exchange_weak(rdic, &ctx, (ntx = randomicInternal(ctx))));
    return ntx.d;
}

//internal functions
static struct randomic randomicInternal (struct randomic ctx) {
    uint32_t e = ctx.a - ((ctx.b << 27)|(ctx.b >> 5));
    ctx.a = ctx.b ^ ((ctx.c << 17)|(ctx.c >> 15));
    ctx.b = ctx.c + ctx.d;
    ctx.c = ctx.d + e;
    ctx.d = e + ctx.a;
    return ctx;
}

#endif //RANDOMIC_IMPLEMENTATION
#endif //RANDOMIC_H
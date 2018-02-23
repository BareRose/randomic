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
    The struct randomic type represents a PRNG context and should be initialized and seeded using randomicSeed before usage.
    Usually you'll want either randomicFloatCO/CC or randomicDoubleCO/CC, which return pseudo-random numbers between 0.0 and 1.0.
    The suffix CO indicates a half-open range [0.0, 1.0) (including 0.0 but not 1.0), CC a closed one (including 0.0 and 1.0).
    For percentage chances the CO functions should be used, e.g. if (randomicFloat(...) < 0.5f) for a uniform 50% probability.
    For inclusive ranges the CC functions should be used, e.g. randomicFloat(...)*12.0f for an inclusive range of [0.0, 12.0].
    If needed randomicNext can be used to get the raw uint32 output of the pseudo-random generator (from 0 to UINT32_MAX).

randomic details:
    For float, randomicFloatCO produces one of 2^24 possible values with perfect uniformity in its half-open range [0.0, 1.0),
    whereas randomicFloatCC uses more of float's total precision at a loss of uniformity and provides a closed range [0.0, 1.0].
    For double, randomicDoubleCO produces one of 2^32 possible values with perfect uniformity in its half-open range [0.0, 1.0),
    whereas randomicDoubleCC also produces one of 2^32 possible values with possibly less uniformity but a closed range [0.0, 1.0].
    With only 2^32 possible values these do not use all of double's available precision, but only require one call to randomicNext.

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

//structs
struct randomic {
    _Atomic struct randomic_ctx {
        uint32_t a, b, c, d;
    } ctx;
};

//function declarations
RADEF void randomicSeed(struct randomic*, uint32_t);
RADEF float randomicFloatCO(struct randomic*);
RADEF float randomicFloatCC(struct randomic*);
RADEF double randomicDoubleCO(struct randomic*);
RADEF double randomicDoubleCC(struct randomic*);
RADEF uint32_t randomicNext(struct randomic*);

//implementation section
#ifdef RANDOMIC_IMPLEMENTATION

//function declarations
static struct randomic_ctx randomicStep(struct randomic_ctx);

//public functions
RADEF void randomicSeed (struct randomic* rdic, uint32_t seed) {
    //initialization as per smallprng algorithm
    struct randomic_ctx ctx;
    ctx.a = 0xf1ea5eed;
    ctx.b = ctx.c = ctx.d = seed;
    for (int i = 0; i < 20; i++)
        ctx = randomicStep(ctx);
    //store initialized state atomically
    atomic_store(&rdic->ctx, ctx);
}
RADEF float randomicFloatCO (struct randomic* rdic) {
    //returns a random float in the range [0.0, 1.0) (not including 1.0)
    //provides perfect uniformity but only 2^24 of 2^32 possible values
    return (float)(randomicNext(rdic) >> 8)/16777216.0f;
}
RADEF float randomicFloatCC (struct randomic* rdic) {
    //returns a random float in the range [0.0, 1.0] (including 0.0 and 1.0)
    //uses more of float's available precision at the cost of uniformity
    return (float)randomicNext(rdic)/(float)UINT32_MAX;
}
RADEF double randomicDoubleCO (struct randomic* rdic) {
    //returns a random double in the range [0.0, 1.0) (not including 1.0)
    //provides perfect uniformity with a full 2^32 possible values
    return (double)randomicNext(rdic)/4294967296.0;
}
RADEF double randomicDoubleCC (struct randomic* rdic) {
    //returns a random double in the range [0.0, 1.0] (including 0.0 and 1.0)
    //provides possibly less uniformity as a result of the odd division
    return (double)randomicNext(rdic)/(double)UINT32_MAX;
}
RADEF uint32_t randomicNext (struct randomic* rdic) {
    //returns a random uint32 (raw output of the generator)
    struct randomic_ctx ctx = atomic_load(&rdic->ctx), ntx;
    while (!atomic_compare_exchange_weak(&rdic->ctx, &ctx, (ntx = randomicStep(ctx))));
    return ntx.d;
}

//internal functions
static struct randomic_ctx randomicStep (struct randomic_ctx ctx) {
    //advances the PRNG state by a single step
    uint32_t e = ctx.a - ((ctx.b << 27)|(ctx.b >> 5));
    ctx.a = ctx.b ^ ((ctx.c << 17)|(ctx.c >> 15));
    ctx.b = ctx.c + ctx.d;
    ctx.c = ctx.d + e;
    ctx.d = e + ctx.a;
    return ctx;
}

#endif //RANDOMIC_IMPLEMENTATION
#endif //RANDOMIC_H
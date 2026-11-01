#ifndef COMPILER_SPECIFIC_H
#define COMPILER_SPECIFIC_H
/*
 * Compiler-specific data and function attributes for AmigaOS 3 SAS/C.
 */

#if __STDC__
#define __CLIB_PROTOTYPE(a) a
#else
#define __CLIB_PROTOTYPE(a) ()
#endif

#ifndef __ASM__
#ifdef __SASC
#define __ASM__ __asm
#else
#define __ASM__
#endif
#endif

#ifndef __REG__
#if defined(__SASC) || defined(__MAXON__) || defined(__STORM__)
#define __REG__(r, p) register __ ## r p
#elif defined(__VBCC__)
#define __REG__(r, p) __reg(#r) p
#elif defined(__GNUC__) && defined(AMIGA)
#define __REG__(r, p) p __asm(#r)
#elif defined(_DCC)
#define __REG__(r, p) __ ## r p
#else
#define __REG__(r, p) p
#endif
#endif

#ifndef __STDARGS__
#if (defined(__GNUC__) && defined(AMIGA)) || defined(__SASC)
#define __STDARGS__ __stdargs
#elif defined(_DCC)
#define __STDARGS__ __stkargs
#else
#define __STDARGS__
#endif
#endif

#ifndef __SAVE_DS__
#if defined(_DCC) || (defined(__GNUC__) && defined(AMIGA)) || defined(__SASC) || defined(__STORM__) || defined(__VBCC__)
#define __SAVE_DS__ __saveds
#else
#define __SAVE_DS__
#endif
#endif

#ifndef __FAR__
#if defined(_DCC) || defined(__MAXON__) || defined(__SASC) || defined(__STORM__) || defined(__VBCC__)
#define __FAR__ __far
#else
#define __FAR__
#endif
#endif

#ifndef __INTERRUPT__
#if defined(_DCC) || (defined(__GNUC__) && defined(AMIGA)) || defined(__MAXON__) || defined(__SASC) || defined(__STORM__)
#define __INTERRUPT__ __interrupt
#elif defined(__VBCC__)
#define __INTERRUPT__ __amigainterrupt
#else
#define __INTERRUPT__
#endif
#endif

#ifndef __CHIP__
#if defined(_DCC) || (defined(__GNUC__) && defined(AMIGA)) || defined(__MAXON__) || defined(__SASC) || defined(__STORM__) || defined(__VBCC__)
#define __CHIP__ __chip
#else
#define __CHIP__
#endif
#endif

#ifndef __FAST__
#if defined(_DCC) || (defined(__GNUC__) && defined(AMIGA)) || defined(__MAXON__) || defined(__SASC) || defined(__STORM__) || defined(__VBCC__)
#define __FAST__ __fast
#else
#define __FAST__
#endif
#endif

#endif /* COMPILER_SPECIFIC_H */

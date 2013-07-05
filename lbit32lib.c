/*
** $Id: lbitlib.c,v 1.18 2013/03/19 13:19:12 roberto Exp $
** Standard library for bitwise operations
** See Copyright Notice in lua.h
**
*** transplanted from lua-5.2.2 and modified by microwish@gmail.com
*/

#define lbitlib_c
#define LUA_LIB

#include <lua.h>

#include <lauxlib.h>
#include <lualib.h>

#include <ctype.h>
#include <stdlib.h>

/* number of bits to consider in a number */
#if !defined(LUA_NBITS)
#define LUA_NBITS  32
#endif


#define ALLONES		(~(((~(lua_Unsigned)0) << (LUA_NBITS - 1)) << 1))

/* macro to trim extra bits */
#define trim(x)		((x) & ALLONES)


/* builds a number with 'n' ones (1 <= n <= LUA_NBITS) */
#define mask(n)		(~((ALLONES << 1) << ((n) - 1)))


/*
@@ LUAI_BITSINT defines the number of bits in an int.
** CHANGE here if Lua cannot automatically detect the number of bits of
** your machine. Probably you do not need to change this.
*
** lua-5.2.2 luaconf.h
*/
/* avoid overflows in comparison */
#if INT_MAX-20 < 32760          /* { */
#define LUAI_BITSINT    16
#elif INT_MAX > 2147483640L     /* }{ */
/* int has at least 32 bits */
#define LUAI_BITSINT    32
#else                           /* }{ */
#error "you must define LUA_BITSINT with number of bits in an integer"
#endif                          /* } */

/*
@@ LUA_INT32 is an signed integer with exactly 32 bits.
@@ LUAI_UMEM is an unsigned integer big enough to count the total
@* memory used by Lua.
@@ LUAI_MEM is a signed integer big enough to count the total memory
@* used by Lua.
** CHANGE here if for some weird reason the default definitions are not
** good enough for your machine. Probably you do not need to change
** this.
*
** lua-5.2.2 luaconf.h
*/
#if LUAI_BITSINT >= 32          /* { */
#define LUA_INT32       int
#define LUAI_UMEM       size_t
#define LUAI_MEM        ptrdiff_t
#else                           /* }{ */
/* 16-bit ints */
#define LUA_INT32       long
#define LUAI_UMEM       unsigned long
#define LUAI_MEM        long
#endif                          /* } */

/*
@@ LUA_UNSIGNED is the integral type used by lua_pushunsigned/lua_tounsigned.
** It must have at least 32 bits.
*
** lua-5.2.2 luaconf.h
*/
#define LUA_UNSIGNED    unsigned LUA_INT32

/* unsigned integer type
 *
 * lua-5.2.2 lua.h
 */
typedef LUA_UNSIGNED lua_Unsigned;


typedef lua_Unsigned b_uint;



/*
** Some tricks with doubles
*
** lua-5.2.2 luaconf.h
*/

#if defined(LUA_NUMBER_DOUBLE) && !defined(LUA_ANSI)	/* { */
/*
** The next definitions activate some tricks to speed up the
** conversion from doubles to integer types, mainly to LUA_UNSIGNED.
**
@@ LUA_MSASMTRICK uses Microsoft assembler to avoid clashes with a
** DirectX idiosyncrasy.
**
@@ LUA_IEEE754TRICK uses a trick that should work on any machine
** using IEEE754 with a 32-bit integer type.
**
@@ LUA_IEEELL extends the trick to LUA_INTEGER; should only be
** defined when LUA_INTEGER is a 32-bit integer.
**
@@ LUA_IEEEENDIAN is the endianness of doubles in your machine
** (0 for little endian, 1 for big endian); if not defined, Lua will
** check it dynamically for LUA_IEEE754TRICK (but not for LUA_NANTRICK).
**
@@ LUA_NANTRICK controls the use of a trick to pack all types into
** a single double value, using NaN values to represent non-number
** values. The trick only works on 32-bit machines (ints and pointers
** are 32-bit values) with numbers represented as IEEE 754-2008 doubles
** with conventional endianess (12345678 or 87654321), in CPUs that do
** not produce signaling NaN values (all NaNs are quiet).
*/

/* Microsoft compiler on a Pentium (32 bit) ? */
#if defined(LUA_WIN) && defined(_MSC_VER) && defined(_M_IX86)	/* { */

#define LUA_MSASMTRICK
#define LUA_IEEEENDIAN		0
#define LUA_NANTRICK

/* pentium 32 bits? */
#elif defined(__i386__) || defined(__i386) || defined(__X86__) /* }{ */

#define LUA_IEEE754TRICK
#define LUA_IEEELL
#define LUA_IEEEENDIAN		0
#define LUA_NANTRICK

/* pentium 64 bits? */
#elif defined(__x86_64)						/* }{ */

#define LUA_IEEE754TRICK
#define LUA_IEEEENDIAN		0

#elif defined(__POWERPC__) || defined(__ppc__)			/* }{ */

#define LUA_IEEE754TRICK
#define LUA_IEEEENDIAN		1

#else								/* }{ */

/* assume IEEE754 and a 32-bit integer type */
#define LUA_IEEE754TRICK

#endif								/* } */

#endif							/* } */


/*
** lua_number2unsigned is a macro to convert a lua_Number to a lua_Unsigned.
** lua_unsigned2number is a macro to convert a lua_Unsigned to a lua_Number.
** luai_hashnum is a macro to hash a lua_Number value into an integer.
** The hash must be deterministic and give reasonable values for
** both small and large values (outside the range of integers).
*
** lua-5.2.2 llimits.h
*/

/* On a Microsoft compiler, use assembler */
#if defined(_MSC_VER) /* { */

#define lua_number2unsigned(i,n)  \
  {__int64 l; __asm {__asm fld n   __asm fistp l} i = (unsigned int)l;}

#elif defined(LUA_IEEE754TRICK)		/* }{ */
/* the next trick should work on any machine using IEEE754 with
   a 32-bit int type */

union luai_Cast_v522 { double l_d; LUA_INT32 l_p[2]; };

#if !defined(LUA_IEEEENDIAN)	/* { */
#define LUAI_EXTRAIEEE	\
  static const union luai_Cast_v522 ieeeendian = {-(33.0 + 6755399441055744.0)};
#define LUA_IEEEENDIANLOC	(ieeeendian.l_p[1] == 33)
#else /* }{ */
#define LUA_IEEEENDIANLOC	LUA_IEEEENDIAN
#define LUAI_EXTRAIEEE		/* empty */
#endif				/* } */

#define lua_number2int32(i,n,t) \
  { LUAI_EXTRAIEEE \
    volatile union luai_Cast_v522 u; u.l_d = (n) + 6755399441055744.0; \
    (i) = (t)u.l_p[LUA_IEEEENDIANLOC]; }

#define luai_hashnum(i,n)  \
  { volatile union luai_Cast_v522 u; u.l_d = (n) + 1.0;  /* avoid -0 */ \
    (i) = u.l_p[0]; (i) += u.l_p[1]; }  /* add double bits for his hash */

#define lua_number2unsigned(i,n)	lua_number2int32(i, n, lua_Unsigned)

#endif				/* } */


/* the following definitions always work, but may be slow */

#if !defined(lua_number2unsigned)	/* { */
/* the following definition assures proper modulo behavior */
#if defined(LUA_NUMBER_DOUBLE) || defined(LUA_NUMBER_FLOAT) /* { */
#include <math.h>
#define SUPUNSIGNED	((lua_Number)(~(lua_Unsigned)0) + 1)
#define lua_number2unsigned(i,n)  \
	((i)=(lua_Unsigned)((n) - floor((n)/SUPUNSIGNED)*SUPUNSIGNED))
#else /* }{ */
#define lua_number2unsigned(i,n)	((i)=(lua_Unsigned)(n))
#endif /* } */
#endif /* } */


#if !defined(lua_unsigned2number) /* { */
/* on several machines, coercion from unsigned to double is slow,
   so it may be worth to avoid */
#define lua_unsigned2number(u)  \
    (((u) <= (lua_Unsigned)INT_MAX) ? (lua_Number)(int)(u) : (lua_Number)(u))
#endif /* } */


#if defined(ltable_c) && !defined(luai_hashnum) /* { */

#include <float.h>
#include <math.h>

#define luai_hashnum(i,n) { int e;  \
  n = l_mathop(frexp)(n, &e) * (lua_Number)(INT_MAX - DBL_MAX_EXP);  \
  lua_number2int(i, n); i += e; }

#endif /* } */


#ifndef lua_str2number
#define lua_str2number(s,p)     strtod((s), (p))
#endif

#ifndef cast
#define cast(t, exp)    ((t)(exp))
#endif

#ifndef cast_num
#define cast_num(i)     cast(lua_Number, (i))
#endif

static int luaO_str2d (const char *s, lua_Number *result) {
  char *endptr;
  *result = lua_str2number(s, &endptr);
  if (endptr == s) return 0;  /* conversion failed */
  if (*endptr == 'x' || *endptr == 'X')  /* maybe an hexadecimal constant? */
    *result = cast_num(strtoul(s, &endptr, 16));
  if (*endptr == '\0') return 1;  /* most common case */
  while (isspace(cast(unsigned char, *endptr))) endptr++;
  if (*endptr != '\0') return 0;  /* invalid trailing characters? */
  return 1;
}

/**
 * modified from lua-5.2.2 lapi.c
 * to avoid direct use of basic types and funcs & macros
 */
static lua_Unsigned lua_tounsignedx(lua_State *L, int idx, int *isnum)
{
	lua_Number num;
	lua_Unsigned res;

	switch (lua_type(L, idx)) {
		case LUA_TNUMBER:
		{
			num = lua_tonumber(L, idx);
			lua_number2unsigned(res, num);
			if (isnum) *isnum = 1;
			return res;
		}
		case LUA_TSTRING:
		{
			const char *s;
			s = lua_tostring(L, idx);
			if (luaO_str2d(s, &num)) {
				if (isnum) *isnum = 1;
				lua_number2unsigned(res, num);
				return res;
			} else {
				if (isnum) *isnum = 0;
				return 0;
			}
		}
		default:
			if (isnum) *isnum = 0;
			return 0;
	}
}

/**
 * modified from lua-5.2.2 lapi.c
 * to avoid direct use of basic types and funcs & macros
 */
static void lua_pushunsigned(lua_State *L, lua_Unsigned u)
{
	lua_Number n = lua_unsigned2number(u);

	lua_pushnumber(L, n);
}

static void tag_error (lua_State *L, int narg, int tag) {
  luaL_typerror(L, narg, lua_typename(L, tag));
}

/**
 * modified from lua-5.2.2 lauxlib.c
 */
static lua_Unsigned luaL_checkunsigned (lua_State *L, int narg) {
  int isnum;
  lua_Unsigned d = lua_tounsignedx(L, narg, &isnum);
  if (!isnum)
    tag_error(L, narg, LUA_TNUMBER);
  return d;
}


static b_uint andaux (lua_State *L) {
  int i, n = lua_gettop(L);
  b_uint r = ~(b_uint)0;
  for (i = 1; i <= n; i++)
    r &= luaL_checkunsigned(L, i);
  return trim(r);
}


static int b_and (lua_State *L) {
  b_uint r = andaux(L);
  lua_pushunsigned(L, r);
  return 1;
}


static int b_test (lua_State *L) {
  b_uint r = andaux(L);
  lua_pushboolean(L, r != 0);
  return 1;
}


static int b_or (lua_State *L) {
  int i, n = lua_gettop(L);
  b_uint r = 0;
  for (i = 1; i <= n; i++)
    r |= luaL_checkunsigned(L, i);
  lua_pushunsigned(L, trim(r));
  return 1;
}


static int b_xor (lua_State *L) {
  int i, n = lua_gettop(L);
  b_uint r = 0;
  for (i = 1; i <= n; i++)
    r ^= luaL_checkunsigned(L, i);
  lua_pushunsigned(L, trim(r));
  return 1;
}


static int b_not (lua_State *L) {
  b_uint r = ~luaL_checkunsigned(L, 1);
  lua_pushunsigned(L, trim(r));
  return 1;
}


static int b_shift (lua_State *L, b_uint r, int i) {
  if (i < 0) {  /* shift right? */
    i = -i;
    r = trim(r);
    if (i >= LUA_NBITS) r = 0;
    else r >>= i;
  }
  else {  /* shift left */
    if (i >= LUA_NBITS) r = 0;
    else r <<= i;
    r = trim(r);
  }
  lua_pushunsigned(L, r);
  return 1;
}


static int b_lshift (lua_State *L) {
  return b_shift(L, luaL_checkunsigned(L, 1), luaL_checkint(L, 2));
}


static int b_rshift (lua_State *L) {
  return b_shift(L, luaL_checkunsigned(L, 1), -luaL_checkint(L, 2));
}


static int b_arshift (lua_State *L) {
  b_uint r = luaL_checkunsigned(L, 1);
  int i = luaL_checkint(L, 2);
  if (i < 0 || !(r & ((b_uint)1 << (LUA_NBITS - 1))))
    return b_shift(L, r, -i);
  else {  /* arithmetic shift for 'negative' number */
    if (i >= LUA_NBITS) r = ALLONES;
    else
      r = trim((r >> i) | ~(~(b_uint)0 >> i));  /* add signal bit */
    lua_pushunsigned(L, r);
    return 1;
  }
}


static int b_rot (lua_State *L, int i) {
  b_uint r = luaL_checkunsigned(L, 1);
  i &= (LUA_NBITS - 1);  /* i = i % NBITS */
  r = trim(r);
  r = (r << i) | (r >> (LUA_NBITS - i));
  lua_pushunsigned(L, trim(r));
  return 1;
}


static int b_lrot (lua_State *L) {
  return b_rot(L, luaL_checkint(L, 2));
}


static int b_rrot (lua_State *L) {
  return b_rot(L, -luaL_checkint(L, 2));
}


/*
** get field and width arguments for field-manipulation functions,
** checking whether they are valid.
** ('luaL_error' called without 'return' to avoid later warnings about
** 'width' being used uninitialized.)
*/
static int fieldargs (lua_State *L, int farg, int *width) {
  int f = luaL_checkint(L, farg);
  int w = luaL_optint(L, farg + 1, 1);
  luaL_argcheck(L, 0 <= f, farg, "field cannot be negative");
  luaL_argcheck(L, 0 < w, farg + 1, "width must be positive");
  if (f + w > LUA_NBITS)
    luaL_error(L, "trying to access non-existent bits");
  *width = w;
  return f;
}


static int b_extract (lua_State *L) {
  int w;
  b_uint r = luaL_checkunsigned(L, 1);
  int f = fieldargs(L, 2, &w);
  r = (r >> f) & mask(w);
  lua_pushunsigned(L, r);
  return 1;
}


static int b_replace (lua_State *L) {
  int w;
  b_uint r = luaL_checkunsigned(L, 1);
  b_uint v = luaL_checkunsigned(L, 2);
  int f = fieldargs(L, 3, &w);
  int m = mask(w);
  v &= m;  /* erase bits outside given width */
  r = (r & ~(m << f)) | (v << f);
  lua_pushunsigned(L, r);
  return 1;
}


static const luaL_Reg bitlib[] = {
  {"arshift", b_arshift},
  {"band", b_and},
  {"bnot", b_not},
  {"bor", b_or},
  {"bxor", b_xor},
  {"btest", b_test},
  {"extract", b_extract},
  {"lrotate", b_lrot},
  {"lshift", b_lshift},
  {"replace", b_replace},
  {"rrotate", b_rrot},
  {"rshift", b_rshift},
  {NULL, NULL}
};


static int setreadonly(lua_State *L)
{
	return luaL_error(L, "Must not update the readonly");
}

LUALIB_API int luaopen_bit32(lua_State *L)
{
	//main table for this module
	lua_createtable(L, 1, 0);

	//metatable for main table
	lua_createtable(L, 0, 2);

	luaL_register(L, "bit32", bitlib);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, setreadonly);
	lua_setfield(L, -2, "__newindex");

	lua_setmetatable(L, -2);

	return 1;
}

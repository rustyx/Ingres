/*
** Ingres bzarch.h created on Tue Dec 13 18:19:14 CET 2022
** by the /home/me/lvnl/Ingres/src/bin/mkbzarch shell script
*/

#if ! (defined(BUILD_ARCH32) || defined(BUILD_ARCH64))
# define BUILD_ARCH64
#endif
#if defined(BUILD_ARCH32)
# ifndef int_lnx
# define int_lnx
# endif
#else
# ifndef a64_lnx
# define a64_lnx
# endif
#endif
#define conf_BUILD_ARCH_64
# define conf_libc6
# define conf_DBL
# define conf_CAS_ENABLED
# define conf_SVR_AR
# define conf_WITH_GEO
# define conf_ADI_MAX_OPERANDS 5
# define conf_all_defined
# define PTR_BITS_64
# define LP64
# define SCALARP long
# define SCALARP_IS_LONG
# if !defined(MAINWIN) || defined(conf_W4GL)
# define UNIX
# endif
# define BAD_BITFIELDS
# ifdef __cplusplus
# define GLOBALREF extern "C"
# else  /* __cplusplus */
# define GLOBALREF extern
# endif  /* __cplusplus */
# define GLOBALDEF
# define NODY
# define II_DMF_MERGE
# define INGRES65
# define GCF65
# define LOADDS
# define FILE_UNIX
# define IEEE_FLOAT
# define OS_THREADS_USED
# define POSIX_THREADS
# define NO_INTERNAL_THREADS
# define LNX
# define ZERO_FILL = {0}
# define READONLY	const
# define WSCREADONLY const
# define GLOBALCONSTDEF const
# define GLOBALCONSTREF extern const
# define VOLATILE volatile
# define LITTLE_ENDIAN_INT
# define BITSPERBYTE	8
# define	ALIGN_I2	0
# define	ALIGN_I4	0
# define	ALIGN_I8	7
# define	ALIGN_F4	0
# define	ALIGN_F8	3
# define I2ASSIGN_MACRO(a,b) ((*(i2 *)&b) = (*(i2 *)&a))
# define I4ASSIGN_MACRO(a,b) ((*(i4 *)&b) = (*(i4 *)&a))
# define I8ASSIGN_MACRO(a,b) ((*(i8 *)&b) = (*(i8 *)&a))
# define F4ASSIGN_MACRO(a,b) ((*(f4 *)&b) = (*(f4 *)&a))
# define F8ASSIGN_MACRO(a,b) ((*(f8 *)&b) = (*(f8 *)&a))
# define ALIGN_RESTRICT   double
# define I1_CHECK	(i4)
/* 6.1 renamed this... */
# define I1_CHECK_MACRO( x )	I1_CHECK( (x) )
# define CSASCII
# define DOUBLEBYTE
# define INGRESII
# define xEX_HARDMATH
# define xCL_FLOAT_H_EXISTS
# define xCL_ABS_EXISTS
# define xCL_FINITE_EXISTS
# define xCL_RPM_EXISTS
# define HAS_VARIADIC_MACROS

/* End of bzarch.h */

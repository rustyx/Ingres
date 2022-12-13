
# ifndef __IIAPIDEP_H__
# define __IIAPIDEP_H__

typedef	int		II_BOOL;	
typedef	char		II_CHAR;	
typedef	int		II_INT;		
typedef	signed char	II_INT1;	
typedef	short		II_INT2;	
typedef	int		II_INT4;	
typedef	long long	II_INT8;	
typedef	int		II_LONG;	
typedef	float		II_FLOAT4;	
typedef	double		II_FLOAT8;	
typedef	void *		II_PTR;		
typedef	unsigned char	II_UCHAR;	
typedef	unsigned char	II_UINT1;	
typedef	unsigned short	II_UINT2;	
typedef	unsigned int	II_UINT4;	
typedef	unsigned long long II_UINT8;	
typedef	unsigned int	II_ULONG;	

# define II_VOID	void 
# define II_FAR
#ifdef __cplusplus
# define II_EXTERN   extern "C"
#else
# define II_EXTERN   extern
# endif
# define II_EXPORT
# define II_CALLBACK

# define II_printf  printf
# define II_sprintf sprintf
# define II_scanf    
#endif 

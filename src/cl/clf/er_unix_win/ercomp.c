/*
**Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <lo.h>
#include <pc.h>
#include <cv.h>
#include <me.h>
#include <si.h>
#include <st.h>
#include <er.h>
#include <cs.h>	    /* Needed for "erloc.h" */
#include "erloc.h"
#ifdef  VMS
#include <fab.h>
#include <rab.h>
#include <rmsdef.h>
#endif
/*
**  Forward and/or External function references.
*/

/*
**  Local defines.
*/
#define MAXLINE		512
#define DELCHAR		'.'
#define ONFLAGBIT	0x10000000
#define HON		01
#define FON		02
#define SON		04
#define APON		010
#define SAON		020
#define SOON		040
#define TON		0100
#define	QON		0400
#define THFSON		0107
#define HFSON		07
#define FOFF		(~02)
#define SOFF		(~04)
#define TMPFAST		"tmp.f"
#define TMPSLOW		"tmp.s"
#define FASTTXT		"txt.f"
#define SLOWTXT		"txt.s"
#define TMPSORT	 	"tmp.srt"
#define SS_FILE		"sqlstate.h"
#define	SSREQSTRING	"SQLSTATE_MAPPING_REQUIRED"
#define NOSS_STRING	"NO_SQLSTATE_MAPPING"
#define	MISC_ERR_SS	"50000"   /* E_SS50000_MISC_ING_ERRORS */
#define	SS_MAX_LEN	40
#define	SS_MAX_CODES	150
#define END		1

/*	VMS only:
**      This structure is used in calls to open, create, read, write, and 
**      close the files used by the ERCOMPILE program.
*/
#ifdef  VMS
typedef struct _FILE_CONTEXT
{
    struct FAB      fc_fab;             /* RMS FAB. */
    struct RAB      fc_rab;		/* RMS RAB. */
    i4		    *fc_result_size;	/* Location to store result size. */
    i4		    *fc_line_number;	/* Location to store line number. */
}   FILE_CONTEXT;
#else
# define FILE_CONTEXT FILE
#endif

/*
** context information passed down to record handlers while handling
** slow messages.  We make fp a pointer for the VMS definition so that
** we can simply set up the address once, and pass ->fp to put_record
** without an ifdef.
**
** dat array is coerced into a character buffer when used for message
** text.
*/
typedef struct
{
#ifdef  VMS
	FILE_CONTEXT *fp;
#else
	FILE *fp;
#endif
	i4 pages;		/* index page count */
	i4 pbreak;		/* page break count on current page */
	i4 mcount;		/* message count on current break */
	char *textp;		/* text pointer */
	INDEX_PAGE ip;		/* index page */

	i4 dat[sizeof(INDEX_PAGE)/sizeof(i4)];
} SLOW_CONTEXT;

/*
**  Definition of static variables and forward static functions.
*/
static	    bool	SS_map_req;
static	    i4		SS_codes = 0;
static	    char	SS_list[SS_MAX_CODES][SS_MAX_LEN];

static void build_fast_index(char *input_file_name,
	char *output_file_name, i4 flag);

static void build_slow_index(char *infile, char *outfile, i4 flag);

static VOID build_ss_list(char *fn);

static VOID check_arg(i4 argc, char **argv, char *slow, char *fast,
	char *sqlstate, char *headersuffix, i4 *flag);

static i4 convert_esc(char *buf, i4 length);

static STATUS convert_hex(char *record, i4 record_size, i4 *number);

static VOID delete_file(char *file_name);

static STATUS er__qsortids(char *arr,i4 nel,i4 size,
	int (*compare)(const char *, const char *) );

static bool get_next_rec(char *buf, char **next_char,
	i4 *ln, char **fn, FILE *fp);

static bool get_record(char *record, i4 *result_size, i4 record_size,
	FILE *fp, i4 *line_count);

static bool open_input_file(char *file_name, FILE **fp);

static STATUS parser(i4 argc, char **argv, i4 *flag,
	bool *fnosort, bool *snosort, char *headersuffix, char *fastfile,
	char *slowfile, char *ssfile);

static VOID put_record(FILE *file_context, i4 record_number,
	void *record, i4 record_size);

static char * scan(char *buf, char *stp, i4 *ln, char **fn, FILE *fp);

static VOID scan_slow(char *infile,
	VOID (*hdl)(MESSAGE_ENTRY *, bool, SLOW_CONTEXT *),
	SLOW_CONTEXT *ctx);

static VOID sctx_init(char *outfile, SLOW_CONTEXT *ctx);

static VOID shdr_flush(SLOW_CONTEXT *ctx);

static VOID shdr_hdl(MESSAGE_ENTRY *mess,bool pb,SLOW_CONTEXT *ctx);

static VOID sort_msg(char *inf_name, char *outf_name, i4 flag);

static VOID stxt_flush(SLOW_CONTEXT *ctx);

static VOID stxt_hdl(MESSAGE_ENTRY *mess,bool pb,SLOW_CONTEXT *ctx);

static STATUS take(char *buf, char *start_char,
	char **end_char, char *wordbuf,
	i4 *ln, char **fn, FILE *fp);

static VOID usage(void);

static STATUS validate_sqlstate(char *ss_code);

static i4 write_fastrec(FILE_CONTEXT *file_context,char *record,i4 recordsize);

#ifdef VMS
static VOID close_file(FILE_CONTEXT *file_context);

static VOID create_output_file(FILE_CONTEXT *file_context,
	char *file_name, i4 record_size);

#endif
/*
**	MKMFIN Hints
PROGRAM =	ercompile
NEEDLIBS =	COMPATLIB MALLOCLIB
NO_OPTIM =      rs4_us5 dg8_us5 i64_aix
*/

/*{
** Name: ERcompile - The error message file compiler.
**
** Description:
**	The message compiler creates fast and slow binary message files
**	and message header files.  This program reads a set of class .msg 
**	files that describe the messages to be compiled, and outputs a 
**	fast binary message file, a slow binary message file, and
**	and a set of header files that are placed in the location specified.
**
**	The fast and slow message files are accessed via the ERget and
**	ERlookup calls in the code.  Their location and naming convention
**	is up to the implementor of the CL.
**
**	Command line syntax:
**
**		ercompile [-a] [-l] [-o] [-h] [-s[file]] [-f[file]] classfiles
**		    -a      - Append the message into TMPFAST and TMPSLOW
**		    -l      - Save the temporary files.
**		    -o	    - Sort the temporary files.
**		    -h      - Create the header files.
**		    -t      - Create the fast and slow files named with
**			      the class name, so that the II_MSG_TEST
**			      can be used.  If this is used, it overrides
**			      the -f and -s flags.
**		    -sfile  - Create the named slow message file.
**		    -ffile  - Create the named fast message file.
**		    -qfile  - Use named file to cross-check SQLSTATE status
**			      codes
**
**		    header file name		     - "<class name>.h"
**		    default fast file name	     - "fast_v2.mnx"
**		    default slow file name	     - "slow_v2.mnx"
**		    default SQLSTATE check file	     - "sqlstate.h"
** 
**-----------------------------------------------------------------------------
**	Details on the ERcompile program (from Himuro, who wrote it).
**
** 1. CHARACTER SET
**
**  The ER compiler character set consists of uppercase letters, lowercase let-
**  ters, digits, and special characters.
**
**  A letter is one of the following characters:
**
**	A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
**
**	a b c d e f g h i j k l m n o p q r s t u v w x y z
**
**  ER compiler recognizes both uppercase and lowercase characters as distinct 
**  characters. Therefor, ER compiler is case-sensitive.
**
**  A digit is one of the following characters:
**
**	0 1 2 3 4 5 6 7 8 9
**
**  A special character is one of the following characters:
**
**	= > < + - * % / \ ( ) { } [ ] , . # ' " : ; & | ! ^ ? ~
**
**
** 2. COMMENTS
**
**  A comments is explanatory text inserted within your program.  Comments  are
**  ignored by compiler.
**
**  The slash character  followed  by  an asterisk characrer       introduces a
**  comment. The compiler ignores all characters following the slash / asterisk
**  combination until it encounters an asterisk / slash conbination. 
**  Comments do not nest.
**
**  The following is a comment:
**
**	/ * This is a comment * /
**
**  (This is the same as a C comment, but I had to put the extra spaces in
**   to avoid a compilation error in compiling this file.)
**
**
** 3. IDENTIFIERS
**
**  An identifier is a sequence of letters, underscores (_), and digits.
**  The compiler is case-sensitive.
**
**  The following are examples of identifiers:
**
**	FCMN0001_Help
**	SMNU0010_Out_of_range
**	EQBF0100_DOES_NOT_EXIST_TABLE
**
**
** 4. CLASS DECLARATION
**
**    A class declaration has the following form:
**
**	#define <class id>_class	<class number>
**	#define <class id>_CLASS	<class number>
**
**	    Class id must be three characters.
**	    Class number must be digits.
**
**  The following are examples of class declarations:
**
**	#define CMN_class		 1
**	#define MNU_CLASS		10
**	#define QBF_class		15
**
**
** 5. MESSAGE DECLARATION
**
**  A message declaration has a following form:
**
**	F<class id><message number>_<strings>		<string constants>
**	S<class id><message number>_<strings>		<string constants>
**	E<class id><message number>_<strings>		<string constants>
**
**	    F is fast message id.	
**
**	    S or E is slow message id.
**
**	    Class id  must be the same as class name defined  at class declara-
**	    tion.
**
**	    Message number must be four digits.
**
**	    Strings is a sequence of letters, underscores and digits.
**
**	    String constants are a sequence of zero or more characters enclosed
**	    in double quotes. This is the same as C string constants.
**
**  The following are examples of message declaration:
**
**	FCMN0001_Help			"Help"
**	FMNU0001_Go			"Go(Enter)"
**	SQBF0010_JoinDefs		"QBF - JoinDefs Catalog"
**	SQBF0100_Use			"Use the appropriate menu item to\
**	perform the desired operation on that JoinDef"
**	EQBF0500_Error_Mes		"ERROR: Invalid JoinDef name specified\
**	. Please enter valid name."
**
**
** 6. C PREPROCESSOR COMMANDS
**
**  You can describe C preprocessor commands.
**
**  Forms of Command:
**
**	#define identifier 		token-string
**
**	    Note that C preprocessor commands must be describe after class def-
**	    inition.
**
**  The following are the examples of C preprocessor command:
**
**	#define	CMN_HELP		FCMN0001_Help
**	#define CMN_END			FCMN0002_End
**	#define CMN_GO			FCMN0010_Go
**
**
** 1. PROCESS FLOWCHART
**
**				+-----------------+
**				|		  |
**				| Check arguments |
**				|		  |
**				+-----------------+
**					|
**					|
**					|
**	    +----+		+-----------------+
**  header  |	 |		|		  |
**  file    | .h |<-------------|    Parser	  |
**    ..... |	 |	       -|		  |
**	    +----+	   -	+-----------------+
**		       -		|
**		   -			|
**	       -			|
**	+----+	+----+			|
**	|    |	|    | - temporary	*
**	|fast|	|slow|	 file	   *  error  *------------------+
**	|    |	|    |			*			|
**	+----+	+----+		        |			|
**	       -			|			|
**		   -			|			|
**		       -		|			|
**			   -	+-----------------+		|
**			       -|		  |		|
**				|      Sort	  |		|
**			       -|		  |		|
**			   -	+-----------------+		|
**		       -		|			|
**		   -			|			|
**	       -			|			|
**	+----+	+----+			|			|
**	|    |	|    | - Sorted		|			|
**	|fast|	|slow|	 temporary file |			|
**	|    |	|    |			|			|
**	+----+	+----+		        |			|
**	       -			|			|
**		   -			|			|
**		       -		|			|
**			   -	+-----------------+		|
**			       -|		  |		|
**				|    Builder	  |		|
**			       -|		  |		|
**			   -	+-----------------+		|
**		       -		|			|
**		   -			|			|
**	       -			|			|
**	+----+	+----+			|<----------------------+
**	|    |	|    |			|
**	|fast|	|slow| - Message file	|
**	|    |	|    |			|
**	+----+	+----+		        |
**					|
**					|
**
**				    <  Exit  >

** 2. FILE FORMAT
**
** 2.1 Temporary file
**  This file is a text file created by the parser.
**
**  The file has the following format:
**
**	~Gid<newline>Name[:SQLSTATE]<newline>Message<newline>
**
**  The ":SQLSTATE" is included only for the slow temporary file.  It was
**  appended to the second line because the sort routines all assume a 3 line
**  entry format and it was easier to keep things compatible when adding
**  generic error support
**
** 2.2 Sorted temporary file
**  This file is also a text file created by the sort routine. It is possible
**  to edit it with a text editor.  Its format is the same as above except that
**  it is sorted by Gid.
**
**
** 2.3 Message file
**
** 2.3.1 Fast message file
**  This file fall into two parts. That is, There are index part and text part.
**
**  The following is the format of fast message file:
**
**	i4		number_of_class;		 -Control part-
**	struct control_r			     ---+
**	{				   	        |
**	    i4		number_of_message_in_class;     |--- up to 2040 bytes
**	    i4		length_of_whole_text_in_class;  |
**	    i4		distance_from_beginning_of_file;|
**	} ERCONTROL;				     ---+
**	    .
**	    .
**	    .
**	i4		filler;	(null)
**							 -Text part-
**	char		text[2048];		         --- 2048 * n bytes
**	    .
**	    .
**	    .
**
**
** 2.3.2 Slow message file
**  This file also fall into two parts, index part and message entry.    
**
**  The following is the format of slow message file:
**								  -Index page-
**	struct _INDEX_PAGE				      --+
**	{							|  up to
**	    i4	number_of_index_entries;		|-   2048 bytes
**	    i4	highest_index_key_corresponding_bucket	|
**	}   INDEX_PAGE;					      --+
**								  -Data page-
**	struct _MESSAGE_ENTRY				      --+
**	{							|
**	    i2		length_of_the_message_entry;		|
**	    i2		length_of_the_text_portion;		|-   * n  bytes
**	    i4		the_message_number_for_this_message;	|
**	    i2		length_of_the_name;			|
**	    char	sqlstate_status_code_for_this_error[5]	|
**	    char	message_name_and_text[1033];		|
**	}   MESSAGE_ENTRY;				      --+
**
**	Note that a data page is 2048 bytes, Messages are always a multiple of
**	4 bytes long, and messages do not span across pages.
** 
**
** 2.3.3 Header file
**  The header file is text file created by parser.
**
**  The file has the following format:
**
**	#define<tab>Name<tab>Gid<newline>
**
**
** 3. SORT
**  The ercompiler use a quick sort routine in order to sort a temporary file.
**  The temporary file is sorted in ascending order on msgid column.
**
**------------------------------------------------------------------------
**
** Inputs:
**	argc			Number of command line arguments.
**	argv			Array of pointers to the arguments.
**
** Outputs:
**	fast message file
**	slow message file
**	header files
**
** 	Returns:
**		Error status if input files do not exist.
**
** 	Exceptions:
**		None.
**
** Side Effects:
**	Fast and slow message file and header files created in the current
**	directory.
**
** History:
**	03-oct-1986
**		Create new for 5.0.
**	10-mar-1987 (peter)	Combine for unix.
**	13-mar-1987 (peter)	Allow 'W_' and 'I_' leading names for
**		different types of error messages.  Also, put in support
**		for \n and \t to force in newlines and tabs.  Also, allow
**		parentheses in message names, and check for digits after
**		the % in message strings.
**	14-aug-1987 (peter)
**		Add the -t flag.
**	08-oct-1987 (peter)
**		Allow multiple classes in a single file.
**	15-oct-1987 (bab)
**		Changed get_record to use SIgetrec.
**	22-jan-1988 (mhb)
**		Changed sort_msg function to do its own quick sort
**		instead of calling database sort
**		New functions added:
**		    er__cntids();	* Count message ids in input file *
**		    er__qsortids();	* Quick sort copied from IIugqsort *
**		    er__bldracc();	* Writes tmp racc file & msgid arr * 
**		    er__wrtids();	* Write msgs from file using array *
**		    er__cmpids();	* Compare two message ids *
**		Changed local define NAMESIZE (31) to be MAX_LOC (256)
**		for maximum file name size.
**		Made some minor changes as a result of lint check
**	03-feb-88  (mhb)
**		Added additional tmp file to write tmp file out in RACC mode
**	12-jun-89 (jrb)
**	    Added support for generic error codes.
**	12-jun-89 (jrb)
**	    Removed spurious & from in front of array name in build_slow_index.
**	    Changed & to && where it was obviously the wrong usage.
**	15-may-90 (blaise)
**	    Integrated changes from 61:
**		index_key page initialization loop should be i < ER_SMAX_KEY;
**		Remove warning on name truncation -- it's just noise
**              Added additional tmp file to write tmp file out in RACC mode
**	5/90 (bobm)
**	    Changed slow message format to prevent overflows.
**      4/91 (Mike S)
**              Added statements to put the _XX_CLASS #define in the header
**              file.  This comes from the Sapphire CL.
**	20-oct-92 (andre)
**	    instead of using generic error messages, we will be using SQLSTATE
**	    status codes
**	12-feb-93 (dianeh)
**		Removed HFSON from -q flag; -q just points to SQLSTATE file.
**      25-feb-93 (smc)
**          Forward declared get_next_rec as static & cast MEfree param as
**          a PTR.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-dec-1994 (canor01)
**	    added NO_OPTIM ris_us5 to MING hints for RS6000 AIX 4.1
**	20-Jun-1995 (walro03/mosjo01)
**	    added NO_OPTIM dg8_us5 to MING hints for DG/UX.
**	    Symptom: segmentation fault in ercompile during build.
**	    Works in DG/UX 3.00 MU05; fails in DG/UX 3.10 MU02.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	26-arp-1999 (hanch04)
**	    SIftell now returns the offset.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      03-jun-1999 (hanch04)
**          Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	02-aug-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	26-Apr-2002 (bonro01)
**	    Fix storage overlay caused by using wrong array length.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	18-Nov-2004 (gupsh01)
**	    Fixed to make sure that the correct Character attribute 
**	    table is loaded.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**	18-Feb-2008 (hanje04)
**	    SIR S119978
**	    put_record(), er__bldracc() and er__wrtids() are not externs.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Fix lots of prototypes.
*/

int
int main(i4 argc, char **argv)
{
    static char	fast[MAX_LOC + 1];
    static char	slow[MAX_LOC + 1];
    static char	sqlstate[MAX_LOC + 1];
    static char	headersuffix[MAX_LOC + 1];
    static i4	flag;
    static bool fnosort,
		snosort;
    char	*env = 0;
    char	name[MAX_LOC];
    CL_ERR_DESC	cl_err;

    MEadvise(ME_INGRES_ALLOC);

    /* set the Character attribute table */
    NMgtAt(ERx("II_INSTALLATION"), &env);
    if (env && *env)
    {
	STprintf(name, ERx("II_CHARSET%s"), env);
	NMgtAt(name, &env);
    }
    else
    {
	NMgtAt(ERx("II_CHARSET"), &env);
    }

    if(env && *env)
    {
	STcopy(env, name);
	CMset_attr(name, &cl_err);
    }
	
    check_arg(argc, argv, slow, fast, sqlstate, headersuffix, &flag);
    if ( flag & SOON )
    {
	fnosort = FALSE;
	snosort = FALSE;
    }
    else
    {
	fnosort = TRUE;
	snosort = TRUE;
    }
    SIprintf("Parsing input files ...\n");
    SIflush(stdout);
    if (parser(argc, argv, &flag, &fnosort, &snosort, headersuffix,
	     fast, slow, sqlstate) == OK )
    {
	if ( flag & FON )
	{
	    if ( !fnosort )
	    {
	        SIprintf("Sorting for fast message file ...\n");
	        SIflush(stdout);
	        sort_msg(TMPFAST, FASTTXT, flag);
	        SIprintf("Creating fast message file ...\n");
	        SIflush(stdout);
	        build_fast_index(FASTTXT, fast, flag);
	    }
	    else
	    {
	        SIprintf("No sort needed for fast message file ...\n");
	        SIprintf("Creating fast message file ...\n");
	        SIflush(stdout);
	        build_fast_index(TMPFAST, fast, flag);
	    }
	}
	if ( flag & SON )
	{
	    if ( !snosort )
	    {
	        SIprintf("Sorting for slow message file ...\n");
	        SIflush(stdout);
	        sort_msg(TMPSLOW, SLOWTXT, flag);
	        SIprintf("Creating slow message file ...\n");
	        SIflush(stdout);
	        build_slow_index(SLOWTXT, slow, flag);
	    }
	    else
	    {
	        SIprintf("No sort needed for slow message file ...\n");
	        SIprintf("Creating slow message file ...\n");
	        SIflush(stdout);
	        build_slow_index(TMPSLOW, slow, flag);
	    }
	}
    }
    else
    {
	SIprintf("No message files produced.\n");
	SIprintf("Completed with error(s)\n");
	SIflush(stdout);
    }
    PCexit(OK);
}

/*{
** Name: check_arg - Check arguments.
**
** Description:
**	This routine processes command-line arguments.
**	The following is command-line syntax:
**	
**   ercompile [-a] [-l] [-o] [-h] [-t] [-f[file]] [-s[file]] [-gfile] file...
**	
** Inputs:
**	argc			Number of command line arguments.
**	argv			Array of pointers to the arguments.
**	slow			Pointer to area of slow message file name.
**	fast			Pointer to area of fast message file name.
**	sqlstate		Pointer to SQLSTATE check file name.
**	headersuffix		Pointer to area of header file name suffix.
**	flag			Pointer to control flag.
**
** Outputs:
** 	Returns:
**	    VOID
**
** 	Exceptions:
**	    None.
**
** Side Effects:
**	Error message is writen and program terminated if error occurs.
**
** History:
**	01-Nov-1986
**		Create new for 5.0.
**	12-jun-1989 (jrb)
**	    Added `-g' flag for generic error support (integrated from VMS CL).
**	20-oct-92 (andre)
**	    removed support for -g flag and added support for -q flag which can
**	    be used to specify name of SQLSTATE check file name.
*/
static VOID
check_arg(i4 argc, char **argv, char *slow, char *fast,
	char *sqlstate, char *headersuffix, i4 *flag)
{
    i4		i, j;

    if (argc < 2)
	usage();
    /*  Initialize file name  */
    STcopy(".", headersuffix);
    STcat(headersuffix, ER_HEADSUFF);
    STcopy(ER_FASTFILE, fast);
    STcopy(ER_SLOWFILE, slow);
    STcopy(SS_FILE, sqlstate);
    i = argc;
    /*  Check arguments and set flag of compile option  */
    while (argc > 1 && **++argv == '-')
    {
	argc--;
	switch ( *(*argv + 1) )
	{
	    case 'a':
		*flag |= APON;
		continue;
	    case 'l':
		*flag |= SAON;
		continue;
	    case 'o':
		*flag |= SOON;
		continue;
	    case 'h':
		*flag |= HON;
		if ( *(*argv + 2) != EOS )
		{
		    headersuffix[0] = '.';
		    for (j = 1; (headersuffix[j] = *(*argv + 2 + j)) != EOS; j++)
		    {
			if (j == MAX_LOC)
			{
			    headersuffix[j] = EOS;
			    break;
			}
		    }
		}
		continue;
	    case 'f':
		*flag |= FON;
		if ( *(*argv + 2) != EOS )
		{
		    for (j = 0; (fast[j] = *(*argv + 2 + j)) != EOS; j++)
		    {
			if (j == MAX_LOC)
			{
			    fast[j] = EOS;
			    break;
			}
		    }
		}
		continue;
	    case 's':
		*flag |= SON;
		if ( *(*argv + 2) != EOS )
		{
		    for (j = 0; (slow[j] = *(*argv + 2 + j)) != EOS; j++)
		    {
			if (j == MAX_LOC)
			{
			    slow[j] = EOS;
			    break;
			}
		    }
		}
		continue;
	    case 'q':
		*flag |= QON;
		if ( *(*argv + 2) != EOS )
		{
		    for (j = 0; (sqlstate[j] = *(*argv + 2 + j)) != EOS; j++)
		    {
			if (j == MAX_LOC)
			{
			    sqlstate[j] = EOS;
			    break;
			}
		    }
		}
		continue;
	    case 't':
		*flag |= THFSON;
		continue;
	    default:
		SIprintf("Bad flag: %c\n", *(*argv + 1) );
		usage();
	}
    }
    if (argc == 1)
	usage();
    if (argc == i)
	*flag |= HFSON;
#ifdef xDEBUG
    SIprintf("headersuffix %s\n", headersuffix);
    SIprintf("Fast %s\n", fast);
    SIprintf("Slow %s\n", slow);
    SIprintf("SQLSTATE %s\n", sqlstate);
#endif
    return;
}

static VOID
usage(void)
{
    SIprintf("Usage: ercompile [-a][-l][-o][-h][-t][-f[file]][-s[file]][-gfile] classfiles\n");
    SIprintf("    -a       - Append the new messsages to temporary files.\n");
    SIprintf("    -l       - Save the temporary files.\n");
    SIprintf("    -o       - Sort the temporary files.\n");
    SIprintf("    -h       - Create the header files.\n");
    SIprintf("    -t       - Create message files for testing.\n");
    SIprintf("    -f[file] - Create the named fast message file.  (default: %s)\n", ER_FASTFILE);
    SIprintf("    -s[file] - Create the named slow message file.  (default: %s)\n", ER_SLOWFILE);
    SIprintf("    -qfile   - Check SQLSTATE status codes with this file. (default: sqlstate.h)\n");
    SIflush(stdout);
    PCexit (FAIL);
}

/*{
** Name: parser - Parser for the ercompile.
**
** Description:
**	This routine checks syntax. If no error, creates temporary files and
**	header files. 
**	
** Inputs:
**	argc			Number of command line arguments.
**	argv			Array of pointers to the argments.
**	flag			Option of compile.
**	fnosort			Set to TRUE if no sort required on fast file.
**	snosort			Set to TRUE if no sort required on slow file.
**	headersuffix		Pointer to area of header file name suffix.
**	fastfile		pointer to name of fast message file.
**	slowfile		pointer to name of slow message file.
**	ssfile			pointer to name of sqlstate check file.
**
** Outputs:
**	tmp.f			Temporary file for fast message file.
**	tmp.s			Temporary file for slow message file.
** 	Returns:
**	    OK			If error does not occur.
**	    FAIL		If error occurs.
**
** 	Exceptions:
**	    None.
**
** Side Effects:
**	    Error message is written. If fatal error occurs, program is
**	    terminated.
**
** History:
**	01-Nov-1986
**		Create new for 5.0.
**	12-jun-89 (jrb)
**	    Added support for generic error check file and new syntax for
**	    generic error mapping support (integrated from VMS CL).
**	4/91 (Mike S)
**          Added statement to add the _XX_CLASS #define to the header
**          file.
**	20-oct-92 (andre)
**	    replaced support for generic errors with support for SQLSTATEs
**	01-nov-92 (andre)
**	    if a file containing SQLSTATE_MAPPING_REQUIRED string consists of
**	    both SLOW and FAST messages, we will insist that SLOW messages be
**	    mapped and that FAST messages not be mapped; if the string is not
**	    present, neither SLOW nor FAST messages will be expected to be
**	    mapped
**	12-nov-92 (andre)
**	    Added support for NO_SQLSTATE_MAPPING directive.  It will
**	    allow FEs to concatenate message files some of which allow SQLSTATE
**	    mapping
**	27-dec-2004 (gupsh01)
**	    Fixed ercomp.c to correctly parse the back slash followed by a 
**	    double byte character.
*/
static STATUS
parser(i4 argc, char **argv, i4 *flag,
	bool *fnosort, bool *snosort, char *headersuffix, char *fastfile,
	char *slowfile, char *ssfile)
{
    char	buf[MAXLINE+1];
    char	token[MAX_MSG_LINE + 1];
    char	mes[MAX_MSG_LINE + 1];
    char	name[ER_MAX_NAME + 1];
    char	*start_char;
    char	*next_char;
    char	*msg;
    char	header[MAX_LOC];
    char	*deflit = "#define";
    char	class[4];
    char	sgid[11];
    i4		classno;
    i4		number;
    char	ss_code[5];
    i4		a, b, c, gid;
    i4		m_class,
		m_fast,
		m_slow;
    i4		line_number;
    i4		err = 0;
    i4		f_cnt = 0;
    i4		s_cnt = 0;
    i4		c_cnt;
    i4		cont_flag;
    i4		i;
    LOCATION	f_loc,
		s_loc,
		h_loc,
		c_loc;
    FILE	*f_fp,
		*s_fp,
		*h_fp,
		*c_fp;
    char	dev[80];
    char	path[80];
    char	fpre[80];
    char	fsuf[80];
    char	ver[5];

    m_fast = 0;
    m_slow = 0;
    m_class = 0;
    while (**++argv == '-')
	argc--;
    if (*flag & FON)
    {
	LOfroms(PATH & FILENAME, TMPFAST, &f_loc);
	if ( *flag & APON )
	{
	    if ( SIopen(&f_loc, "a", &f_fp) != OK )
	    {
	        SIprintf("Error - Can't open temporary file 'TMPFAST'.\n");
	        PCexit(FAIL);
	    }
	}
	else
	{
	    if ( SIopen(&f_loc, "w", &f_fp) != OK )
	    {
	        SIprintf("Error - Can't open temporary file 'TMPFAST'.\n");
	        PCexit(FAIL);
	    }
	}
    }
    if (*flag & SON)
    {
	LOfroms(PATH & FILENAME, TMPSLOW, &s_loc);
	if ( *flag & APON )
	{
	    if ( SIopen(&s_loc, "a", &s_fp) != OK )
	    {
	        SIprintf("Error - Can't open temporary file 'TMPSLOW'.\n");
	        PCexit(FAIL);
	    }
	}
	else
	{
	    if ( SIopen(&s_loc, "w", &s_fp) != OK )
	    {
	        SIprintf("Error - Can't open temporary file 'TMPSLOW'.\n");
	        PCexit(FAIL);
	    }
	}
    }

    /*			   */
    /*  Main process loop  */
    /*			   */
    while (argc-- > 1)
    {
	line_number = 0;
	/*  Open class file  */
	SIprintf("Reading File '%s' ...\n",*argv);
	SIflush(stdout);
	if ( LOfroms(PATH & FILENAME, *argv, &c_loc) != OK )
	{
	    SIprintf("Error - Can't open class file '%s'.\n", *argv);
	    PCexit(FAIL);
	}
	if ( SIopen(&c_loc, "r", &c_fp) != OK )
	{
	    SIprintf("Error - Can't open class file '%s'.\n", *argv);
	    PCexit(FAIL);
	}
	/*  Create header file name  */
	if ( (!err) && (*flag & HON) )
	{
	    LOdetail(&c_loc, dev, path, fpre, fsuf, ver);
# ifdef UNIX
	    STprintf(header, "%s%s%s",path,fpre,headersuffix);
# else
	    STcopy(fpre, header);
	    STcat(header, headersuffix);
# endif
	    /*  Open header file  */
	    if ( LOfroms(PATH & FILENAME, header, &h_loc) != OK )
	    {
		SIprintf("Error - Can't open header file '%s'.\n", header);
		PCexit(FAIL);
	    }
	    if ( SIopen(&h_loc, "w", &h_fp) != OK )
	    {
		SIprintf("Error - Can't open header file '%s'.\n", header);
		PCexit(FAIL);
	    }
	}

	/* SQLSTATEs aren't required for this file unless specified later */
	SS_map_req = FALSE;
	
	/* begin reading the file */
	next_char = buf;
	*next_char = '\n';
	start_char = next_char;
	if (take(buf, start_char, &next_char, token, &line_number, argv, c_fp) != OK)
	{
	    SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
	    PCexit(FAIL);
	}
	if (STcompare(token, SSREQSTRING) == 0)
	{
#ifdef xDEBUG
	    SIprintf("SQLSTATE mappings will be required for this file.\n");
#endif
	    SS_map_req = TRUE;
	    build_ss_list(ssfile);

	    start_char = next_char;
	    if (take(buf, start_char, &next_char, token, &line_number, argv, c_fp) != OK)
	    {
		SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
		PCexit(FAIL);
	    }
	}
	if (token[0] != '#')
	{
	    SIprintf("Syntax error - Must have class define clause before error entries\n");
	    SIprintf("\tAt line %d in file '%s'\n", line_number, *argv);
	    PCexit(FAIL);
	}
	/*  Search 'define'  */
	start_char = next_char;
	if (take(buf, start_char, &next_char, token, &line_number, argv, c_fp) != OK)
	{
	    SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
	    PCexit(FAIL);
	}
	if ( STcompare(token, "define") != 0)
	{
	    SIprintf("Syntax error - Must have class define clause before error entries\n");
	    SIprintf("\tAt line %d in file '%s'\n", line_number, *argv);
	    PCexit(FAIL);
	}
	/*  Search 'class name'  */
	start_char = next_char;
	if (take(buf, start_char, &next_char, token, &line_number, argv, c_fp) != OK)
	{
	    SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
	    PCexit(FAIL);
	}
	if ( STcompare(&token[3], "_CLASS") != 0 && STcompare(&token[3], "_class") != 0)
	{
	    SIprintf("Syntax error - Must have class define clause before error entries\n");
	    SIprintf("\tAt line %d in file '%s'\n", line_number, *argv);
	    PCexit(FAIL);
	}
	for (i = 0; i < 3; i++)
	    class[i] = token[i];
	class[3] = EOS;
	/*  Get class number  */
	start_char = next_char;
	if (take(buf, start_char, &next_char, token, &line_number, argv, c_fp) != OK)
	{
	    SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
	    PCexit(FAIL);
	}
	for (i = 0; token[i] != EOS; i++)
	{
	    if ( !CMdigit(&token[i]) )
	    {
		SIprintf("Syntax error - Invalid class number.\n\tAt line %d in file '%s'\n", line_number, *argv);
		PCexit(FAIL);
	    }
	}
	CVal(token, &classno);
	if (classno >= CLASS_SIZE)
	{
	    SIprintf("Syntax error - Too big class number\n\tAt line %d in file '%s'\n", line_number, *argv);
	    PCexit(FAIL);
	}

	/*
	**	If the -t option is set, use the class number to construct
	**	a message file name for the fast and slow files.
	*/

	if (*flag & TON)
	{	/* Flag is set */
		STcopy(ERx("f"),fastfile);
		STcat(fastfile, token);
		STcat(fastfile, ERx(".mnx"));

		STcopy(ERx("s"),slowfile);
		STcat(slowfile, token);
		STcat(slowfile, ERx(".mnx"));
	}

	if (classno < m_class)
	{
	    *fnosort = FALSE;
	    *snosort = FALSE;
	}
	m_class = classno;
	m_fast = 0;
	m_slow = 0;
#ifdef xDEBUG
	SIprintf("Class name = %s\n", class);
	SIprintf("Class number = %d\n", classno);
#endif
        /*
        ** Output the _XX_CLASS define to the .h file.
        */
        if ( (!err) && (*flag & HON) )
            SIfprintf(h_fp, "#define %s_CLASS %d\n", class, classno);

	/* Get message or C preprocessor statment */
	start_char = next_char;
	while (take(buf, start_char, &next_char, token, &line_number, argv, c_fp) == OK)
	{
#ifdef xDEBUG
	SIprintf("Token = %s\n", token);
#endif
	    if ( token[0] != 'F' && token[0] != 'S' && token[0] != 'E' 
		    && token[0] != 'I' &&token[0] != 'W' && token[0] != '#' 
		     && (token[0] != 'N' || STcompare(token, NOSS_STRING))) 
	    {
		SIprintf("Token -%s- starts with unexpected char %c=%d at line %d in file %s\n",
		    token,token[0],token[0], line_number, *argv);
		err = get_next_rec(buf, &next_char, &line_number, argv, c_fp);
		goto errorexit;
	    }
	    /*				  */
	    /*  Copy text to header file  */
	    /*				  */
	    if ( token[0] == '#' )
	    {
		/*  Search 'define'  */
		start_char = next_char;
		if (take(buf, start_char, &next_char, 
			token, &line_number, argv, c_fp) != OK)
		{
	    	    SIprintf("Error reading file '%s'.\n\tOn line %d.\n", 
			*argv, line_number);
	    	    PCexit(FAIL);
		}
		if ( STcompare(token, "define") != 0)
		{
	    	    SIprintf("Syntax error - no define clause\n\tAt line %d in file '%s'\n", 
			line_number, *argv);
	    	    PCexit(FAIL);
		}
		/*  Search 'class name'  */
		start_char = next_char;
		if (take(buf, start_char, &next_char, token, 
			&line_number, argv, c_fp) != OK)
		{
	    	    SIprintf("Error reading file '%s'.\n\tOn line %d.\n", 
			*argv, line_number);
	    	    PCexit(FAIL);
		}
		if ( STcompare(&token[3], "_CLASS") != 0 
			&& STcompare(&token[3], "_class") != 0)
		{	/* Not a class define statement */
		    STpolycat((i4)2,"# define\t", token, mes);
		    for(i=0; mes[i]!=EOS; i++)
			;
		    for (;;)
		    {
		        while ( *next_char != '\n' )
			    mes[i++] = *next_char++;
		        mes[i] = EOS;
		        if ( (!err) && (*flag & HON) )
			    SIfprintf(h_fp, "%s\n", mes);
		        if ( *(next_char - 1) == '\\' )
		        {
			    if ( SIgetrec(buf, MAXLINE, c_fp) != OK)
			    {
			        SIprintf("Error - Unexpected end of file '%s'.\n\tOn line %d.\n", *argv, line_number);
			        PCexit(FAIL);
			    }
			    line_number++;
			    next_char = buf;
			    i = 0;
		        }
		        else
			    break;
		    }
		}
		else
		{	/* A class define statement has been found.
			** switch to the new class number */
		    if (*flag & TON)
		    {	/* -t flag is set.  multiple message classes no good */
			SIprintf("Error - multiple message class defines found with -t flag.\n\tAt line %d\n", line_number);
			PCexit(FAIL);
		    }
		    for (i = 0; i < 3; i++)
		    {
	    	    	class[i] = token[i];
		    }
		    class[3] = EOS;
		    /*  Get class number  */
		    start_char = next_char;
		    if (take(buf, start_char, &next_char, token, 
			&line_number, argv, c_fp) != OK)
		    {
	    	        SIprintf("Error reading file '%s'.\n\tOn line %d.\n", 
				*argv, line_number);
	    	    	PCexit(FAIL);
		    }
		    for (i = 0; token[i] != EOS; i++)
		    {
	    	        if ( !CMdigit(&token[i]) )
	    	        {
			    SIprintf("Syntax error - Invalid class number.\n\tAt line %d in file '%s'\n", line_number, *argv);
			    PCexit(FAIL);
	    	        }
		    }
		    CVal(token, &classno);
		    if (classno >= CLASS_SIZE)
		    {
	    	        SIprintf("Syntax error - Too big class number\n\tAt line %d in file '%s'\n", line_number, *argv);
	    	        PCexit(FAIL);
		    }
		    /* Check to see if sorts are needed */
		    if (classno < m_class)
		    {
	    	        *fnosort = FALSE;
	    	        *snosort = FALSE;
		    }
		    m_class = classno;
		    m_fast = 0;
		    m_slow = 0;
		}
	    }
	    else if (STcompare(token, SSREQSTRING) == 0)
	    {
		if (!SS_map_req)
		{
#ifdef xDEBUG
		    SIprintf("SQLSTATE mappings will be required for the next segment.\n");
#endif
		    SS_map_req = TRUE;
		    build_ss_list(ssfile);
		}
	    }
	    else if (STcompare(token, NOSS_STRING) == 0)
	    {
		if (SS_map_req)
		{
#ifdef xDEBUG
		    SIprintf("SQLSTATE mappings will be disallowed for the next segment.\n");
#endif
		    SS_map_req = FALSE;
		}
	    }
	    /*			     */
	    /* Process for message   */
	    /*			     */
	    else
	    {
		/*  Check class name  */
		if ( STncmp( class, &token[1], 3 ) != 0 )
		{
		    SIprintf("Syntax error - Invalid class name\n\tAt line %d in file '%s'\n", line_number, *argv);
		    err = get_next_rec(buf, &next_char, &line_number, argv, c_fp);
		    goto errorexit;
		}
		/*  Check message number  */
		number = 0;
		for (i = 4; i < 8; i++)
		{
		    if (token[i] >= '0' && token[i] <= '9')
			number = (number << 4) + token[i] - '0'; 
		    else if (token[i] >= 'a' && token[i] <= 'f')
			number = (number << 4) + token[i] - 'a' + 10; 
		    else if (token[i] >= 'A' && token[i] <= 'F')
			number = (number << 4) + token[i] - 'A' + 10; 
		    else
		    {
			SIprintf("Syntax error - Invalid message number\n");
			SIprintf("\tAt line %d in file '%s'\n", line_number, *argv);
			err = get_next_rec(buf, &next_char, &line_number, argv, c_fp);
			goto errorexit;
		    }
		}
#ifdef xDEBUG
		SIprintf("Error number = %d, 0x%x\n", number, number);
#endif
		/*  Generate GID  */
		a = 0;
		b = classno;
		c = number;
		if ( token[0] == 'F' )
		{
		    a = ONFLAGBIT;
		}
		b <<= 16;
		gid = a|b|c;
		if (token[0] == 'F')
		{
		    if (number < m_fast)
		    {
			*fnosort = FALSE;
		    }
		    m_fast = number;
		}
		else
		{
		    if (number < m_slow)
		    {
			*snosort = FALSE;
		    }
		    m_slow = number;
		}
		STprintf(sgid, "0x%08lx", gid);
#ifdef xDEBUG
		SIprintf("GID = %s\n", sgid);
#endif
# ifdef DONT_WANT_THIS_USELESS_WARNING	/* i.e. never */
                /*
                **  We will always have message names that are longer,
                **  and we can't change ER_MAX_NAME, so this is just
                **  frustrating to see all the time. (daveb)
                */
		if ( STlength(token) > ER_MAX_NAME )
		    SIprintf("Warning - Message name truncated\n\tAt line %d in file '%s'\n", line_number, *argv);
# endif
		STncpy(name, token, ER_MAX_NAME);
		name[ ER_MAX_NAME ] = EOS;

		/*					    */
		/* Check for possible SQLSTATE status cope  */
		/* mapping				    */
		/*					    */
		start_char = next_char;
		if ( take(buf, start_char, &next_char, token, &line_number, argv, c_fp) != OK )
		{
		    SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
		    PCexit(FAIL);
		}
		/* if the next token is a colon, then we have an SQLSTATE mapped here */
		if (token[0] == ':')
		{
		    char	*s;
		    
		    if (SS_map_req == FALSE)
		    {
			SIprintf("Error - You may not have SQLSTATE mappings unless you specify\n");
			SIprintf("\"%s\" as the first line of your MSG file.\n", SSREQSTRING);
			SIprintf("\tAt line %d in file '%s'\n", line_number,
			    *argv);
			PCexit(FAIL);
		    }

		    start_char = next_char;
		    if (take(buf, start_char, &next_char, token, &line_number, argv, c_fp) != OK )
		    {
			SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
			PCexit(FAIL);
		    }

		    /* we have an SQLSTATE status code -- validate it */
		    if (STncmp(token, "SS", 2) != 0)
		    {
			SIprintf("Error - Invalid sqlstate '%s'.\n",token);
			SIprintf("All SQLSTATE status codes must begin with 'SS'.\n");
			SIprintf("\tAt line %d in file '%s'\n", line_number,
			    *argv);
			PCexit(FAIL);
		    }

		    for (i = 2, s = ss_code; i < 7; i++, s++)
		    {
			/* SQLSTATE "number" may consist of [A-Z0-9] */
			
			*s = token[i];
			if (   (*s < '0' || *s > '9')
			    && (*s < 'A' || *s > 'Z')
			   )
			{
			    SIprintf("Syntax error - Invalid SQLSTATE number\n");
			    SIprintf("\tAt line %d in file '%s'\n", line_number,
				*argv);
			    err = get_next_rec(buf, &next_char, &line_number,
				argv, c_fp);
			    goto errorexit;
			}
		    }
		    
		    /* now verify that this is one of defined SQLSTATEs */
		    if (validate_sqlstate(token+2) != OK)
		    {
			SIprintf("Error - Invalid SQLSTATE status code '%s'.\n", token);
			SIprintf("Not Found in file '%s'\n", ssfile);
			SIprintf("\tAt line %d in file '%s'\n", line_number,
			    *argv);
			PCexit(FAIL);
		    }

#ifdef xDEBUG
		    SIprintf("SQLSTATE status code = %5s\n", ss_code);
#endif

		    /*
		    ** FAST messages in files where SLOW messages are required
		    ** to be mapped into SQLSTATEs are still expected to remain
		    ** unmapped
		    */
		    if ( (*flag & FON) && (name[0] == 'F') )
		    {
			SIprintf("Warning - fast message was mapped into \
SQLSTATE on line %d in file '%s'\n", line_number, *argv);
			SIprintf("SQLSTATE value will be discarded.\n");
		    }

		    /*
		    ** read in the next token for the message text checking code
		    ** which follows
		    */ 
		    start_char = next_char;
		    if ( take(buf, start_char, &next_char, token, &line_number, argv, c_fp) != OK )
		    {
			SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
			PCexit(FAIL);
		    }
		}
		else if (   (*flag & SON)
			 && (   name[0] == 'S' || name[0] == 'E'
			     || name[0] == 'I' || name[0] == 'W')
			)
		{
		    char	*s1, *s2;

		    /*
		    ** even if the file contains SQLSTATE_MAPPING_REQUIRED,
		    ** FAST messages are not expected to be mapped to SQLSTATEs
		    */
		    
		    if (SS_map_req == TRUE)
		    {
			SIprintf("Error - Since \"%s\" was specified at the top of this file,\n", SSREQSTRING);
			SIprintf("every error entry in this file must have an SQLSTATE mapping.\n");
			SIprintf("\tAt line %d in file '%s'\n", line_number, *argv);
			PCexit(FAIL);
		    }
		    
		    for (s1 = ss_code, s2 = MISC_ERR_SS, i = 0; i < 5; i++)
			*s1++ = *s2++;
		}


		/*		      */
		/* Get message text   */
		/*		      */
	
		/* Next token has already been read above */
		if ( token[0] != '"' )
		{
		    SIprintf("Syntax error - Invalid message\n\tAt line %d in file '%s'\n", line_number, *argv);
		    err = get_next_rec(buf, &next_char, &line_number, argv, c_fp);
		    goto errorexit;
		}
		cont_flag = 1;
		c_cnt = 0;
		msg = mes;
		while ( cont_flag && (c_cnt < MAX_MSG_LINE + 1) )
		{
		  if (CMdbl1st(next_char))
		  {
		      int bcnt = 0;

		      bcnt = CMbytecnt(next_char);
		      CMcpychar (next_char, msg);
		      msg += bcnt;
		      next_char +=  bcnt;
		      c_cnt += bcnt;
		  }
		  else
		  {
		    switch (*next_char++)
		    {
			case '"':
			    cont_flag = 0;
			    *msg = EOS;
			    break;
			case '\\':
			{
			    switch (*next_char)
			    {
				case '"': case '\\': case '%':
				    *msg++ = *(next_char - 1);
				    *msg++ = *next_char++;
				    c_cnt += 2;
				    continue;
				case '\n':
				    if ( SIgetrec(buf, MAXLINE, c_fp) == OK )
				    {
					line_number++;
					next_char = buf;
					continue;
				    }
				    else
				    {
					SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
					PCexit(FAIL);
				    }
				default:
				    *msg++ = *(next_char - 1);
				    c_cnt++;
				    continue;
			    }
			}
			case '\n':
			    /* On a newline, simply put in a space */
			    *msg++ = ' ';
			    c_cnt++;
			    if ( SIgetrec(buf, MAXLINE, c_fp) == OK )
			    {
				line_number++;
				next_char = buf;
				continue;
			    }
			    else
			    {
				SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *argv, line_number);
				PCexit(FAIL);
			    }
			case '\t':
			    /* On tab character, put in a tab */
			    *msg++ = '\t';
			    c_cnt++;
			    continue;
/*			case '%'
** If a parameter is given, it must be followed
** by a digit.
**
**			    if(!CMdigit(next_char) && !(*next_char=='!'))
**			    {
**				SIprintf("Syntax error in file '%s'.\n\tOn \
** line %d.\n\tParameter substitution must have sequence number.\n",
**				    *argv, line_number);
**				PCexit(FAIL);
**			    }
**			    *msg++ = '%';
**			    c_cnt++;
**			    continue;
*/
			default :
			    *msg++ = *(next_char - 1);
			    c_cnt++;
			    continue;
		    }   /* endswitch */
		  }	/* end else */
		}	/* endwhile get message strings end */
		if (c_cnt > MAX_MSG_LINE)
		{
		    mes[MAX_MSG_LINE] = EOS;
		    SIprintf("Warning - Message text truncated\n\tAt line %d in file '%s'\n", line_number, *argv);
		}
		/* Got null string */
		if (c_cnt == 0)
		    STcopy("\\0", mes);
		if (!err)
		{
		    /* Write header record */
		    if (*flag & HON)
		    {
			SIfprintf(h_fp, "%s %-30s %s\n", deflit, name, sgid);
		    }

		    /* Write fast and/or slow record */
		    if ( (*flag & FON) && (name[0] == 'F') )
		    {
			SIfprintf(f_fp, "~%s\n%s\n%s\n", sgid+2, name, mes);
			f_cnt++;
		    }
		    else if ( (*flag & SON) && 
			(name[0] == 'S' || name[0] == 'E' || name[0] == 'I'
				        || name[0] == 'W') )
		    {
			SIfprintf(s_fp,
				    "~%s\n%s:%5s\n%s\n",
				    sgid+2, 
				    name,
				    ss_code, 
				    mes);
			s_cnt++;
		    }
#ifdef xDEBUG
		    SIprintf("Header file = %s %s %s\n", deflit, name, sgid);
		    SIprintf("Temporary file = %s %s %s\n", sgid, name, mes);
#endif
		}
	    }   /* endif : end of process for message */
errorexit:
	    start_char = next_char;
	}	/* endwhile : end of a class file loop */
	SIclose(c_fp);	/*  Close class file  */
	if ( (!err) && (*flag & HON) )
	    SIclose(h_fp);	/* Close header file */
	++argv;
    }	/* endwhile : end of all class files loop */
    /*				   */
    /*  End of main process loop   */
    /*				   */
    if (*flag & FON)
	SIclose(f_fp);
    if (*flag & SON)
	SIclose(s_fp);
    if ( (*flag & FON) && (f_cnt < 1) )
    {
        *flag &= FOFF;
	if ( !((*flag & APON) && (*flag & SAON)) )
	{
            LOdelete(&f_loc);
	}
    }
    if ( (*flag & SON) && (s_cnt < 1) )
    {
        *flag &= SOFF;
	if ( !((*flag & APON) && (*flag & SAON)) )
	{
            LOdelete(&s_loc);
	}
    }
    if (err)
	return(FAIL);
    else
	return(OK);
}


/*{
** Name: build_ss_list - Build list of valid SQLSTATE status codes.
**
** Description:
**	This routine builds a list of SQLSTATE status codes so that we can
**	validate SQLSTATE mappings when compiling message files
**	containing them.  The format of a SQLSTATE string is:
**
**	    SSnnnnn_...    where 'nnnnn' is a five character string consisting
**			   of [A-Z0-9]
**
**	we don't store the SS since it will be redundant for every SQLSTATE
**	and would therefore slow down the validation.  The list is
**	sorted after being built so we can use a binary search later on.
**	
** Inputs:
**	fn	    - filename to get list from
**
** Outputs:
**	none
**
** 	Returns:
**		Nothing.
**
** 	Exceptions:
**		None.
**
** Side Effects:
**	Fills in the SS_list global and the SS_codes global
**
** History:
**	21-oct-92 (andre)
**	    plagiarized from jrb's build_ge_list.
*/
static VOID
build_ss_list(char *fn)
{
    LOCATION	    ss_loc;
    FILE	    *ss_fp;
    char	    buf[MAXLINE+1];
    char	    token[100];
    char	    *start;
    char	    *next;
    i4		    line = 0;
    i4		    i = 0;

    /* if we've already done this, then let's save some time */
    if (SS_codes != 0)
	return;

    LOfroms(PATH & FILENAME, fn, &ss_loc);
    if (SIopen(&ss_loc, "r", &ss_fp) != OK)
    {
	SIprintf("Error - Can't open SQLSTATE file '%s'.\n", fn);
	PCexit(FAIL);
    }

    *(start = next = buf) = '\n';
    while (take(buf, start, &next, token, &line, &fn, ss_fp) == OK)
    {
	/*
	** format of SQLSTATE status code name is SSnnnnn while constants
	** describing CLASS, SUBCLASS, or GE_MAP values start with SS_
	*/
	if (   STncmp(token, "SS", 2 ) == 0
	    && STncmp(token, "SS_", 3 ) != 0)
	{
	    if (i >= SS_MAX_CODES)
	    {
		SIprintf("Error -- Too many SQLSTATE status codes found in file '%s'.\n", fn); 
		SIprintf("Currently a maximum of %d SQLSTATE status codes can be handled by\n", SS_MAX_CODES);
		SIprintf("the ERcompiler.  To fix this, increase SS_MAX_CODES in the ERcompiler\n");
		SIprintf("source and recompile it.\n");
		PCexit(FAIL);
	    }
	    STcopy(token+2, SS_list[i++]);
	}

	start = next;
    }
    SS_codes = i;

    /* sort them by SQLSTATE status code */
    er__qsortids((char *) SS_list, SS_codes, SS_MAX_LEN, STcompare);
    
#ifdef xDEBUG
    SIprintf("The %d SQLSTATE status codes read were:\n", SS_codes);
    for (i = 0; i < SS_codes; i++)
	SIprintf("%s\n", SS_list[i]);
#endif

    SIclose(ss_fp);
}


/*{
** Name: validate_sqlstate - Validate an SQLSTATE status code
**
** Description:
**	This routine uses a binary search to look up an SQLSTATE status code to
**	validate mappings in the user's error file.
**
** Inputs:
**	ss_code	    - The SQLSTATE status code minus the "SS" part
**
** Outputs:
**	none.
**
**	Returns:
**	    OK	    - The code is valid
**	    FAIL    - The code is invalid
**
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	21-oct-92 (andre)
**	    plaigiarised from jrb's validate_generr
*/
static STATUS
validate_sqlstate(char *ss_code)
{
    i4	    s;
    i4	    left = 0;
    i4	    right = SS_codes - 1;
    i4	    comp;

    do
    {
	s = (left + right) / 2;

	if ((comp = STcompare(ss_code, SS_list[s])) < 0)
	    right = s-1;
	else
	    left = s+1;
    } while (comp != 0  &&  left <= right);

    if (comp == 0)
	return(OK);
    else
	return(FAIL);
}


/*{
** Name: take - Get next token.
**
** Description:
**	Scan off the next token from input stream.
**	'Wordbuf' is filled in extracted character. 
**	The return value is OK or FAIL.
**	
** Inputs:
**	buf			Input buffer.
**	start_char		Beginning position in buffer.
**	end_char		Pointer of character pointer to ending position in buffer.
**	wordbuf			Token is filled in this array.
**	ln			Pointer to Line number.
**	fn			Pointer to file name.
**	fp			Pointer to input file.
**
** Outputs:
** 	Returns:
**	    OK			if token is found.
**	    FAIL		if token does not exist.	
**
** 	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	01-Nov-1986
**	    Create new for 5.0.
**	14-mar-1987 (peter)
**	    Allow parentheses in tokens.
**	27-dec-2004 (gupsh01)
**	    Add support to handle double byte characters.
**	    correctly.
*/
static STATUS
take(char *buf, char *start_char, char **end_char, char *wordbuf,
	i4 *ln, char **fn, FILE *fp)
{
    char	*scan(); 
    char	*next_char;
    char	*word;
    i4		notfound;
    i4		count;
    if ((next_char = scan(buf, start_char, ln, fn, fp)) != NULL)
    {
        word = wordbuf;
        notfound = 1;
        count = 1;
        while (notfound)
        {
	  if (CMdbl1st (next_char))
	  {
	    int bcnt = 0;
	    bcnt = CMbytecnt (next_char);
	    CMcpychar (next_char, word);
	    word += bcnt;
	    count += bcnt;
	    next_char += bcnt;
	    continue;
	  }
	  else 
	  {
	    switch (*next_char++)
	    {
	        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':	
	        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': 
	        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': 
	        case 's': case 't': case 'u': case 'v': case 'w': case 'x': 
	        case 'y': case 'z': case 'A': case 'B': case 'C': case 'D': 
	        case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': 
	        case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': 
	        case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': 
	        case 'W': case 'X': case 'Y': case 'Z': case '1': case '2': 
	        case '3': case '4': case '5': case '6': case '7': case '8': 
	        case '9': case '0': case '_':
		    *word++ = *(next_char - 1);
		    count++;
		    continue;
	        case '!':  case '"': case '#': case '$': case '%': case '&':
		case '\'': case '*': case '+': case ',': case '(': case ')':
		case '-':  case '.': case '/': case ':': case ';': case '<': 
		case '=':  case '>': case '?': case '@': case '[': case ']': 
		case '^':  case '`': case '{': case '|': case '}': case '~': 
		case '\\':
		    if (count == 1)
		    {
		        *word++ = *(next_char - 1);
		    }
		    else
		    {
			next_char--;
		    }
		    notfound = 0;		
		    *word = EOS;
		    break;
	        case ' ': case '\t': case '\v': case '\b': case '\r': case '\f':
		case '\0':
		    next_char--;
		    notfound = 0;
		    *word = EOS;
		    break;
		default :
		    notfound = 0;	/* control character */
		    *word = EOS;
		    break;
	    }
	  }
        }
#ifdef xDEBUG
	SIprintf("Take word = %s\n", wordbuf);
#endif
	*end_char = next_char;
        return(OK);
    }
    else
	return(FAIL);
}	

/*{
** Name: scan - get next character.
**
** Description:
**	Search next character and return pointer to next character.
**	Skip sentence of comment.
**
** Inputs:
**	buf			Input buffer.
**	stp			Beginning position in buffer.
**	ln			Pointer to line number.
**	fn			Pointer to file name.
**	fp			pointer to input file. 
**
** Outputs:
** 	Returns:
**	    Character pointer to next character.
**
** 	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**	
** History:
**	01-Nov-1986
**		Create new for 5.0.
**	27-dec-2004 (gupsh01)
**	    Add support to handle double byte characters.
**	    correctly.
*/
static char *
scan(char *buf, char *stp, i4 *ln, char **fn, FILE *fp)
{
    i4		tokenfound;
    i4		filestatus;
    i4          commentend;
    char	*next_char;
    tokenfound = 0;
    filestatus = 0;
    next_char = stp;
    while (tokenfound == 0 && filestatus != END)
    {
#ifdef xDEBUG
	SIprintf("Scan char = %c\n", *next_char);
#endif
	if (CMdbl1st (next_char))
	{
	    tokenfound = 1;
	}
	else
	{
    	  switch (*next_char)
	  {
	    case ' ':
	    case '\t':
	    case '\f':
	    {
		next_char++;
		continue;
	    }
	    case '\n':
	    case '\0':
	    {
		if ( SIgetrec(buf, MAXLINE, fp) == OK )
		{
		    *ln += 1;
		    next_char = buf;
		    continue;
		}
		else
		{
		    /* End of file */
		    tokenfound = 2;
		    filestatus = END;
		    break;
		}
	    }
	    case '/':
	    {
		if ( *(next_char + 1) == '*' )
		{
		    commentend = 0;
		    next_char += 2;
		    while (commentend == 0 && filestatus != END)
		    {
			switch (*next_char++)
			{
			    case '*': 
			    {
				if (*(next_char) == '/')
				{
				    commentend = 1;
				    break;
				}
				else
				{
				    continue;
				}
			    }
			    case '\n':
			    case '\0':
			    {
				if ( SIgetrec(buf, MAXLINE, fp) == OK )
				{
				    *ln += 1;
				    next_char = buf;
				    continue;
				}
				else
				{
				    SIprintf("Syntax error - Invalid comment\n\tAt line %d in file '%s'", *ln, *fn);
				    tokenfound = 3;
				    filestatus = END;
				    break;
				}
			    }
			    default :
				continue;
			}
		    }
		    next_char++;
		    continue;
		}
		else
		{
		    tokenfound = 1;
		    break;
		}
	    }
	    default:
	    {
		tokenfound = 1;
		break;
	    }
	  }
	} /* else */
    }
    if (tokenfound == 1)
       	return(next_char);
    else
	return(NULL);
}

/*{
** Name: get_next_rec - Get next record from input file and return TRUE.
**
** Description:
**	Read next record from the input file and return value TRUE.
**	
** Inputs:
**	buf			Input buffer.
**	next_char		Pointer of character pointer to input buffer.
**	ln			Pointer to line number.
**	fn			Character pinter to file name.
**	fp			File pointer to input file.
**
** Outputs:
** 	Returns:
**	        TRUE.	
**
** 	Exceptions:
**		None.
**
** Side Effects:
**	Error message is written and program exitted if error occurs.
**	
** History:
**	01-Nov-1986
**		Create new for 5.0.
*/
static bool
get_next_rec(char *buf, char **next_char, i4 *ln, char **fn, FILE *fp)
{
    if ( SIgetrec(buf, MAXLINE, fp) != OK )
    {
	SIprintf("Error reading file '%s'.\n\tOn line %d.\n", *fn, *ln);
	PCexit(FAIL);
    }
    *ln += 1;
    *next_char = buf;
    return(TRUE);
}

/*
**  Local defines for the build procedures.
*/
/*  VMS abort and success codes for programs. */
#define	CLASSMASK1	0x0fff0000
#define	CLASSMASK2	0x0fff
#define	MESSMASK	0xffff
#define BS		0x08
#define HT		0x09
#define NL		0x0a
#define VT		0x0b
#define FF		0x0c
#define CR		0x0d
#ifdef  VMS
#define	WRITE_SIZE	4096
#endif

/*}
** Name: FILE_CONTROL - Information needed to load files.
**
** Description:
**      This structure is used in calls to load in run time.
**
** History:
**     03-oct-1986 (kobayashi)
**          Created new for 5.0.
*/
typedef struct _FILE_CONTROL
{
    i4			classsize;
    ERCONTROL		control_record[CLASS_SIZE];
}   FILE_CONTROL;


/*{
** Name: build_fast_index - Build fast run time message file.
**
** Description:
**      This program read a set of message files and convert them into a 
**      single file with a search index.  The format of the message text
**	files is: 
**	    ~hex_number		    - The number assigned to the message
**	    KEY_WORD		    - The keyword assigned to the message.
**	    Text ...		    - The text associated with the message.
**
** Inputs:
**	input_file_name			Pointer to input file name.
**	output_file_name		Pointer to output file name.
**
** Outputs:
**	Returns: nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    File named fast_v?.mnx created in the current directory.
**
** History:
**	07-oct-1986 (kobayashi)
**          Created new for 5.0 KANJI.
*/
#ifdef  VMS
static void
build_fast_index(char *input_file_name, char *output_file_name, i4 flag)
{
    FILE_CONTROL    file_control;
    char	    record[RW_MAXLINE];
    FILE	    *in_fp;
    FILE_CONTEXT    output_file_context;
    i4		    end_of_file;
    i4		    record_size;
    i4		    line_number = 1;
    ER_MSGID	    last_msgid = 0;
    ER_MSGID	    msgid;
    ER_CLASS	    class_no;
    i4              mess_no;
    i4		    text_offset;
    i4		    tblsize;
    i4		    text_size;
    i4		    i;
    ER_CLASS	    last_class_no = -1;
    i4		    last_mess_no;
    i4		    loop_flag = 1;
    char	    textbuf[ER_MAX_LEN];
    i4		    text_length;

    /*	Create the output file. */
    create_output_file(&output_file_context, output_file_name, WRITE_SIZE);
    /*	Write an empty index block. */
    MEfill(sizeof(file_control), 0, &file_control);
    put_record(&output_file_context, 0, &file_control, WRITE_SIZE);
    /*  Open the input file. */
    if (open_input_file(input_file_name, &in_fp))
    {
	SIprintf("Error opening input file '%s'.\n", input_file_name);
	PCexit(FAIL);
    }
    /*  Read the first record. */
    if ( get_record(record, &record_size, sizeof(record), in_fp, &line_number) )
        loop_flag = 0;
#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif
    /*  Read message from file until end. */
    while(loop_flag)
    {
	/*  This record must begin with a '~'. */
        if (record[0] != '~')
	{
	    SIprintf("Error - Expecting '~' on line %d.\n\tOf file '%s'.\n",
		line_number, input_file_name);
	    PCexit(FAIL);
        }
	/* Convert the hexidecimal number that follows. */
	if ( convert_hex(record, record_size, &msgid) != OK )
        {
            SIprintf("Error - Bad hexidecimal digit on line %d.\n\tOf file '%s'.\n",
		line_number, input_file_name);
	    PCexit(FAIL);
	}
	/*	Check that the errors have been presented in ascending order. */
	if (msgid <= last_msgid)
	{
	    SIprintf("Error - Message number out of sequence at line %d.\n\tOf file '%s'.\n",
		line_number, input_file_name);
		SIprintf("msgid <= last_msgid: %d <= %d\n", msgid, last_msgid);
	    PCexit(FAIL);
	}
	last_msgid = msgid;
	/* Get class_number and mess_number from msgid */
	 class_no = ((msgid & CLASSMASK1) >> 16);
	 mess_no = msgid & MESSMASK;
        if (class_no  >= CLASS_SIZE)
        {
	    SIprintf("Error - Too many class number to fit in current file design.\n");
	    PCexit(FAIL);
        }
	/* If class number isn't order sequential, dummy data must be set.*/
	if (class_no != last_class_no)
	{
	    if(last_class_no != -1)
	    {
		file_control.control_record[last_class_no].offset = text_offset;
		file_control.control_record[last_class_no].areasize = text_size;
		file_control.control_record[last_class_no].tblsize = tblsize;
		text_offset += text_size;
	    }
	    else
	    {
		text_offset = WRITE_SIZE;
	    }
	    text_size = 0;
	    tblsize = 0;
	    last_class_no++;
	    for(; last_class_no != class_no; last_class_no++)
	    {
		file_control.control_record[last_class_no].offset = 
		0;
		file_control.control_record[last_class_no].areasize =
		0; 
		file_control.control_record[last_class_no].tblsize =
		0;
	    }
	    last_mess_no = -1;
	} 
    /* If mess number isn't order sequential, dummy data must be set.*/
        last_mess_no++;
        for(; last_mess_no < mess_no; last_mess_no++)
	{
        /* '0xff' for dummy data has to be set in file.
	**	And record_size is 1, Because null is usually set to file in 
	**	write_fastrec function.
	*/
	    text_size += write_fastrec(&output_file_context,"\377",1);
	    ++tblsize;
	}
    /*
    **	In fast message, next record isn't used, because next record
    **	is used only error message.
    */
	if ( get_record(record, &record_size, sizeof(record), in_fp,
	     &line_number) )
	{
	    SIprintf("Error - Unexpected end of file at line %d.\n\tOf file '%s'.\n",
		line_number, input_file_name);
	    break;
	}
#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif
	/* Read the text of the message. */
	text_length = 0;
	MEfill(sizeof(textbuf), 0, textbuf);
	for (;;)
	{
	    if ( end_of_file = get_record(record, &record_size, sizeof(record),
		in_fp, &line_number) )
		break;
#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif
	    if (record[0] == '~')
	    {
		text_size += write_fastrec(&output_file_context,textbuf,text_length);
		tblsize++;
		break;
	    }
	    record_size = convert_esc(record, record_size);
	    if ( record_size + text_length > ER_MAX_LEN )
	    {
		if ( text_length < ER_MAX_LEN )
		{
		    SIprintf("Warning - Message text truncated at line %d.\n\tOf file '%s'.\n",
			line_number, input_file_name);
		}
		record_size = ER_MAX_LEN - text_length;
	    }
	    MEcopy(record, record_size, &textbuf[text_length]);
	    text_length += record_size;
	}
	if (end_of_file)
	    break;
    }
    text_size += write_fastrec(&output_file_context,textbuf,text_length);
    tblsize++;
	/*  Close the current file. */
    SIclose(in_fp);    
    /*	Force the last data block to disk. */
    (VOID)write_fastrec(&output_file_context, (char *)NULL,0);
    file_control.control_record[class_no].offset = text_offset;
    file_control.control_record[class_no].areasize = text_size;
    file_control.control_record[class_no].tblsize = tblsize;
    file_control.classsize = class_no + 1;
    /*	Write the index block. */
    put_record(&output_file_context, 0, &file_control, WRITE_SIZE);
    /*	Close the output file. */
    close_file(&output_file_context);
    if ( !(flag & SAON) )
        delete_file(input_file_name);
}
#else
static void
build_fast_index(char *input_file_name, char *output_file_name, i4 flag)
{
    FILE_CONTROL    file_control;
    char	    record[RW_MAXLINE];
    FILE	    *in_fp,
		    *out_fp;
    LOCATION	    f_loc;
    i4		    end_of_file;
    i4		    record_size;
    i4		    line_number = 1;
    ER_MSGID	    last_msgid = 0;
    ER_MSGID	    msgid;
    ER_CLASS	    class_no;
    i4              mess_no;
    i4		    text_offset = 0;
    i4		    tblsize = 0;
    i4		    text_size = 0;
    ER_CLASS	    last_class_no = -1;
    i4		    last_mess_no;
    i4		    loop_flag = 1;
    char	    textbuf[ER_MAX_LEN];
    i4		    text_length;
    /*	Open the output file. */
    LOfroms(PATH & FILENAME, output_file_name, &f_loc);
    if ( SIfopen(&f_loc, "w", SI_RACC, sizeof(file_control), &out_fp) != OK )
    {
	SIprintf("Error opening output file '%s'.\n", output_file_name);
	PCexit(FAIL);
    }
    /*	Write an empty index block. */
    MEfill(sizeof(file_control), 0, &file_control);
    put_record(out_fp, 0, &file_control, sizeof(file_control));
    /*  Open the input file. */
    if (open_input_file(input_file_name, &in_fp))
    {
	SIprintf("Error opening input file '%s'.\n", input_file_name);
	PCexit(FAIL);
    }
    /*  Read the first record. */
    if ( get_record(record, &record_size, sizeof(record), in_fp, &line_number) )
        loop_flag = 0;
#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif
    /*  Read message from file until end. */
    while(loop_flag)
    {
	/*  This record must begin with a '~'. */
        if (record[0] != '~')
	{
	    SIprintf("Error - Expecting '~' on line %d.\n\tOf file '%s'.\n",
		line_number, input_file_name);
	    PCexit(FAIL);
        }
	/* Convert the hexidecimal number that follows. */
	if ( convert_hex(record, record_size, &msgid) != OK )
        {
            SIprintf("Error - Bad hexidecimal digit on line %d.\n\tOf file '%s'.\n",
		line_number, input_file_name);
	    PCexit(FAIL);
	}
	/*	Check that the errors have been presented in ascending order. */
	if (msgid <= last_msgid)
	{
	    SIprintf("Error - Message number out of sequence at line %d.\n\tOf file '%s'.\n",
		line_number, input_file_name);
	    PCexit(FAIL);
	}
	last_msgid = msgid;
	/* Get class_number and mess_number from msgid */
	 class_no = ((msgid & CLASSMASK1) >> 16);
	 mess_no = msgid & MESSMASK;
#ifdef xDEBUG
SIprintf("Msgid: %d, %x\n", msgid, msgid);
SIprintf("Class no.: %d\n", class_no);
SIprintf("Mesg no.: %d\n", mess_no);
#endif
        if (class_no  >= CLASS_SIZE)
        {
	    SIprintf("Error - Too many class number to fit in current file design.\n");
	    PCexit(FAIL);
        }
	/* If class number isn't order sequential, dummy data must be set.*/
	if (class_no != last_class_no)
	{
	    if(last_class_no != -1)
	    {
		file_control.control_record[last_class_no].offset = text_offset;
		file_control.control_record[last_class_no].areasize = text_size;
		file_control.control_record[last_class_no].tblsize = tblsize;
		text_offset += text_size;
	    }
	    else
	    {
		text_offset = sizeof(file_control);
	    }
	    text_size = 0;
	    tblsize = 0;
	    last_class_no++;
	    for(; last_class_no != class_no; last_class_no++)
	    {
		file_control.control_record[last_class_no].offset = 
		0;
		file_control.control_record[last_class_no].areasize =
		0; 
		file_control.control_record[last_class_no].tblsize =
		0;
	    }
	    last_mess_no = -1;
	} 
    /* If mess number isn't order sequential, dummy data must be set.*/
        last_mess_no++;
        for(; last_mess_no < mess_no; last_mess_no++)
	{
        /* '0xff' for dummy data has to be set in file.
	**	And record_size is 1, Because null is usually set to file in 
	**	write_fastrec function.
	*/
	    text_size += write_fastrec(out_fp, "\377", 1);
	    ++tblsize;
	}
    /*
    **	In fast message, next record isn't used, because next record
    **	is used only error message.
    */
	if ( get_record(record, &record_size, sizeof(record), in_fp,
	     &line_number) )
	{
	    SIprintf("Error - Unexpected end of file at line %d.\n\tOf file '%s'.\n",
		line_number, input_file_name);
	    break;
	}
#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif
	/* Read the text of the message. */
	text_length = 0;
	MEfill(sizeof(textbuf), 0, textbuf);
	for (;;)
	{
	    if ( end_of_file = get_record(record, &record_size, sizeof(record),
		in_fp, &line_number) )
		break;
#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif
	    if (record[0] == '~')
	    {
		text_size += write_fastrec(out_fp, textbuf, text_length);
		tblsize++;
		break;
	    }
	    record_size = convert_esc(record, record_size);
	    if ( record_size + text_length > ER_MAX_LEN )
	    {
		if ( text_length < ER_MAX_LEN )
		{
		    SIprintf("Warning Message text truncated at line %d.\n\tOf file '%s'.\n",
			line_number, input_file_name);
		}
		record_size = ER_MAX_LEN - text_length;
	    }
	    MEcopy(record,record_size, &textbuf[text_length]);
	    text_length += record_size;
	}
	if (end_of_file)
	    break;
    }
    text_size += write_fastrec(out_fp, textbuf, text_length);
    tblsize++;
	/*  Close the current file. */
    SIclose(in_fp);    
    /*	Force the last data block to disk. */
    (VOID)write_fastrec(out_fp, (char *)NULL, 0);
    file_control.control_record[class_no].offset = text_offset;
    file_control.control_record[class_no].areasize = text_size;
    file_control.control_record[class_no].tblsize = tblsize;
    file_control.classsize = class_no + 1;
    /*	Write the index block. */
    put_record(out_fp, 0, &file_control, sizeof(file_control));
    /*	Close the output file. */
    SIclose(out_fp);
    if ( !(flag & SAON) )
        delete_file(input_file_name);
}
#endif


/*{
** Name: build_slow_index	- The build function of ERCOMPILE.
**
** Description:
**      This program read a set of message files and convert them into a 
**      single file with a search index.  The format of the message text
**	files is: 
**	    ~hex_number		    - The number assigned to the message
**	    KEY_WORD		    - The keyword assigned to the message.
**	    Text ...		    - The text associated with the message.
**
**	Format change to prevent overflows:
**
**	Rather than starting the file with simply an INDEX_PAGE containing
**	a maximum of ER_SMAX_KEY buckets, we allow multiple index page
**	records.  The added pages are simply extensions to the i4
**	array on the end of the INDEX_PAGE structure.
**
**	To accomodate a multiple page index without changing things too
**	much, we have to scan our input twice, writing the index pages 
**	on the first pass, and the message text on the second.  There is
**	an implicit assumption here that the file is not going to change
**	underneath us between passes.
**
**
** Inputs:
**      infile				Pointer to input file name.
**      output				Pointer to output file name.
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    File named errmsg.mnx created in the current directory.
**
** History:
**	03-nov-1986 
**          Created new for 5.0.
**	12-jun-1989 (jrb)
**	    Changed for new txt.s format and new slow.mnx format to support
**	    generic error codes in slow messages.
**	5/90 (bobm)
**	    Changed slow message format to remove 511 bucket restriction
**	    by using multiple pages to contain the bucket array.  Made
**	    VMS ifdef's bracket a few lines rather than different versions
**	    of the entire routine.
*/
static void
build_slow_index(char *infile, char *outfile, i4 flag)
{
	SLOW_CONTEXT	ctx;
#ifdef VMS
	FILE_CONTEXT	fctx;

	ctx.fp = &fctx;
#endif

	/*
	** scan_slow() scans the input, passing message information and
	** our SLOW_CONTEXT block to a handler.  The first handler,
	** shdr_hdl(), writes out index pages.  The second, stxt_hdl()
	** writes out message records.  scan_slow() also handles where
	** page breaks should occur, and passes a flag to the handler,
	** assuring that breaks occur the same places in both passes.
	*/
	sctx_init(outfile, &ctx);
	scan_slow(infile, shdr_hdl, &ctx);
	shdr_flush(&ctx);
	scan_slow(infile, stxt_hdl, &ctx);
	stxt_flush(&ctx);

	if ( !(flag & SAON) )
	    delete_file(infile);
}

/*
** create the output file and initialize the SLOW_CONTEXT structure
*/
static VOID
sctx_init(char *outfile, SLOW_CONTEXT *ctx)
{
	LOCATION loc;

	MEfill (sizeof(SLOW_CONTEXT), (char) 0, (PTR) ctx);

#ifdef VMS
	create_output_file(ctx->fp,outfile,sizeof(INDEX_PAGE));
#else
	LOfroms(PATH & FILENAME, outfile, &loc);
	if (SIfopen(&loc, "w", SI_RACC, sizeof(INDEX_PAGE), &(ctx->fp)) != OK)
	{
	    SIprintf("Error opening output file '%s'.\n", outfile);
	    PCexit(FAIL);
	}
#endif

}

/*
** handler to write index records:
**
** On a page break:
**	bump index_count of number of buckets, and the pbreak count.
**	reset mcount, which counts the number of message on current page.
**	if pbreak indicates we've filled up an index page record, write
**	out the record, reset pbreak and bump the page count.
**
**	NOTE - on the first write, a trash record will be written, since
**		we are writing ctx->dat, and not the leading INDEX_PAGE.
**		We have to write something to be able write the following
**		records (at least on UNIX), and we have to backpatch the
**		leading index page when we're done because until then we
**		don't have a correct index_count.  When we backpatch, the
**		trash gets overwritten with the correct INDEX_PAGE.
**
** On every message, we bump mcount, and record it's index in the appropriate
** array slot.
*/
static VOID
shdr_hdl(MESSAGE_ENTRY *mess,bool pb,SLOW_CONTEXT *ctx)
{
	if (pb)
	{
		++(ctx->ip.index_count);
		++(ctx->pbreak);
		ctx->mcount = 0;
		if ((ctx->pages == 0 && ctx->pbreak == ER_SMAX_KEY) ||
			ctx->pbreak == (sizeof(INDEX_PAGE)/sizeof(i4)))
		{
			put_record (ctx->fp, ctx->pages,
				ctx->dat, sizeof(INDEX_PAGE));
			++(ctx->pages);
			ctx->pbreak = 0;
		}
	}

	++(ctx->mcount);

	if (ctx->pages == 0)
		(ctx->ip.index_key)[ctx->pbreak] = mess->me_msg_number;
	else
		(ctx->dat)[ctx->pbreak] = mess->me_msg_number;
}

/*
** end of index page pass.  If mcount > 0, there is a partial index
** array waiting to be flushed out.  Do so.  As in shdr_hdl(), if
** it is record 0, it is trash which will be backpatched over later,
** but we need to write it in order to write the message records.
**
**	(mcount will actually only wind up 0 for a completely empty
**	 message file, given the way scan_slow() handles breaks)
**
** We hold off on backpatching the index page until we are completed
** with everything so that the file doesn't actually get stamped with
** the proper magic number until it is fully written.
**
** We initialize the textp pointer for writing message records, reusing
** the ctx->dat item coerced into (char *).
**
** important - we leave ctx->pages alone so that we can continue to use
** it as a record index in the next pass.
*/
static VOID
shdr_flush(SLOW_CONTEXT *ctx)
{
	if (ctx->mcount > 0)
	{
		++(ctx->ip.index_count);
		put_record (ctx->fp, ctx->pages,
				ctx->dat, sizeof(INDEX_PAGE));
		++(ctx->pages);
	}
	ctx->textp = (char *) ctx->dat;
	MEfill(sizeof(INDEX_PAGE), (char) 0, (PTR) ctx->dat);
}

/*
** handler for a message entry.  On page breaks, write a record and reset
** the text pointer.
**
** the message entries are simply accumulated into a text buffer until
** they are written.
*/
static VOID
stxt_hdl(MESSAGE_ENTRY *mess,bool pb,SLOW_CONTEXT *ctx)
{
	if (pb)
	{
		put_record(ctx->fp, ctx->pages,
				ctx->dat, sizeof(INDEX_PAGE));
		++(ctx->pages);
		ctx->textp = (char *) ctx->dat;
		MEfill(sizeof(INDEX_PAGE), (char) 0, (PTR) ctx->dat);
	}

	MEcopy ((PTR) mess, mess->me_length, (PTR) ctx->textp);
	ctx->textp += mess->me_length;
}

/*
** OK, finished.  If textp != the start of the buffer, there is a final
** record to be flushed.  (again, there should always be something to
** flush unless we have an empty message file).
**
** set magic number / version in the index page, and backpatch it.
**
** close file
*/
static VOID
stxt_flush(SLOW_CONTEXT *ctx)
{
	if (ctx->textp != (char *) ctx->dat)
		put_record (ctx->fp, ctx->pages,
				ctx->dat, sizeof(INDEX_PAGE));
	ctx->ip.sanity = ER_SANITY(ER_MAGIC,ER_VERSION);
	put_record (ctx->fp, 0, &(ctx->ip), sizeof(INDEX_PAGE));
#ifdef VMS
	close_file(ctx->fp);
#else
	SIclose(ctx->fp);
#endif
}

/*{
** Name: scan_slow() - scan slow messages.
**
** Description:
**	This is the guts of a pass through the slow messages.  It
**	constructs MESSAGE_ENTRY's and passes them to a handler, as
**	well as keeping track of where page breaks occur.
**
** Inputs:
**      infile		Pointer to input file name.
**	hdl		handler.
**	ctx		context block for handler.
**
**	Handler call:
**		(*hdl)(mb,pb,ctx)
**		MESSAGE_ENTRY *mb;	current message
**		bool pb;		TRUE if page break
**		SLOW_CONTEXT *ctx;	context block passed to scan_slow
**
** Outputs:
**	Returns:
**	    SUCCESS
**	    ABORT
**	Exceptions:
**	    none
**
** Side Effects:
**	    File named errmsg.mnx created in the current directory.
**
** History:
**	03-nov-1986 
**          Created new for 5.0.
**	12-jun-1989 (jrb)
**	    Changed for new txt.s format and new slow.mnx format to support
**	    generic error codes in slow messages.
**	5/90 (bobm)
**	    extracted from old build_slow_index().
**	21-oct-92 (andre)
**	    removed me_generic_error from MESSAGE_ENTRY and added me_sqlstate
*/
static VOID
scan_slow(char *infile,
	VOID (*hdl)(MESSAGE_ENTRY *, bool, SLOW_CONTEXT *),
	SLOW_CONTEXT *ctx)
{
    char	    record[RW_MAXLINE];
    MESSAGE_ENTRY   current_message;
    FILE	    *in_fp;
    i4		    end_of_file;
    i4		    record_size;
    i4		    line_number = 1;
    i4		    page_next = 0;
    i4		    last_message = -1;
    i4		    message_number;
    char    	    *sqlstate;
    i4		    msg_name_len;
    char	    *p;
    bool	    page_break;


	/*  Open the input file. */
	if (open_input_file(infile, &in_fp))
	{
	    SIprintf("Error opening input file '%s'.\n", infile);
	    PCexit(FAIL);
	}    

	/*  Read the first record. */
	if ( get_record(record, &record_size, sizeof(record), in_fp,
	     &line_number) )
	{
	    SIprintf("Error - Unexpected end of file at line 1.\n\tOf file '%s'.\n", infile);
	    PCexit(FAIL);
	}

#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif /* xDEBUG */

	/*  Read message from file until end. */
	for (;;)
	{
	    /*  This record must begin with a '~'. */
	    if (record[0] != '~')
	    {
		SIprintf("Error - Expecting '~' on line %d.\n\tOf file '%s'.\n",
		    line_number, infile);
		PCexit(FAIL);
	    }

	    /* Convert the hexadecimal number that follows. */
	    if ( convert_hex(record, record_size, &message_number) != OK )
	    {
	 	SIprintf("Error - Bad hexidecimal digit on line %d.\n\tOf file '%s'.\n",
		    line_number, infile);
		PCexit(FAIL);
	    }
	    /*	Check that the errors have been presented in ascending order. */
	    if (message_number <= last_message)
	    {
		SIprintf("Error - Message number out of sequence at line %d.\n\tOf file '%s'.\n",
		    line_number, infile);
		PCexit(FAIL);
	    }
	    if ( get_record(record, &record_size, sizeof(record), in_fp,
		 &line_number) )
	    {
		SIprintf("Error - Unexpected end of file at line %d.\n\tOf file '%s'.\n",
		    line_number, infile);
		break;
	    }
#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif /* xDEBUG */

	    if ((p = STindex(record, ":", record_size)) == NULL)
	    {
		SIprintf("Error - SQLSTATE status code not appended to message name.\n");
		SIprintf("\taborting at %d in file '%s'.\n", line_number, infile);
		PCexit(FAIL);
	    }
	    
	    /* set name length */
	    msg_name_len = p - record;

	    if (msg_name_len > ER_MAX_NAME)
	    {
		SIprintf("Warning - Message name truncated at line %d.\n\tOf file '%s'.\n",
		    line_number, infile);
		msg_name_len = ER_MAX_NAME;
	    }

	    /*
	    ** SQLSTATE status code is already stored as a string - verify that
	    ** it is of correct length (5) and save a ptr to its beginning
	    */
	    if ((record_size - (p - record + 1)) != 5)
	    {
		SIprintf("Error - invalid SQLSTATE status code length - %d\n",
		    record_size - (p - record + 1));
		SIprintf("on line %d of file '%s'\n", line_number, infile);
		PCexit(FAIL);
	    }
	    
	    sqlstate = p + 1;

	    MEfill(sizeof(current_message), 0, &current_message);
	    MEcopy(record, msg_name_len, current_message.me_name_text);
	    current_message.me_name_text[msg_name_len] = '\t';
	    current_message.me_msg_number = message_number;
	    current_message.me_name_length = (i2)msg_name_len;

	    /*
	    ** copy SQLSTATE status code and verify that it consists of valid
	    ** characters - [A-Z0-9]
	    */
	    {
		i4		i;

		for (i = 0; i < 5; i++)
		{
		    if (   (sqlstate[i] < '0' || sqlstate[i] > '9')
			&& (sqlstate[i] < 'A' || sqlstate[i] > 'Z')
		       )
		    {
			SIprintf("Error - invalid SQLSTATE status code '%5s'\n",
			    sqlstate);
			SIprintf("on line %d of file '%s'\n", line_number,
			    infile);
			PCexit(FAIL);
		    }
		    
		    current_message.me_sqlstate[i] = sqlstate[i];
		}
	    }

	    /* Read the text of the message. */
	    for (;;)
	    {
		MEfill(sizeof(record), 0, record);
		if (end_of_file = get_record(record, &record_size,
		    sizeof(record), in_fp, &line_number))
		    break;
#ifdef xDEBUG
    SIprintf("Record : %s\n", record);
    SIprintf("Rec. size : %d\n", record_size);
#endif /* xDEBUG */
		if (record[0] == '~')
		    break;
		record_size = convert_esc(record, record_size);
		if (record_size + current_message.me_text_length > ER_MAX_LEN)
		{
		    if (current_message.me_text_length < ER_MAX_LEN)
		    {
			SIprintf("Warning - Message text truncated at line %d.\n\tOf file '%s'.\n",
			    line_number, infile);
		    }
		    record_size = ER_MAX_LEN - current_message.me_text_length;
		}
		MEcopy(record, record_size, &current_message.me_name_text[
		    current_message.me_name_length + 1 +
		    current_message.me_text_length]);
		current_message.me_text_length += (i2)record_size;
	    }

	    /* Calculate size of message with roundup to multiple of 4. */
	    current_message.me_length = sizeof(current_message) -
		sizeof(current_message.me_name_text) +
		current_message.me_name_length + 1 +
		current_message.me_text_length;
	    current_message.me_length += 4 - (current_message.me_length & 3);

	    /* call handler */
	    page_next += current_message.me_length;
	    if (page_next > sizeof(INDEX_PAGE))
	    {
		page_break = TRUE;
		page_next = current_message.me_length;
	    }
	    else
	    {
		page_break = FALSE;
	    }

	    (*hdl)(&current_message,page_break,ctx);

	    last_message = current_message.me_msg_number;
	    if (end_of_file)
		break;
    	}

	/*  Close the current file. */
	SIclose(in_fp);    
}

/*{
** Name: write_fastrec		- Separate data for page and put page to file
**
** Description:
**	Separate data for each page and page to file. As page's size is 512byte,
**	data is set to temporary buffer, and it put this buffer to file
**	when temporary buffer is full. 
**	
** Input:
**	fp			    pointer to file descripter
**	record			    record to be like to write
**	recordsize		    size of record
**
** Output:
**	Return:
**	    number of byte to be set to temporary buffer
**	Exception:
**	    none
**  
** Side effect:
**	This doesn't write contents of temporary buffer to file until to full
**	temporary file. When temporary file is full, data is written to file.
**
** History:
**	06-Oct-1986 (kobayashi) - first written
*/
#ifdef  VMS
static i4
write_fastrec(FILE_CONTEXT *file_context,char *record,i4 recordsize)
{
    static char	    tempbuf[WRITE_SIZE];
    static char	    *ptemp = tempbuf;
    i4  i;
    static i4	    blk = 1;

    if (record == (char *)NULL)
    {
	if (ptemp != tempbuf)
	    put_record(file_context,blk,tempbuf,WRITE_SIZE);
	return(0);
    }
    if (*record == NULL)
	recordsize = 0;
    for (i = 0; i <= recordsize; ++i)
    {
	if (i == recordsize)
	    *ptemp++ = NULL;
	else
	    *ptemp++ = *record++;
	if (ptemp >= tempbuf + WRITE_SIZE)
	{
	    put_record(file_context,blk,tempbuf,WRITE_SIZE);
	    ptemp = tempbuf;
	    MEfill(WRITE_SIZE,0,tempbuf);
	    blk++;
	}
    }
    return(i);
}
#else
static i4
write_fastrec(FILE_CONTEXT *fp, char *record, i4 recordsize)
{
    static char	    tempbuf[sizeof(FILE_CONTROL)];
    static char	    *ptemp = tempbuf;
    i4  i;
    static i4	    blk = 1;
    if (record == (char *)NULL)
    {
	if (ptemp != tempbuf)
	    put_record(fp, blk, tempbuf, sizeof(FILE_CONTROL));
	return(0);
    }
    if (*record == EOS)
	recordsize = 0;
    for (i = 0; i <= recordsize; ++i)
    {
	if (i == recordsize)
	    *ptemp++ = EOS;
	else
	    *ptemp++ = *record++;
	if (ptemp >= tempbuf + sizeof(FILE_CONTROL))
	{
	    put_record(fp, blk, tempbuf, sizeof(FILE_CONTROL));
	    ptemp = tempbuf;
	    MEfill(sizeof(FILE_CONTROL), 0, tempbuf);
	    blk++;
	}
    }
    return(i);
}
#endif


/*{
** Name: put_record	- Write record to output file.
**
** Description:
**      Write the specified record to the output file.
**
** Inputs:
**      fp                              Pointer to file descripter.
**	record_number			The record number to write this page
**					too.
**      record                          Pointer to record to be written.
**      record_size                     The size of the record to be written.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Error message is written and program terminated if error occurs.
**
** History:
**	03-oct-1986
**          Created new for 5.0.
*/
#ifdef  VMS
static VOID
put_record(FILE_CONTEXT *file_context, i4 record_number,
	void *record, i4 record_size)
{
    FILE_CONTEXT        *f = file_context;
    i4		status;

    /*	Setup for the write. */
    f->fc_rab.rab$l_rbf = (char *) record;
    f->fc_rab.rab$w_rsz = record_size;
    if (record_size != f->fc_fab.fab$w_mrs)
    {
	SIprintf("Error - Bad record size on page %d for output file '%s'\n", record_number,
	    f->fc_fab.fab$l_fna);
	PCexit(FAIL);
    }
    f->fc_rab.rab$l_bkt = 0;
    /* If this is a random write, then set the block number. */
    if (f->fc_rab.rab$l_ctx != record_number)
    {
	f->fc_rab.rab$l_bkt = record_number * (record_size / 512) + 1;
    }
    else
	f->fc_rab.rab$l_ctx++;
    /*	Write the record. */
    status = sys$write(&f->fc_rab);
    if ((status & 1) == 0)
    {
	SIprintf("Error writing page %d to output file '%s'.\n", record_number,
	    f->fc_fab.fab$l_fna);
	PCexit(FAIL);
    }
}
#else
static VOID
put_record(FILE *fp, i4 record_number,
	void *record, i4 record_size)
{
    OFFSET_TYPE		offset;
    i4			count;
    count = 0;
    /*	Setup for the write. */
    offset = record_size * record_number;
    SIfseek(fp, offset, SI_P_START);
    if ( SIwrite(record_size, (char *) record, &count, fp) != OK )
    {
	SIprintf("Error writing page %d to outut file.\n", record_number);
	PCexit(FAIL);
    }
}
#endif


/*{
** Name: create_output_file	- Create the message output file.
**
** Description:
**      Create the file used to output the compiled message text.  The file
**	is created as fixed length records that are a multiple of the block
**	size of the disk.
**
** Inputs:
**      file_context                    File context initialized by the call.
**      file_name                       String containing the name of the file.
**      record_size                     The size of the records written to disk.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    With exit with an error if output file can't be created.
**
** History:
**	03-oct-1985 (derek)
**          Created new for 5.0.
*/
#ifdef  VMS
static VOID
create_output_file(FILE_CONTEXT *file_context,
	char *file_name, i4 record_size)
{
    FILE_CONTEXT        *f = file_context;
    i4		status;

    /*  Initialize the FAB and RAB. */
    MEfill(sizeof(*f), 0, f);
    f->fc_fab.fab$b_bid = FAB$C_BID;
    f->fc_fab.fab$b_bln = FAB$C_BLN;
    f->fc_rab.rab$b_bid = RAB$C_BID;
    f->fc_rab.rab$b_bln = RAB$C_BLN;
    f->fc_rab.rab$l_fab = &f->fc_fab;
    f->fc_fab.fab$b_rfm = FAB$C_FIX;
    f->fc_fab.fab$b_rat = 0;
    f->fc_fab.fab$b_org = FAB$C_SEQ;
    f->fc_fab.fab$w_mrs = record_size;
    f->fc_fab.fab$l_fna = file_name;
    f->fc_fab.fab$b_fns = STlength(file_name);
    /*  Special options to perform block i/o. */
    f->fc_fab.fab$b_fac = FAB$M_BRO;
    f->fc_rab.rab$l_rop = RAB$M_BIO;
    /*	Next sequential block to write. */
    f->fc_rab.rab$l_ctx = 0;
    /*	Create the file. */
    status = sys$create(&f->fc_fab);
    if ((status & 1) == 0)
    {
	SIprintf("Error creating output file '%s'.\n\tVMS ERROR %%%X.\n",
	    file_name, status);
	PCexit(FAIL);
    }
    status = sys$connect(&f->fc_rab);
    if ((status & 1) == 0)
    {
	SIprintf("Error connecting output file '%s'.\n\tVMS ERROR %%%X.\n",
	    file_name, status);
	PCexit(FAIL);
    }    
}
#endif


/*{
** Name: close_file	- Close a input or output file.
**
** Description:
**      Close the input or output file that is passed in.
**
** Inputs:
**      file_context                    The file context of an open file.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Error message is written and program exitted if any erros occur.
**
** History:
**	03-oct-1985 (derek)
**          Created new for 5.0.
*/
#ifdef  VMS
static VOID
close_file(FILE_CONTEXT *file_context)
{
    FILE_CONTEXT        *f = file_context;
    i4		status;

    /*	Close the file. */
    status = sys$disconnect(&f->fc_rab);
    if ((status & 1) == 0)
    {
	SIprintf("Error disconnecting file '%s'.\n", f->fc_fab.fab$l_fna);
	PCexit(FAIL);
    }
    status = sys$close(&f->fc_fab);
    if ((status & 1) == 0)
    {
	SIprintf("Error closing file '%s'.\n", f->fc_fab.fab$l_fna);
	PCexit(FAIL);
    }
}
#endif


/*{
** Name: open_input_file	- Open text file for input.
**
** Description:
**      Open the specified file for input.
**
** Inputs:
**	file_name			The name of the file to open.
**	fp				Pointer to file descripter.
**
** Outputs:
**	Returns:
**	    TRUE			Error opening file.
**	    FALSE			File opened correctly.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Error message is written and program exitted if the file can't
**	    be opened.
**
** History:
**	24-nov-1986
**          Created new for 5.0.
*/
static bool
open_input_file(char *file_name, FILE **fp)
{
    LOCATION		f_loc;
    /*  Open the file. */
    LOfroms(PATH & FILENAME, file_name, &f_loc);
    if ( SIopen(&f_loc, "r", fp) != OK )
    {
	return(TRUE);
    }
    return (FALSE);
}


/*{
** Name: delete_file	- Delete the named file.
**
** Description:
**      Delete the specified file.
**
** Inputs:
**	file_name			The name of the file to delete.
**
** Outputs:
**	Returns:
**	    none.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	11-feb-1987
**          Created new for 5.0.
*/
static VOID
delete_file(char *file_name)
{
    LOCATION		f_loc;
    LOfroms(PATH & FILENAME, file_name, &f_loc);
    LOdelete(&f_loc);
}


/*{
** Name: get_record	- Get next record from input file.
**
** Description:
**      Read the next record from the input file.  
**
** Inputs:
**      record				The address of the record buffer.
**	result_size			Pointer to location to return resulting
**					record size after read.
**	record_size			The size of the record buffer.
**	fp				Pointer to file descriptor.
**	line_count			Pointer to location to return line
**					number of the current record.
**
** Outputs:
**	Returns:
**	    FALSE			Not end of file.
**	    TRUE			End of file.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-nov-1985 
**          Created new for 5.0.
*/
static bool
get_record(char *record, i4 *result_size, i4 record_size,
	FILE *fp, i4 *line_count)
{
    bool	at_end = FALSE;

    *result_size = 0;

    if (SIgetrec(record, record_size, fp) == ENDFILE)
    {
	at_end = TRUE;
    }
    else
    {
	*result_size = (i4)STlength(record);
	/*
	** assume that record_size is longer (much longer) than
	** the longest line in the file, so no need to check that
	** the last character read in is really a \n.
	*/
	--(*result_size);	/* ignore the \n at the end */
	(*line_count)++;
    }

    record[*result_size] = EOS;	/* overwrite the \n (+ terminate the buffer) */

    return(at_end);
}


/*{
** Name: convert_hex	- Converts the hexidecimal number.
**
** Description:
**      This routine converts hexidecimal to i4.  Note: this routine
**	assumes that the hex number starts *after* the first character.
**
** Inputs:
**      record			Pointer to buffer to convert.
**	record_size		Length of buffer. 
**      number			Pointer to number.
** Outputs:
**	Returns:
**	    OK
**	    FAIL
**
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	03-nov-1986 
**          Created new for 5.0.
*/
static STATUS
convert_hex(char *record, i4 record_size, i4 *number)
{
    i4	    i;

    *number = 0;
    for (i = 1; i < record_size; i++)
    {
	if (record[i] >= '0' && record[i] <= '9')
	    *number = (*number << 4) + record[i] - '0';
	else if (record[i] >= 'a' && record[i] <= 'f')
	    *number = (*number << 4) + record[i] - 'a' + 10;
	else if (record[i] >= 'A' && record[i] <= 'F')
	    *number = (*number << 4) + record[i] - 'A' + 10;
	else
	    return(FAIL);
    }
    return(OK);
}


/*{
** Name: convert_esc	- Converts escape sequence to ascii.
**
** Description:
**      This routine converts '\n', '\t' ,'\v', '\b', '\r', '\f', '\0' and
**	'\ddd' to ASCII code.
**
** Inputs:
**      buf			Pointer to buffer to convert.
**	length			Length of buffer. 
**      
** Outputs:
**	Returns:
**	    Length of converted stringth.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	03-nov-1986 
**          Created new for 5.0.
*/
static i4
convert_esc(char *buf, i4 length)
{
    char	*s,
		*c;
    char	t;
    i4		n;
    i2		i;
    s = c = buf;
    while ( c - buf < length )
    {
	switch (t = *c++)
	{
	    case '\\':
	    {
	      if ( CMdbl1st(c - 2) )
	      {
		*s++ = t;
		continue;
	      }
	      else
	      {
		if ( c - buf == length )
		    goto exit;
		switch (t = *c++)
		{
		    case 'n':
			*s++ = NL;
			continue;
		    case 't':
			*s++ = HT;
			continue;
		    case 'v':
			*s++ = VT;
			continue;
		    case 'r':
			*s++ = CR;
			continue;
		    case 'f':
			*s++ = FF;
			continue;
		    case 'b':
			*s++ = BS;
			continue;
		    case '0': case '1': case '2': case '3':
		    case '4': case '5': case '6': case '7':
			n = 0;
			n = 8 * n + t - '0';
			for (i = 0; (i < 2) && (*c >= '0' && *c <= '7'); i++)
			    n = 8 * n + (*c++) - '0';
			*s++ = (char)n;
			continue;
		    default :
			*s++ = t;
			continue;
		}
	      }
	    }
	    default :
		*s++ = t;
	}
    }
exit:
    return ((i4)(s - buf));
}


/*{
** Name: sort_msg - Sort input file by MSGID feild.
**
** Description:
**	This routine sorts input file by MSGID field, and creates output file 
**	ordered by MSGID.  
**	
** Inputs:
**	inf_name			Pointer to input file name.
**	outf_name			Pointer to output file name.
**	flag				Holds description of user options
**
** Outputs:
** 	Returns:
**	    VOID.	
**
** Side Effects:
**	Creates & deletes temporary RACC file
**	Deletes temporary input file unless flag indicates to save
**
** ALgorithm:
**	1.  Reads input file and count the number of message ids
**	2.  Allocates memory for an array to hold message ids and
**	    their offsets in the input file.
**	3.  Reread the input file, and for each message id, store it and
**	    its location in the input file into the message id array.
**	4.  Sort the message ids in the array.
**	5.  Write the messsages from the input file to the output file using
**	    the sorted array to determine the message order in the output file.
**
** History:
**	01-Nov-1986
**	Create new for 5.0.
**	26-aug-1987 
**		Change output to use retrieve to avoid VMS bug in 6.0 copy.
**	22-jan-1988 (mhb)
**		Changed sort_msg function to do its own quick sort
**		instead of calling database sort
*/
typedef struct 	_MSGID_PAIR
{
    i4		msgid;			/* msg id  */
    OFFSET_TYPE	offset;			/* msg location in file as an offset */
    i4		msglen;			/* # bytes for all 3 lines of msg */
}   MSGID_PAIR;

static VOID bubble(char *arr,i4 nel);

static VOID do_qs(char *arr,i4 nel);

static i4 er__bldracc(LOCATION *inf_loc, LOCATION *raccf_loc, MSGID_PAIR *prptr);

static int er__cmpids(const char *id1, const char *id2);

static i4 er__cntids(LOCATION *inf_loc);

static i4 er__wrtids(LOCATION *raccf_loc, LOCATION *outf_loc,
	MSGID_PAIR *arr, i4 nummsgs);


static VOID
sort_msg(char *inf_name, char *outf_name, i4 flag)
{

    MSGID_PAIR	*msgarp;	/* pointer to array of MSGID_PAIRs */
    i4  	nummsgs;	/* number of messages in input file */
    i4		ret_value;
    char	inf_buf[MAX_LOC+1],
    		outf_buf[MAX_LOC+1],
		raccf_buf[MAX_LOC+1];
    LOCATION	inf_loc,
    		outf_loc,
    		raccf_loc;

    STcopy( inf_name, inf_buf );
    LOfroms(PATH & FILENAME, inf_buf, &inf_loc );

    STcopy( outf_name, outf_buf );
    LOfroms(PATH & FILENAME, outf_buf, &outf_loc );

    STcopy( TMPSORT, raccf_buf );
    LOfroms(PATH & FILENAME, raccf_buf, &raccf_loc );
    
    if ((nummsgs = er__cntids(&inf_loc)) <= 0)   /* Count number of msg ids */
    {
	return;
    }

    msgarp = (MSGID_PAIR*)MEreqmem(0, (i4) nummsgs * sizeof(MSGID_PAIR),
		TRUE, NULL);
    if ( msgarp == NULL )
    {
	SIprintf("Unable to allocate memory for message id array \n");
	return;
    }

    /*  Build tmp racc file and msg array id with racc file loc in it */
    if ((ret_value=er__bldracc(&inf_loc, &raccf_loc, msgarp)) != nummsgs) 
    {
	SIprintf("ERROR - %d messages counted, %d messages stored\n",
		nummsgs, ret_value);
	return;
    }

    if ( !(flag & SAON) )	/* Delete temp file unless save was requested */
    {
        LOdelete(&inf_loc);	/* No longer needed */
    }

    if (er__qsortids((char *)msgarp,nummsgs,sizeof(MSGID_PAIR),er__cmpids)!=OK)
    {
	SIprintf("ERROR - er__qsortids\n");
	return;
    }

    if ((ret_value=er__wrtids(&raccf_loc, &outf_loc, msgarp, nummsgs))!=nummsgs)
    {
	SIprintf("ERROR - %d messages counted & stored, %d messages written\n",
		nummsgs, ret_value);
	return;
    }

    LOdelete(&raccf_loc);
}


/*
+*
** Name:  er__cntids	
**
** Description:
**	Count the number of message ids in input file
**
** Inputs:
**  	LOCATION *inf_loc	LOCATION of input file	
**
** Return Values:
**	>= 0	Number of messages counted
**	-1	Couldn't open input file
**
** Side Effects:
**
** History:
**	1/22/88 (mhb)	Written
*/
static i4
er__cntids(LOCATION *inf_loc)
{
    char	inbuf[MAX_MSG_LINE + 1];
    FILE	*inf_fd;
    i4		nummsgs;

    nummsgs = 0;			

    if (SIopen(inf_loc, "r", &inf_fd) != OK)
    {
	SIprintf("Error in er__cntids:  Cannot open input file\n");
	return(-1);
    }

    while (SIgetrec (inbuf, (i4)(MAX_MSG_LINE + 1), inf_fd) == OK)
    {
	if (CMcmpcase(inbuf, "~") == 0)
	{	/* Next message found */
		nummsgs++;
	}
    }
    _VOID_ SIclose(inf_fd);
    return(nummsgs);
}


/*
+*
** Name:  	er__bldracc
**
** Description:
**	Store message ids and the file offset of message in the message
**	id array
**
** Input:
**	LOCATION   *inf_loc	LOCATION of input file
** Output:
**	LOCATION   *raccf_loc	LOCATION of tmp racc file 
** 	MSGID_PAIR *prptr	Pointer to message id array in which to store
**				message ids and their file offsets
** Side affect:
**	Writes out a racc-file version of input file
**	Creates msgid array which contains msgid locations on the racc file
** Return Values:
**	>= 0	Number of ids stored successfully
**	-1	Could not open input file
**
** History:
**	1/22/88 (mhb)	Written
*/
static i4
er__bldracc(LOCATION *inf_loc, LOCATION *raccf_loc, MSGID_PAIR *prptr)
{

    char	iobuf[MAX_MSG_LINE+1];
    FILE	*inf_fd;	/* Input file's fd */
    FILE	*raccf_fd;	/* Output (RACC) file's fd */
    i4	len;		/* Number of bytes in input buf */
    i4	dummy;		/* Needed by SIwrite */ 
    OFFSET_TYPE	offset;		/* location of record in the file */
    i4		ids_stored = 0;	/* Actual # of ids stored by this routine */
    i4		i;
    STATUS	status = OK;

    if (SIopen(inf_loc, "r", &inf_fd) != OK)	/* Open text input tmp file */
    {
	SIprintf("Error in er__bldracc: Cannot reopen input file\n");
	return(-1);
    }
    if (SIfopen(raccf_loc,"w",SI_RACC,(i4)MAX_MSG_LINE+1,&raccf_fd) != OK)
    {
	SIprintf("ERROR in er__bldracc: Cannot create racc file \n");
	return(-1);
    }
    /* For each message:
    **		Get racc file loc
    **		Write msgid in array as hex number
    **		Write message out to racc file
    **		Write total length of message to msglen in msgid array
    */
    SIwrite(1, "\n", &dummy, raccf_fd);
    status = SIgetrec(iobuf,(i4)(MAX_MSG_LINE + 1),inf_fd);
    while (status == OK && CMcmpcase(iobuf, "~") == 0)	/* Message found */
    {
            offset = SIftell(raccf_fd );	/* get msg's loc in racc file */
	    prptr->offset = offset;
	    (void) convert_hex(&iobuf[1], GIDSIZE, &prptr->msgid);
	    prptr->msglen = 0;
	    for (i = 0; i < 3 && status == OK; i++)/* Write 3 lines per msg */
	    {
		len = (i4)STlength(iobuf);
		iobuf[len-1] = '\n';		/* Replace EOS with newline */
		SIwrite(len, iobuf, &dummy, raccf_fd);
		prptr->msglen += len;		/* Keep # of bytes in 3 lines */
	        status = SIgetrec(iobuf,(i4)(MAX_MSG_LINE + 1),inf_fd);
	    }
	    prptr++;
	    ids_stored++;
    }
    _VOID_ SIclose(inf_fd);
    _VOID_ SIclose(raccf_fd);
    return(ids_stored);
}

            
/*
+*
** Name:  	er__wrtids
**
** Description:
**	For each message id in the message id array, read the message
**	from the input file using the file offset stored with the id
**	in the message id array.  Write the message onto the output file.
**
** Inputs:
**
**	LOCATION    *inf_loc	LOCATION of input file
**	LOCATION    *outf_loc	LOCATION of output file
**	MSGID_PAIR  *arr	Pointer to message id array
**	i4 	    nummsgs	Number of messages to write
**
** Return Values:
**	>= 0	Number of messages written
**	-1	Couldn't open input or output file
**
**
** History:
**	1/22/88 (mhb)	Written
*/

static i4
er__wrtids(LOCATION *raccf_loc, LOCATION *outf_loc,
	MSGID_PAIR *arr, i4 nummsgs)
{
    FILE	*raccf_fd,
		*outf_fd;
    MSGID_PAIR	*arrp;
    char	iobuf[3*MAX_MSG_LINE];	/* Must hold 3 lines per message */
    i4		count;
    i4	dummy;			/* Needed by SIwrite */
    i4		ids_written;

    ids_written = 0;
    if (SIfopen(raccf_loc, "r", SI_RACC, (i4)MAX_MSG_LINE+1, &raccf_fd) != OK)
    {
	SIprintf("Error reopening of racc input file \n");
	return(FAIL);
    }
    if (SIopen(outf_loc, "w", &outf_fd)!=OK)
    {
	SIprintf("Error opening output file \n");
    	_VOID_ SIclose(raccf_fd);
	return(FAIL);
    }
    for (arrp = arr; nummsgs-- ; arrp++)
    {
	if (SIfseek(raccf_fd, arrp->offset, SI_P_START) != OK)
	{
	    SIprintf("ERROR - er_wrtids: fseek racc input file, offset = %d\n",
	    	arrp->offset);
	    break;
	}
	if (SIread(raccf_fd, arrp->msglen, &count, iobuf) == ENDFILE)
	{
	    SIprintf("ERROR - er__wrtids: EOF while trying to read racc\n"); 
	    break; 
	}
	if (CMcmpcase(iobuf, "~") != 0)
	{
	    SIprintf("ERROR - er__wrtids: Expecting msg, read: %s\n",
		iobuf);
	    break; 
        }
	SIwrite(count, iobuf, &dummy, outf_fd);
	ids_written++;
    } 	
    _VOID_ SIclose(outf_fd);
    _VOID_ SIclose(raccf_fd);
    return(ids_written);
}


#ifdef TIME_THRESHOLD
i4  THRESHOLD;
#else
#define THRESHOLD 6
#endif

/* remember - compund statement */
#define SWAP(x,y) MEcopy (x, Eltsize,Tempblock); \
			MEcopy (y, Eltsize,x); \
			MEcopy (Tempblock, Eltsize,y)

static i4  Tempsize = 0;
static i4  Eltsize;
static char *Tempblock;
static int  (*Compfunc)(const char *, const char *);


/*{
** Name:    er__qsortids() -	quick sort of message ids 
**
** Description:
**	A generic quick sort which uses a caller provided comparison
**	function to sort an array of elements of arbitrary size.
**	Precautions are taken to assure termination no matter what
**	the comparison function returns, although nonsense return
**	values will obviously lead to a nonsense ordering.
**
** Inputs:
**	char *arr		the array to be sorted
**	i4 nel			the number of elements in the array
**	i4 size		the size in bytes of a single element
**	i4 (*compare)()	pointer to the comparison function:
**
**		(*compare)(elt1,elt2)
**		char *elt1;
**		char *elt2;
**
**		elt1, elt2 will actually be the addresses of blocks
**		of size "size", starting at address "arr", ie. the
**		elements of the array, which (*compare) may coerce
**		into whatever datatype or structure is appropriate in
**		order to compare the elements.
**
**		return <0 for elt1 < elt2
**		return 0 for elt1 = elt2
**		return >0 for elt1 > elt2
**
** Outputs:
**	sorted array
**
**	Exceptions:
**		syserr for memory allocation failure
**		(an element may be allocated to use in swapping memory)
**
** History:
**	3/87 (bobm)	written
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	21-jan-88 (mhb) Copied into ercompile for sorting msgs
*/
static STATUS
er__qsortids(char *arr,i4 nel,i4 size,
	int (*compare)(const char *, const char *) )
{

    Eltsize = size;
    Compfunc = compare;

    /*
    ** keep Tempblock around to reuse as long as big enough.
    ** use the "generic" memory pool.
    */
    if (Eltsize > Tempsize)
    {
	if (Tempsize > 0)
	    MEfree((PTR) Tempblock);
	Tempsize = Eltsize;
	if ((Tempblock = (char *)MEreqmem(0, Tempsize, FALSE, (STATUS *)NULL))
		== NULL)
	{
	    SIprintf("er__qsortids: Bad Memory Allocation\n");
	    return (FAIL);
	}
    }
    do_qs(arr,nel);
    return (OK);
}

/*
** the real quick sort logic:
**
** Grab the middle element, take one pass through the array separating
** it into two arrays, the first strictly less than the comparison
** element, the second >= to it.  The normal trick for this is to
** run a pointer from each end of the array, swapping as we find elements
** that belong in the other array, and bailing out when they cross.
** Then sort the two pieces recursively.
*/
static VOID
do_qs(char *arr,i4 nel)
{
	register char *cmp;		/* comparison pointer */
	register char *hp,*tp;		/* head and tail pointers */
	register int hbump,tdec;	/* bump / decrement counters */

	if (nel < THRESHOLD)
	{
		bubble(arr,nel);
		return;
	}

	/*
	** Set comparison element.  We will have to reset this pointer
	** anytime we swap the comparison element, since we are pointing
	** directly into the array, rather than maintaining an element
	** off to the side.
	*/
	cmp = arr + (nel/2)*Eltsize;

	tdec = nel - 1;
	hbump = 0;
	hp = arr;
	tp = arr + (nel - 1)*Eltsize;
	for (;;)
	{
		/*
		** find the next elements to swap.
		**
		** We check against cmp to save a procedure call.
		** We know the answer is 0.  We also keep cmp >= hp,
		** which assures that the first while terminates even
		** even if Compfunc returns < 0 all the time.
		**
		** We keep hbump & tdec to avoid having to compare
		** pointers for anything except equality.
		*/
		while (hp != cmp && (*Compfunc)(hp,cmp) < 0)
		{
			++hbump;
			hp += Eltsize;
		}
		while (tp != arr && (tp == cmp || (*Compfunc)(tp,cmp) >= 0))
		{
			--tdec;
			tp -= Eltsize;
		}

		/* passed each other yet ? ( "=" can happen if ALL >= cmp) */
		if (hbump >= tdec)
			break;

		/*
		** check to see if we're swapping comparator, and if so
		** change cmp pointer.  tp can't point to comparator.
		*/
		if (hp == cmp)
			cmp = tp;
		SWAP(hp,tp);

		/*
		** Bump to avoid useless Compfunc calls.
		** Note that tp > hp at this point, and cmp is either
		** = tp, or hp didn't reach it at the top of the loop.
		** Either way cmp > hp at this point, so we can assure
		** cmp >= hp again at the top of the loop to guarantee
		** termination of the first while.
		*/
		hp += Eltsize;
		++hbump;
		--tdec;
		tp -= Eltsize;

		/*
		** save ourselves a comparison if crossed because of last swap.
		** if EQUAL, we must continue because hp must be bumped
		** if the element they both point to is < cmp
		*/
		if (hbump > tdec)
			break;
	}


	/*
	** See if we've had the bad luck to choose the smallest element
	** for our comparator.  If so, put it first, and sort the rest.
	** This is a needed special case because we fell out on the first
	** loop iteration after not bumping hp, and finding tp decremented
	** all the way back to arr.  We've legitimately made the >= piece
	** the entire array, in other words.
	*/
	if (hbump == 0)
	{
		SWAP(arr,cmp);
		do_qs(arr+Eltsize,nel-1);
		return;
	}

	/*
	** We've got non-zero length "<" and ">=" pieces.  Sort each.
	*/
	do_qs(arr,hbump);
	do_qs(hp,nel-hbump);
}

/* the bubble sort finisher - lengths <= 1 must cause no problems */
static VOID
bubble(char *arr,i4 nel)
{
	register i4  i;
	register char *ptr, *ptrnxt, *ptrlim;
	register bool swaps;

	for (i=(nel-1)*Eltsize; i > 0; i -= Eltsize)
	{
		swaps = FALSE;
		ptrlim = arr+i;
		for (ptr = arr; ptr != ptrlim; ptr = ptrnxt)
		{
			ptrnxt = ptr + Eltsize;
			if ((*Compfunc)(ptr,ptrnxt) > 0)
			{
				SWAP(ptr,ptrnxt);
				swaps = TRUE;
			}
		}
		if (! swaps)
			break;
	}
}


/*
+*
** Name:  	er__cmpids
**
** Description:
**	Compare the two message ids and return the following:
**		return <0 for id1 < id2
**		return 0 for id1 = id2
**		return >0 for id1 > id2
**
** Inputs:
**	char *id1	Message id to compare with id2
**	char *id2	Message id to compare with id1
**
** Exceptions:
**	none
**
** History:
**	1/22/88 (mhb)	Written
*/
static int
er__cmpids(const char *id1, const char *id2)
{
	return( ((MSGID_PAIR *)id1)->msgid - ((MSGID_PAIR *)id2)->msgid);
}

#ifndef __ADUPATEXEC_OPS_H_INC
#define _ADUPATEXEC_OPS_H_INC
/*
** adupatexec_ops.h - Generated from /home/me/lvnl/Ingres/src/common/adf/adu/adupatexec.dotty
**
** This include file defines the constants for the
** pattern matching state engine and its interface.
**
** The _DEFINEOP extra parameters are used by the
** pat_load raw interface for decoding the optional
** parameters and by adu_pat_disass() called by the
** pat_dump code.
*/

#define PAT_OPCODES \
_DEFINEOP(PAT_NOP             ,   0,     NOP, _, _, _, _, _)\
_DEFINEOP(PAT_CASE            ,   1,    CASE, _, _, C, _, _)\
_DEFINEOP(PAT_LABEL           ,   2,   LABEL, _, _, J, _, _)\
_DEFINEOP(PAT_JUMP            ,   3,    JUMP, _, _, J, _, _)\
_DEFINEOP(PAT_FOR_N_M         ,   4,     FOR, N, M, J, _, _)\
_DEFINEOP(PAT_NEXT            ,   5,    NEXT, _, _, J, _, _)\
_DEFINEOP(PAT_NEXT_W          ,   6,    NEXT, _, _, J, _, W)\
_DEFINEOP(PAT_COMMENT         ,   7, COMMENT, _, _, L, _, _)\
_DEFINEOP(PAT_FINAL           ,   8,   FINAL, _, _, _, _, _)\
_DEFINEOP(PAT_BUGCHK          ,   9,  BUGCHK, _, _, _, _, _)\
_DEFINEOP(PAT_MATCH           ,  10,   MATCH, _, _, _, _, _)\
_DEFINEOP(PAT_NOMATCH         ,  11, NOMATCH, _, _, _, _, _)\
_DEFINEOP(PAT_TRACE           ,  12,   TRACE, _, _, _, _, _)\
_DEFINEOP(PAT_NOTRACE         ,  13, NOTRACE, _, _, _, _, _)\
_DEFINEOP(PAT_BOS             ,  14,     BOS, _, _, _, _, _)\
_DEFINEOP(PAT_EOS             ,  15,     EOS, _, _, _, _, _)\
_DEFINEOP(PAT_BOW             ,  16,     BOW, _, _, _, _, _)\
_DEFINEOP(PAT_EOW             ,  17,     EOW, _, _, _, _, _)\
_DEFINEOP(PAT_BOM             ,  18,     BOM, _, _, _, _, _)\
_DEFINEOP(PAT_EOM             ,  19,     EOM, _, _, _, _, _)\
_DEFINEOP(PAT_BEGIN           ,  20,   BEGIN, _, _, _, _, _)\
_DEFINEOP(PAT_END             ,  21,     END, _, _, _, _, _)\
_DEFINEOP(PAT_LIT             ,  22,     LIT, _, _, L, _, _)\
_DEFINEOP(PAT_ANY_1           ,  23,     ANY, _, _, _, 1, _)\
_DEFINEOP(PAT_ANY_N           ,  24,     ANY, N, _, _, _, _)\
_DEFINEOP(PAT_ANY_0_W         ,  25,     ANY, _, _, _, 0, W)\
_DEFINEOP(PAT_ANY_1_W         ,  26,     ANY, _, _, _, 1, W)\
_DEFINEOP(PAT_ANY_N_W         ,  27,     ANY, N, _, _, _, W)\
_DEFINEOP(PAT_ANY_N_M         ,  28,     ANY, N, M, _, _, _)\
_DEFINEOP(PAT_SET_1           ,  29,     SET, _, _, S, 1, _)\
_DEFINEOP(PAT_SET_N           ,  30,     SET, N, _, S, _, _)\
_DEFINEOP(PAT_SET_0_W         ,  31,     SET, _, _, S, 0, W)\
_DEFINEOP(PAT_SET_1_W         ,  32,     SET, _, _, S, 1, W)\
_DEFINEOP(PAT_SET_N_W         ,  33,     SET, N, _, S, _, W)\
_DEFINEOP(PAT_SET_N_M         ,  34,     SET, N, M, S, _, _)\
_DEFINEOP(PAT_BSET_1          ,  35,    BSET, _, _, B, 1, _)\
_DEFINEOP(PAT_BSET_N          ,  36,    BSET, N, _, B, _, _)\
_DEFINEOP(PAT_BSET_0_W        ,  37,    BSET, _, _, B, 0, W)\
_DEFINEOP(PAT_BSET_1_W        ,  38,    BSET, _, _, B, 1, W)\
_DEFINEOP(PAT_BSET_N_W        ,  39,    BSET, N, _, B, _, W)\
_DEFINEOP(PAT_BSET_N_M        ,  40,    BSET, N, M, B, _, _)\
_DEFINEOP(PAT_NSET_1          ,  41,    NSET, _, _,NS, 1, _)\
_DEFINEOP(PAT_NSET_N          ,  42,    NSET, N, _,NS, _, _)\
_DEFINEOP(PAT_NSET_0_W        ,  43,    NSET, _, _,NS, 0, W)\
_DEFINEOP(PAT_NSET_1_W        ,  44,    NSET, _, _,NS, 1, W)\
_DEFINEOP(PAT_NSET_N_W        ,  45,    NSET, N, _,NS, _, W)\
_DEFINEOP(PAT_NSET_N_M        ,  46,    NSET, N, M,NS, _, _)\
_DEFINEOP(PAT_NBSET_1         ,  47,   NBSET, _, _,NB, 1, _)\
_DEFINEOP(PAT_NBSET_N         ,  48,   NBSET, N, _,NB, _, _)\
_DEFINEOP(PAT_NBSET_0_W       ,  49,   NBSET, _, _,NB, 0, W)\
_DEFINEOP(PAT_NBSET_1_W       ,  50,   NBSET, _, _,NB, 1, W)\
_DEFINEOP(PAT_NBSET_N_W       ,  51,   NBSET, N, _,NB, _, W)\
_DEFINEOP(PAT_NBSET_N_M       ,  52,   NBSET, N, M,NB, _, _)\
_DEFINEOP(PAT_FNDLIT          ,  53,  FNDLIT, _, _, L, _, _)\
_DEFINEOP(PAT_ENDLIT          ,  54,  ENDLIT, _, _, _, _, _)\
_DEFINEEX(PAT_LIT_ChAv        ,  55)\
_DEFINEEX(PAT_FNDLIT_ChAv     ,  56)\
_DEFINEEX(PAT_ENDLIT_EndChk   ,  57)\
_DEFINEEX(PAT_ENDLIT_ChAv     ,  58)\
_DEFINEEX(PAT_ANY_N_ChAv      ,  59)\
_DEFINEEX(PAT_ANY_0_W_ChAv    ,  60)\
_DEFINEEX(PAT_ANY_1_W_ChAv    ,  61)\
_DEFINEEX(PAT_ANY_N_W_ChAv    ,  62)\
_DEFINEEX(PAT_ANY_N_W_ChAv2   ,  63)\
_DEFINEEX(PAT_ANY_N_M_ChAv    ,  64)\
_DEFINEEX(PAT_ANY_N_M_ChAv2   ,  65)\
_DEFINEEX(PAT_SET_N_ChAv      ,  66)\
_DEFINEEX(PAT_SET_0_W_ChAv    ,  67)\
_DEFINEEX(PAT_SET_1_W_ChAv    ,  68)\
_DEFINEEX(PAT_SET_N_W_ChAv    ,  69)\
_DEFINEEX(PAT_SET_N_W_ChAv2   ,  70)\
_DEFINEEX(PAT_SET_N_M_ChAv    ,  71)\
_DEFINEEX(PAT_SET_N_M_ChAv2   ,  72)\
_DEFINEEX(PAT_BSET_N_ChAv     ,  73)\
_DEFINEEX(PAT_BSET_0_W_ChAv   ,  74)\
_DEFINEEX(PAT_BSET_1_W_ChAv   ,  75)\
_DEFINEEX(PAT_BSET_N_W_ChAv   ,  76)\
_DEFINEEX(PAT_BSET_N_W_ChAv2  ,  77)\
_DEFINEEX(PAT_BSET_N_M_ChAv   ,  78)\
_DEFINEEX(PAT_BSET_N_M_ChAv2  ,  79)\
_DEFINEEX(PAT_NSET_N_ChAv     ,  80)\
_DEFINEEX(PAT_NSET_0_W_ChAv   ,  81)\
_DEFINEEX(PAT_NSET_1_W_ChAv   ,  82)\
_DEFINEEX(PAT_NSET_N_W_ChAv   ,  83)\
_DEFINEEX(PAT_NSET_N_W_ChAv2  ,  84)\
_DEFINEEX(PAT_NSET_N_M_ChAv   ,  85)\
_DEFINEEX(PAT_NSET_N_M_ChAv2  ,  86)\
_DEFINEEX(PAT_NBSET_N_ChAv    ,  87)\
_DEFINEEX(PAT_NBSET_0_W_ChAv  ,  88)\
_DEFINEEX(PAT_NBSET_1_W_ChAv  ,  89)\
_DEFINEEX(PAT_NBSET_N_W_ChAv  ,  90)\
_DEFINEEX(PAT_NBSET_N_W_ChAv2 ,  91)\
_DEFINEEX(PAT_NBSET_N_M_ChAv  ,  92)\
_DEFINEEX(PAT_NBSET_N_M_ChAv2 ,  93)\
_ENDDEFINE

#define PAT_OPTYPES \
_DEFINE(FNDLIT    ,  0, 53)\
_DEFINE(NSET      ,  1, 41)\
_DEFINE(EOS       ,  2, 15)\
_DEFINE(NEXT      ,  3,  5)\
_DEFINE(ENDLIT    ,  4, 54)\
_DEFINE(BOW       ,  5, 16)\
_DEFINE(JUMP      ,  6,  3)\
_DEFINE(BEGIN     ,  7, 20)\
_DEFINE(NOTRACE   ,  8, 13)\
_DEFINE(TRACE     ,  9, 12)\
_DEFINE(NOP       , 10,  0)\
_DEFINE(LIT       , 11, 22)\
_DEFINE(EOW       , 12, 17)\
_DEFINE(FOR       , 13,  4)\
_DEFINE(BOM       , 14, 18)\
_DEFINE(NOMATCH   , 15, 11)\
_DEFINE(MATCH     , 16, 10)\
_DEFINE(LABEL     , 17,  2)\
_DEFINE(BSET      , 18, 35)\
_DEFINE(EOM       , 19, 19)\
_DEFINE(NBSET     , 20, 47)\
_DEFINE(SET       , 21, 29)\
_DEFINE(BUGCHK    , 22,  9)\
_DEFINE(CASE      , 23,  1)\
_DEFINE(END       , 24, 21)\
_DEFINE(COMMENT   , 25,  7)\
_DEFINE(ANY       , 26, 23)\
_DEFINE(BOS       , 27, 14)\
_DEFINE(FINAL     , 28,  8)\
_ENDDEFINE

#endif /*_ADUPATEXEC_OPS_H_INC*/

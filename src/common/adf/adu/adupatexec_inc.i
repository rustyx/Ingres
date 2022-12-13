/*
** adupatexec_inc.i - Generated from /home/me/lvnl/Ingres/src/common/adf/adu/adupatexec.dotty
**
** This include file implements the state machine for
** pattern matching.
*/

/**/_GetPatOP:
if (REG_PC >= ctx->patbufend) goto _PAT_FINAL;
DISASS(ctx)
ctx->state = *REG_PC++;
_restart_from_stall:
switch(ctx->state){

case PAT_FINAL:
/**/_PAT_FINAL:
	/*CHAV(_SUCCESS)*/
	ctx->state = PAT_FINAL;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _SUCCESS;
		goto _STALL;
	}
	goto _FAIL1;

case PAT_NOP:
	goto _GetPatOP;

case PAT_CASE:
	tmp = GETNUM(REG_PC);
	REG_STORE(ctx)
	FORKN(ctx, (u_i4)tmp);
	REG_SYNC(ctx)
	goto _GetPatOP;

case PAT_LABEL:
	tmp = GETNUM(REG_PC);
	goto _GetPatOP;

case PAT_JUMP:
	tmp = GETNUM(REG_PC);
	REG_PC += tmp;
	goto _GetPatOP;

case PAT_FOR_N_M:
	/*FRAMEPUSH*/
	ctx->sp++;
	/*Get args*/
	ctx->frame[ctx->sp].N = GETNUM(REG_PC);
	ctx->frame[ctx->sp].M = GETNUM(REG_PC);
	tmp = GETNUM(REG_PC);
	REG_PC += tmp;
	goto _GetPatOP;

case PAT_NEXT:
	/*Get arg*/
	tmp = GETNUM(REG_PC);
	/*Check N count expired*/
	if (ctx->frame[ctx->sp].N > 0){
	    /*Decr & do 1 iter*/
	    ctx->frame[ctx->sp].N--;
	    REG_PC += tmp;
	} else {
	    /*Check M count expired*/
	    if (ctx->frame[ctx->sp].M > 0){
	        /*Decr & do 1 iter FORKED*/
	        ctx->frame[ctx->sp].M--;
	        REG_STORE(ctx)
	        FORKB(ctx,tmp);
	        REG_SYNC(ctx)
	    }
	    /*FRAMEPOP & cont next instr*/
	    ctx->sp--;
	}
	goto _GetPatOP;

case PAT_NEXT_W:
	/*Get arg*/
	tmp = GETNUM(REG_PC);
	/*Check N count expired*/
	if (ctx->frame[ctx->sp].N > 0){
	    /*Decr & do 1 iter*/
	    ctx->frame[ctx->sp].N--;
	    REG_PC += tmp;
	} else {
	    REG_STORE(ctx)
	    FORKB(ctx,tmp);
	    REG_SYNC(ctx)
	    /*FRAMEPOP & cont next instr*/
	    ctx->sp--;
	}
	goto _GetPatOP;

case PAT_COMMENT:
	tmp = GETNUM(REG_PC);
	REG_PC += tmp;
	goto _GetPatOP;

case PAT_BUGCHK:
	actPAT_BUGCHK(ctx);
	goto _GetPatOP;

case PAT_MATCH:
	goto _SUCCESS;

case PAT_NOMATCH:
	goto _FAIL1;

case PAT_TRACE:
	ctx->parent->trace = ADU_pat_debug;
	goto _GetPatOP;

case PAT_NOTRACE:
	ctx->parent->trace = 0;
	goto _GetPatOP;

case PAT_BOS:
	/*PAT_BOS(_FAILm1)*/
	if (!(tstPAT_BOS(ctx))) goto _FAILm1;
	goto _GetPatOP;

case PAT_EOS:
	/*PAT_EOS(_FAILm1)*/
	if (!(tstPAT_EOS(ctx))) goto _FAILm1;
	goto _GetPatOP;

case PAT_BOW:
	/*PAT_BOW(_FAILm1)*/
	if (!(tstPAT_BOW(ctx))) goto _FAILm1;
	goto _GetPatOP;

case PAT_EOW:
	/*PAT_EOW(_FAILm1)*/
	if (!(tstPAT_EOW(ctx))) goto _FAILm1;
	goto _GetPatOP;

case PAT_BOM:
	actPAT_BOM(ctx);
	goto _GetPatOP;

case PAT_EOM:
	actPAT_EOM(ctx);
	goto _GetPatOP;

case PAT_BEGIN:
	/*PAT_BEGIN(_FAILm1)*/
	if (!(tstPAT_BEGIN(ctx))) goto _FAILm1;
	goto _GetPatOP;

case PAT_END:
	/*PAT_END(_FAILm1)*/
	if (!(tstPAT_END(ctx))) goto _FAILm1;
	goto _GetPatOP;

case PAT_LIT:
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
#ifdef LITPRIME
	LITPRIME
#endif
	/*EOFSEEN(_PAT_LIT_Loop)*/
#ifdef LITPRIME
	goto _PAT_LIT_Loop;
#endif
	if (!sea_ctx->at_eof)goto _PAT_LIT_Loop;
	/*PATROOM(_FAILm1)*/
	if (REG_CH+REG_L > REG_bufend)goto _FAILm1;
	/*CMPWHLPAT(_FAIL1)*/
#ifndef MECMP_LOOP
	if (MEcmp(REG_CH, REG_litset,REG_L))goto _FAIL1;
#else
	{
	    register CHAR *CHp = (CHAR*)REG_CH;
	    while (REG_litset<REG_litsetend){
		if (*CHp != *(CHAR*)REG_litset){
		    REG_litset = REG_litsetend-REG_L;
		    goto _FAIL1;
		}
		CHp++;
		REG_litset += sizeof(CHAR);
	    }
	}
#endif /*MECMP_LOOP*/
	REG_CH += REG_L;
	goto _GetPatOP;
/**/_PAT_LIT_Loop:
	/*MOREPAT(_GetPatOP)*/
	if (REG_litset >= REG_litsetend)goto _GetPatOP;

case PAT_LIT_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_LIT_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*DIFFPAT(_FAIL)*/
	DIFFPAT(_FAIL)
	NEXTPAT
	NEXTCH
	goto _PAT_LIT_Loop;

case PAT_FNDLIT:
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
#ifdef LITPRIME
	LITPRIME
#endif
	/*EOFSEEN(_PAT_FNDLIT_Loop)*/
#ifdef LITPRIME
	goto _PAT_FNDLIT_Loop;
#endif
	if (!sea_ctx->at_eof)goto _PAT_FNDLIT_Loop;
/**/_PAT_FNDLIT_F_Loop:
	/*PATROOM(_FAILm1)*/
	if (REG_CH+REG_L > REG_bufend)goto _FAILm1;
	/*CMPWHLPAT(_PAT_FNDLIT_F_Act1)*/
#ifndef MECMP_LOOP
	if (MEcmp(REG_CH, REG_litset,REG_L))goto _PAT_FNDLIT_F_Act1;
#else
	{
	    register CHAR *CHp = (CHAR*)REG_CH;
	    while (REG_litset<REG_litsetend){
		if (*CHp != *(CHAR*)REG_litset){
		    REG_litset = REG_litsetend-REG_L;
		    goto _PAT_FNDLIT_F_Act1;
		}
		CHp++;
		REG_litset += sizeof(CHAR);
	    }
	}
#endif /*MECMP_LOOP*/
	REG_CH += REG_L;
	goto _GetPatOP;
/**/_PAT_FNDLIT_F_Act1:
	NEXTCH
	goto _PAT_FNDLIT_F_Loop;
/**/_PAT_FNDLIT_Loop:
	/*MOREPAT(_GetPatOP)*/
	if (REG_litset >= REG_litsetend)goto _GetPatOP;

case PAT_FNDLIT_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_FNDLIT_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*DIFFPAT(_PAT_FNDLIT_NotYet)*/
	DIFFPAT(_PAT_FNDLIT_NotYet)
	NEXTPAT
	NEXTCH
	goto _PAT_FNDLIT_Loop;
/**/_PAT_FNDLIT_NotYet:
	{
	    register u_i1 *PATHOLD = REG_litset;
	    REG_litset = REG_litsetend - REG_L;
	    if (PATHOLD > REG_litset)
	    {
		register u_i1 *CHHOLD = REG_CH;
		register u_i1 *CHp = REG_litset;
#ifdef CHPRIME
		register u_i1 *REG_save;
		CHSAVE(hold)
		REG_CH = CHp;
		CHPRIME
		CHp = REG_CH;
		CHSAVE(pat)
#endif
		for(;;) {/* rescan points of pre-match */
#ifdef LITPRIME
		    LITPRIME
#endif
#ifdef CHPRIME
		    CHRESTORE(pat)
		    REG_save = REG_bufend;
		    REG_bufend = ctx->patbufend;
#endif
		    REG_CH = CHp;
		    NEXTCH
		    CHp = REG_CH;
#ifdef CHPRIME
		    REG_bufend = REG_save;
		    CHSAVE(pat)
#endif
		    for(;;) {
			if (REG_CH >= PATHOLD) {
			    REG_CH = CHHOLD;
#ifdef CHPRIME
			    CHRESTORE(hold)
#endif
			    goto _PAT_FNDLIT_NotYetloop;
			}
			DIFFPAT(_PAT_FNDLIT_NotYetexit)
			NEXTCH
			NEXTPAT
		    }
_PAT_FNDLIT_NotYetexit:
		    REG_litset = REG_litsetend - REG_L;
		}
	    } else {
#ifdef LITPRIME
		LITPRIME
#endif
		NEXTCH
	    }
	}
_PAT_FNDLIT_NotYetloop:
	goto _PAT_FNDLIT_Loop;

case PAT_ENDLIT:
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
#ifdef LITPRIME
	LITPRIME
#endif

case PAT_ENDLIT_EndChk:
	/*EOFCLOSE(_FAILm1)*/
	ctx->state = PAT_ENDLIT_EndChk;
#ifdef LITPRIME
	goto _FAILm1;
#endif
		/* If space too small fail */
	if (da_ctx->eof_offset - da_ctx->seg_offset <
		REG_CH - sea_ctx->buffer + REG_L)goto _FAILm1;
		/* If not in expected segment - stall */
	if (da_ctx->eof_offset - da_ctx->seg_offset >
		da_ctx->under_dv.db_length - sizeof(i2) + REG_L)goto _STALL;
	/*EOFSEEN(_PAT_ENDLIT_Act)*/
#ifdef LITPRIME
	goto _PAT_ENDLIT_Act;
#endif
	if (!sea_ctx->at_eof)goto _PAT_ENDLIT_Act;
	/* Position CH at where ENDLIT must start */
	REG_CH = da_ctx->eof_offset - da_ctx->seg_offset +
				sea_ctx->buffer - REG_L;
	/*CMPWHLPAT(_FAIL1)*/
#ifndef MECMP_LOOP
	if (MEcmp(REG_CH, REG_litset,REG_L))goto _FAIL1;
#else
	{
	    register CHAR *CHp = (CHAR*)REG_CH;
	    while (REG_litset<REG_litsetend){
		if (*CHp != *(CHAR*)REG_litset){
		    REG_litset = REG_litsetend-REG_L;
		    goto _FAIL1;
		}
		CHp++;
		REG_litset += sizeof(CHAR);
	    }
	}
#endif /*MECMP_LOOP*/
	goto _SUCCESS;
/**/_PAT_ENDLIT_Act:
	/* Position CH at where ENDLIT must start */
	REG_CH = da_ctx->eof_offset - da_ctx->seg_offset +
				sea_ctx->buffer - REG_L;
/**/_PAT_ENDLIT_Loop:
	/*MOREPAT(_GetPatOP)*/
	if (REG_litset >= REG_litsetend)goto _GetPatOP;

case PAT_ENDLIT_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_ENDLIT_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*DIFFPAT(_FAIL)*/
	DIFFPAT(_FAIL)
	NEXTPAT
	NEXTCH
	goto _PAT_ENDLIT_Loop;

case PAT_ANY_1:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_ANY_1;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	NEXTCH
	goto _GetPatOP;

case PAT_ANY_N:
	REG_N = GETNUM(REG_PC);
/**/_PAT_ANY_N_ChN:
	/*NGT0(_GetPatOP)*/
	if (REG_N <= 0) goto _GetPatOP;

case PAT_ANY_N_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_ANY_N_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	REG_N--;
	NEXTCH
	goto _PAT_ANY_N_ChN;

case PAT_ANY_0_W:
/**/_PAT_ANY_0_W:
	REG_STORE(ctx)
	FORK(ctx,PAT_ANY_0_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_ANY_0_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_ANY_0_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	NEXTCH
	goto _PAT_ANY_0_W;

case PAT_ANY_1_W:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_ANY_1_W;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	NEXTCH
/**/_PAT_ANY_1_W_Fork:
	REG_STORE(ctx)
	FORK(ctx,PAT_ANY_1_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_ANY_1_W_ChAv:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_ANY_1_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	NEXTCH
	goto _PAT_ANY_1_W_Fork;

case PAT_ANY_N_W:
	REG_N = GETNUM(REG_PC);
/**/_PAT_ANY_N_W_ChN:
	/*NGT0(_PAT_ANY_N_W_Fork)*/
	if (REG_N <= 0) goto _PAT_ANY_N_W_Fork;

case PAT_ANY_N_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_ANY_N_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	REG_N--;
	NEXTCH
	goto _PAT_ANY_N_W_ChN;
/**/_PAT_ANY_N_W_Fork:
	REG_STORE(ctx)
	FORK(ctx,PAT_ANY_N_W_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_ANY_N_W_ChAv2:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_ANY_N_W_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	NEXTCH
	goto _PAT_ANY_N_W_Fork;

case PAT_ANY_N_M:
	REG_N = GETNUM(REG_PC);
	REG_M = GETNUM(REG_PC);
/**/_PAT_ANY_N_M_ChN:
	/*NGT0(_PAT_ANY_N_M_ChM)*/
	if (REG_N <= 0) goto _PAT_ANY_N_M_ChM;

case PAT_ANY_N_M_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_ANY_N_M_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	REG_N--;
	NEXTCH
	goto _PAT_ANY_N_M_ChN;
/**/_PAT_ANY_N_M_ChM:
	/*MGT0(_GetPatOP)*/
	if (REG_M <= 0) goto _GetPatOP;
	REG_M--;
	REG_STORE(ctx)
	FORK(ctx,PAT_ANY_N_M_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_ANY_N_M_ChAv2:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_ANY_N_M_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	NEXTCH
	goto _PAT_ANY_N_M_ChM;

case PAT_SET_1:
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_litsetend = REG_PC;
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_SET_1;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINSET(_FAILm1)*/
#ifdef CHINSET
	CHINSET(_FAILm1)
#else
	if ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _FAILm1;
	for (; ; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _FAILm1;
	}
#endif /* CHINSET */
	NEXTCH
	goto _GetPatOP;

case PAT_SET_N:
	REG_N = GETNUM(REG_PC);
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_litsetend = REG_PC;
/**/_PAT_SET_N_ChN:
	/*NGT0(_GetPatOP)*/
	if (REG_N <= 0) goto _GetPatOP;

case PAT_SET_N_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_SET_N_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINSET(_FAILm1)*/
#ifdef CHINSET
	CHINSET(_FAILm1)
#else
	if ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _FAILm1;
	for (; ; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _FAILm1;
	}
#endif /* CHINSET */
	REG_N--;
	NEXTCH
	goto _PAT_SET_N_ChN;

case PAT_SET_0_W:
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_litsetend = REG_PC;
/**/_PAT_SET_0_W:
	REG_STORE(ctx)
	FORK(ctx,PAT_SET_0_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_SET_0_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_SET_0_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINSET(_FAILm1)*/
#ifdef CHINSET
	CHINSET(_FAILm1)
#else
	if ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _FAILm1;
	for (; ; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _FAILm1;
	}
#endif /* CHINSET */
	NEXTCH
	goto _PAT_SET_0_W;

case PAT_SET_1_W:
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_litsetend = REG_PC;
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_SET_1_W;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
/**/_PAT_SET_1_W_Mat:
	/*CHINSET(_FAILm1)*/
#ifdef CHINSET
	CHINSET(_FAILm1)
#else
	if ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _FAILm1;
	for (; ; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _FAILm1;
	}
#endif /* CHINSET */
	NEXTCH
	REG_STORE(ctx)
	FORK(ctx,PAT_SET_1_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_SET_1_W_ChAv:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_SET_1_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	goto _PAT_SET_1_W_Mat;

case PAT_SET_N_W:
	REG_N = GETNUM(REG_PC);
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_litsetend = REG_PC;
/**/_PAT_SET_N_W_ChN:
	/*NGT0(_PAT_SET_N_W_Fork)*/
	if (REG_N <= 0) goto _PAT_SET_N_W_Fork;

case PAT_SET_N_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_SET_N_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINSET(_FAILm1)*/
#ifdef CHINSET
	CHINSET(_FAILm1)
#else
	if ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _FAILm1;
	for (; ; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _FAILm1;
	}
#endif /* CHINSET */
	REG_N--;
	NEXTCH
	goto _PAT_SET_N_W_ChN;
/**/_PAT_SET_N_W_Fork:
	REG_STORE(ctx)
	FORK(ctx,PAT_SET_N_W_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_SET_N_W_ChAv2:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_SET_N_W_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	/*CHINSET(_FAILm1)*/
#ifdef CHINSET
	CHINSET(_FAILm1)
#else
	if ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _FAILm1;
	for (; ; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _FAILm1;
	}
#endif /* CHINSET */
	NEXTCH
	goto _PAT_SET_N_W_Fork;

case PAT_SET_N_M:
	REG_N = GETNUM(REG_PC);
	REG_M = GETNUM(REG_PC);
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_litsetend = REG_PC;
/**/_PAT_SET_N_M_ChN:
	/*NGT0(_PAT_SET_N_M_ChM)*/
	if (REG_N <= 0) goto _PAT_SET_N_M_ChM;

case PAT_SET_N_M_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_SET_N_M_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINSET(_FAILm1)*/
#ifdef CHINSET
	CHINSET(_FAILm1)
#else
	if ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _FAILm1;
	for (; ; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _FAILm1;
	}
#endif /* CHINSET */
	REG_N--;
	NEXTCH
	goto _PAT_SET_N_M_ChN;
/**/_PAT_SET_N_M_ChM:
	/*MGT0(_GetPatOP)*/
	if (REG_M <= 0) goto _GetPatOP;
	REG_M--;
	REG_STORE(ctx)
	FORK(ctx,PAT_SET_N_M_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_SET_N_M_ChAv2:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_SET_N_M_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINSET(_FAILm1)*/
#ifdef CHINSET
	CHINSET(_FAILm1)
#else
	if ((t = (CHAR*)REG_litset) >= (CHAR*)REG_litsetend) goto _FAILm1;
	for (; ; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)REG_litsetend) goto _FAILm1;
	}
#endif /* CHINSET */
	NEXTCH
	goto _PAT_SET_N_M_ChM;

case PAT_BSET_1:
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_BSET_1;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch>=8*REG_L ||
		!((1<<(ch&7))&REG_litset[ch>>3])) goto _FAILm1;
	NEXTCH
	goto _GetPatOP;

case PAT_BSET_N:
	REG_N = GETNUM(REG_PC);
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
/**/_PAT_BSET_N_ChN:
	/*NGT0(_GetPatOP)*/
	if (REG_N <= 0) goto _GetPatOP;

case PAT_BSET_N_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_BSET_N_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch>=8*REG_L ||
		!((1<<(ch&7))&REG_litset[ch>>3])) goto _FAILm1;
	REG_N--;
	NEXTCH
	goto _PAT_BSET_N_ChN;

case PAT_BSET_0_W:
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
/**/_PAT_BSET_0_W:
	REG_STORE(ctx)
	FORK(ctx,PAT_BSET_0_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_BSET_0_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_BSET_0_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch>=8*REG_L ||
		!((1<<(ch&7))&REG_litset[ch>>3])) goto _FAILm1;
	NEXTCH
	goto _PAT_BSET_0_W;

case PAT_BSET_1_W:
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_BSET_1_W;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
/**/_PAT_BSET_1_W_Mat:
	/*CHINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch>=8*REG_L ||
		!((1<<(ch&7))&REG_litset[ch>>3])) goto _FAILm1;
	NEXTCH
	REG_STORE(ctx)
	FORK(ctx,PAT_BSET_1_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_BSET_1_W_ChAv:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_BSET_1_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	goto _PAT_BSET_1_W_Mat;

case PAT_BSET_N_W:
	REG_N = GETNUM(REG_PC);
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
/**/_PAT_BSET_N_W_ChN:
	/*NGT0(_PAT_BSET_N_W_Fork)*/
	if (REG_N <= 0) goto _PAT_BSET_N_W_Fork;

case PAT_BSET_N_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_BSET_N_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch>=8*REG_L ||
		!((1<<(ch&7))&REG_litset[ch>>3])) goto _FAILm1;
	REG_N--;
	NEXTCH
	goto _PAT_BSET_N_W_ChN;
/**/_PAT_BSET_N_W_Fork:
	REG_STORE(ctx)
	FORK(ctx,PAT_BSET_N_W_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_BSET_N_W_ChAv2:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_BSET_N_W_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	/*CHINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch>=8*REG_L ||
		!((1<<(ch&7))&REG_litset[ch>>3])) goto _FAILm1;
	NEXTCH
	goto _PAT_BSET_N_W_Fork;

case PAT_BSET_N_M:
	REG_N = GETNUM(REG_PC);
	REG_M = GETNUM(REG_PC);
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
/**/_PAT_BSET_N_M_ChN:
	/*NGT0(_PAT_BSET_N_M_ChM)*/
	if (REG_N <= 0) goto _PAT_BSET_N_M_ChM;

case PAT_BSET_N_M_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_BSET_N_M_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch>=8*REG_L ||
		!((1<<(ch&7))&REG_litset[ch>>3])) goto _FAILm1;
	REG_N--;
	NEXTCH
	goto _PAT_BSET_N_M_ChN;
/**/_PAT_BSET_N_M_ChM:
	/*MGT0(_GetPatOP)*/
	if (REG_M <= 0) goto _GetPatOP;
	REG_M--;
	REG_STORE(ctx)
	FORK(ctx,PAT_BSET_N_M_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_BSET_N_M_ChAv2:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_BSET_N_M_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch>=8*REG_L ||
		!((1<<(ch&7))&REG_litset[ch>>3])) goto _FAILm1;
	NEXTCH
	goto _PAT_BSET_N_M_ChM;

case PAT_NSET_1:
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_O = GETNUM(REG_litset);
	REG_litsetend = REG_PC;
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NSET_1;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINSET(_FAILm1)*/
#ifdef CHNINSET
	CHNINSET(_FAILm1)
#else
	for (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _FAILm1;
	}
	for (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) goto _FAILm1;
		}
	    }else if (!COMPARECH(t)) goto _FAILm1;
	    if (last_diff < 0) break;
	}
#endif /* CHNINSET */
	NEXTCH
	goto _GetPatOP;

case PAT_NSET_N:
	REG_N = GETNUM(REG_PC);
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_O = GETNUM(REG_litset);
	REG_litsetend = REG_PC;
/**/_PAT_NSET_N_ChN:
	/*NGT0(_GetPatOP)*/
	if (REG_N <= 0) goto _GetPatOP;

case PAT_NSET_N_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NSET_N_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINSET(_FAILm1)*/
#ifdef CHNINSET
	CHNINSET(_FAILm1)
#else
	for (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _FAILm1;
	}
	for (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) goto _FAILm1;
		}
	    }else if (!COMPARECH(t)) goto _FAILm1;
	    if (last_diff < 0) break;
	}
#endif /* CHNINSET */
	REG_N--;
	NEXTCH
	goto _PAT_NSET_N_ChN;

case PAT_NSET_0_W:
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_O = GETNUM(REG_litset);
	REG_litsetend = REG_PC;
/**/_PAT_NSET_0_W:
	REG_STORE(ctx)
	FORK(ctx,PAT_NSET_0_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_NSET_0_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NSET_0_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINSET(_FAILm1)*/
#ifdef CHNINSET
	CHNINSET(_FAILm1)
#else
	for (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _FAILm1;
	}
	for (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) goto _FAILm1;
		}
	    }else if (!COMPARECH(t)) goto _FAILm1;
	    if (last_diff < 0) break;
	}
#endif /* CHNINSET */
	NEXTCH
	goto _PAT_NSET_0_W;

case PAT_NSET_1_W:
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_O = GETNUM(REG_litset);
	REG_litsetend = REG_PC;
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NSET_1_W;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
/**/_PAT_NSET_1_W_Mat:
	/*CHNINSET(_FAILm1)*/
#ifdef CHNINSET
	CHNINSET(_FAILm1)
#else
	for (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _FAILm1;
	}
	for (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) goto _FAILm1;
		}
	    }else if (!COMPARECH(t)) goto _FAILm1;
	    if (last_diff < 0) break;
	}
#endif /* CHNINSET */
	NEXTCH
	REG_STORE(ctx)
	FORK(ctx,PAT_NSET_1_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_NSET_1_W_ChAv:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_NSET_1_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	goto _PAT_NSET_1_W_Mat;

case PAT_NSET_N_W:
	REG_N = GETNUM(REG_PC);
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_O = GETNUM(REG_litset);
	REG_litsetend = REG_PC;
/**/_PAT_NSET_N_W_ChN:
	/*NGT0(_PAT_NSET_N_W_Fork)*/
	if (REG_N <= 0) goto _PAT_NSET_N_W_Fork;

case PAT_NSET_N_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NSET_N_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINSET(_FAILm1)*/
#ifdef CHNINSET
	CHNINSET(_FAILm1)
#else
	for (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _FAILm1;
	}
	for (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) goto _FAILm1;
		}
	    }else if (!COMPARECH(t)) goto _FAILm1;
	    if (last_diff < 0) break;
	}
#endif /* CHNINSET */
	REG_N--;
	NEXTCH
	goto _PAT_NSET_N_W_ChN;
/**/_PAT_NSET_N_W_Fork:
	REG_STORE(ctx)
	FORK(ctx,PAT_NSET_N_W_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_NSET_N_W_ChAv2:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_NSET_N_W_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	/*CHNINSET(_FAILm1)*/
#ifdef CHNINSET
	CHNINSET(_FAILm1)
#else
	for (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _FAILm1;
	}
	for (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) goto _FAILm1;
		}
	    }else if (!COMPARECH(t)) goto _FAILm1;
	    if (last_diff < 0) break;
	}
#endif /* CHNINSET */
	NEXTCH
	goto _PAT_NSET_N_W_Fork;

case PAT_NSET_N_M:
	REG_N = GETNUM(REG_PC);
	REG_M = GETNUM(REG_PC);
	tmp = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += tmp;
	REG_O = GETNUM(REG_litset);
	REG_litsetend = REG_PC;
/**/_PAT_NSET_N_M_ChN:
	/*NGT0(_PAT_NSET_N_M_ChM)*/
	if (REG_N <= 0) goto _PAT_NSET_N_M_ChM;

case PAT_NSET_N_M_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NSET_N_M_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINSET(_FAILm1)*/
#ifdef CHNINSET
	CHNINSET(_FAILm1)
#else
	for (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _FAILm1;
	}
	for (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) goto _FAILm1;
		}
	    }else if (!COMPARECH(t)) goto _FAILm1;
	    if (last_diff < 0) break;
	}
#endif /* CHNINSET */
	REG_N--;
	NEXTCH
	goto _PAT_NSET_N_M_ChN;
/**/_PAT_NSET_N_M_ChM:
	/*MGT0(_GetPatOP)*/
	if (REG_M <= 0) goto _GetPatOP;
	REG_M--;
	REG_STORE(ctx)
	FORK(ctx,PAT_NSET_N_M_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_NSET_N_M_ChAv2:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NSET_N_M_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINSET(_FAILm1)*/
#ifdef CHNINSET
	CHNINSET(_FAILm1)
#else
	for (t = (CHAR*)REG_litset; t < (CHAR*)(REG_litset+REG_O); ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) break;
		}
	    }else if (!COMPARECH(t)) break;
	    if (last_diff < 0 || t >= (CHAR*)(REG_litset+REG_O)) goto _FAILm1;
	}
	for (t = (CHAR*)(REG_litset+REG_O); t < (CHAR*)REG_litsetend; ADV(t)){
	    if (HAVERANGE(t)){
		if(COMPARECH(t)<0){
		    ADV(t);
		}else{
		    ADV(t);
		    if(COMPARECH(t)<=0) goto _FAILm1;
		}
	    }else if (!COMPARECH(t)) goto _FAILm1;
	    if (last_diff < 0) break;
	}
#endif /* CHNINSET */
	NEXTCH
	goto _PAT_NSET_N_M_ChM;

case PAT_NBSET_1:
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NBSET_1;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch<8*REG_L &&
		(1<<(ch&7))&REG_litset[ch>>3]) goto _FAILm1;
	NEXTCH
	goto _GetPatOP;

case PAT_NBSET_N:
	REG_N = GETNUM(REG_PC);
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
/**/_PAT_NBSET_N_ChN:
	/*NGT0(_GetPatOP)*/
	if (REG_N <= 0) goto _GetPatOP;

case PAT_NBSET_N_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NBSET_N_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch<8*REG_L &&
		(1<<(ch&7))&REG_litset[ch>>3]) goto _FAILm1;
	REG_N--;
	NEXTCH
	goto _PAT_NBSET_N_ChN;

case PAT_NBSET_0_W:
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
/**/_PAT_NBSET_0_W:
	REG_STORE(ctx)
	FORK(ctx,PAT_NBSET_0_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_NBSET_0_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NBSET_0_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch<8*REG_L &&
		(1<<(ch&7))&REG_litset[ch>>3]) goto _FAILm1;
	NEXTCH
	goto _PAT_NBSET_0_W;

case PAT_NBSET_1_W:
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NBSET_1_W;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
/**/_PAT_NBSET_1_W_Mat:
	/*CHNINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch<8*REG_L &&
		(1<<(ch&7))&REG_litset[ch>>3]) goto _FAILm1;
	NEXTCH
	REG_STORE(ctx)
	FORK(ctx,PAT_NBSET_1_W_ChAv);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_NBSET_1_W_ChAv:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_NBSET_1_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	goto _PAT_NBSET_1_W_Mat;

case PAT_NBSET_N_W:
	REG_N = GETNUM(REG_PC);
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
/**/_PAT_NBSET_N_W_ChN:
	/*NGT0(_PAT_NBSET_N_W_Fork)*/
	if (REG_N <= 0) goto _PAT_NBSET_N_W_Fork;

case PAT_NBSET_N_W_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NBSET_N_W_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch<8*REG_L &&
		(1<<(ch&7))&REG_litset[ch>>3]) goto _FAILm1;
	REG_N--;
	NEXTCH
	goto _PAT_NBSET_N_W_ChN;
/**/_PAT_NBSET_N_W_Fork:
	REG_STORE(ctx)
	FORK(ctx,PAT_NBSET_N_W_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_NBSET_N_W_ChAv2:
	/*CHAV(_GetPatOP)*/
	ctx->state = PAT_NBSET_N_W_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _GetPatOP;
		goto _STALL;
	}
	/*CHNINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch<8*REG_L &&
		(1<<(ch&7))&REG_litset[ch>>3]) goto _FAILm1;
	NEXTCH
	goto _PAT_NBSET_N_W_Fork;

case PAT_NBSET_N_M:
	REG_N = GETNUM(REG_PC);
	REG_M = GETNUM(REG_PC);
	REG_O = GETNUM(REG_PC);
	REG_L = GETNUM(REG_PC);
	REG_litset = REG_PC;
	REG_PC += REG_L;
	REG_litsetend = REG_PC;
/**/_PAT_NBSET_N_M_ChN:
	/*NGT0(_PAT_NBSET_N_M_ChM)*/
	if (REG_N <= 0) goto _PAT_NBSET_N_M_ChM;

case PAT_NBSET_N_M_ChAv:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NBSET_N_M_ChAv;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch<8*REG_L &&
		(1<<(ch&7))&REG_litset[ch>>3]) goto _FAILm1;
	REG_N--;
	NEXTCH
	goto _PAT_NBSET_N_M_ChN;
/**/_PAT_NBSET_N_M_ChM:
	/*MGT0(_GetPatOP)*/
	if (REG_M <= 0) goto _GetPatOP;
	REG_M--;
/**/_PAT_NBSET_N_M_Fork:
	REG_STORE(ctx)
	FORK(ctx,PAT_NBSET_N_M_ChAv2);
	/* REG_SYNC(ctx) */
	goto _GetPatOP;

case PAT_NBSET_N_M_ChAv2:
	/*CHAV(_FAILm1)*/
	ctx->state = PAT_NBSET_N_M_ChAv2;
	if (REG_CH >= REG_bufend){
		if (ctx->parent->at_eof) goto _FAILm1;
		goto _STALL;
	}
	/*CHNINBSET(_FAILm1)*/
	ch = *(CHAR*)REG_CH - REG_O;
	if ((CHAR)ch<8*REG_L &&
		(1<<(ch&7))&REG_litset[ch>>3]) goto _FAILm1;
	NEXTCH
	goto _PAT_NBSET_N_M_Fork;
}

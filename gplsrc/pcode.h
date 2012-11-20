/* PCODE.H
 * Pcode definitions.
 * Copyright (c) 2007 Ladybridge Systems, All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 * Ladybridge Systems can be contacted via the www.openqm.com web site.
 * 
 * START-HISTORY:
 * 04 Mar 07  2.5-1 Added EXTENDLIST.
 * 17 Mar 06  2.3-8 Added IN.
 * 22 Sep 05  2.2-12 Added CHAIN.
 * 01 Sep 05  2.2-9 Added MESSAGE and GETMSG.
 * 24 Aug 05  2.2-8 Added DFLTDATE.
 * 19 Jul 05  2.2-4 Added PREFIX.
 * 28 Jun 05  2.2-1 Added journalling routines.
 * 08 Nov 04  2.0-10 Removed IFS pcode.
 * 20 Sep 04  2.0-2 New module.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

Pcode(ak)        /* AK(MODE, MAT AK.DATA, ID, OLD.REC, NEW.REC) */
Pcode(banner)    /* BANNER(UNIT, TEXT) */
Pcode(bindkey)   /* BINDKEY(STRING, ACTION) */
Pcode(break)     /* BREAK() */
Pcode(cconv)     /* CCONV(SRC, CONV) */
Pcode(chain)     /* CHAIN() */
Pcode(data)      /* DATA(STRING) */
Pcode(dellist)   /* DELLIST(NAME) */
Pcode(extendlist) /* EXTENDLIST(ITEMS, LIST.NO) */
Pcode(fold)      /* FOLD(STRING, WIDTH) */
Pcode(formcsv)   /* FORMCSV(STR) */
Pcode(formlst)   /* FORMLIST(SRC, LIST.NO) */
Pcode(getlist)   /* DELLIST(NAME, LIST.NO) */
Pcode(getmsg)    /* GETMSG() */
Pcode(hf)        /* HF(PU, PGNO, HF.IN) */
Pcode(iconv)     /* ICONV(SRC, CONV) */
Pcode(in)        /* IN(TIMEOUT) */
Pcode(indices)   /* INDICES(MAT AK.DATA, AKNO) */
Pcode(input)     /* INPUT(STRING, MAX.LENGTH, FLAGS) */
Pcode(inputat)   /* INPUTAT(X, Y, STRING, MAX.LENGTH, MASK, FLAGS) */
Pcode(itype)     /* ITYPE(DICT.REC) */
Pcode(keycode)   /* KEYCODE(TIMEOUT) */
Pcode(keyedit)   /* KEYEDIT(KEY.CODE, KEY.STRING) */
Pcode(login)     /* LOGIN(USERNAME, PASSWORD) */
Pcode(maximum)   /* MAXIMUM(DYN.ARRAY) */
Pcode(message)   /* MESSAGE() */
Pcode(minimum)   /* MINIMUM(DYN.ARRAY) */
Pcode(msgargs)   /* MSGARGS(TEXT,ARG1,ARG2,ARG3,ARG4) */
Pcode(nextptr)   /* NEXTPTR() */
Pcode(oconv)     /* OCONV(SRC, CONV) */
Pcode(ojoin)     /* OJOIN(FILE.NAME, INDEX.NAME, INDEXED.VALUE) */
Pcode(overlay)   /* OVERLAY(PU) */
Pcode(pclstart)  /* PCL.START(PU) */
Pcode(pickmsg)   /* PICKMSG(ID, ARGS, IS.ABORT) */
Pcode(prefix)    /* PREFIX(UNIT, PATHNAME) */
Pcode(prfile)    /* PRFILE(FILE, RECORD, PATHNAME, STATUS.CODE) */
Pcode(readlst)   /* READLIST(TGT, LIST.NO, STATUS.CODE) */
Pcode(readv)     /* READV(TGT, FILE, ID, FIELD.NO, LOCK) */
Pcode(repadd)    /* REPADD(DYN.ARRAY, F, V, S, VAL) */
Pcode(repcat)    /* REPCAT(DYN.ARRAY, F, V, S, VAL) */
Pcode(repdiv)    /* REPDIV(DYN.ARRAY, F, V, S, VAL) */
Pcode(repmul)    /* REPMUL(DYN.ARRAY, F, V, S, VAL) */
Pcode(repsub)    /* REPSUB(DYN.ARRAY, F, V, S, VAL) */
Pcode(repsubst)  /* REPSUBST(DYN.ARRAY, F, V, S, P, Q, VAL) */
Pcode(savelst)   /* SAVELIST(NAME, LIST.NO) */
Pcode(sselct)    /* SSELECT(FVAR, LIST.NO) */
Pcode(subst)     /* SUBSTITUTE(STR, OLD.LIST, NEW.LIST, DELIMITER) */
Pcode(substrn)   /* SUBSTRNG(STR, START, LENGTH) */
Pcode(sum)       /* SUM(STR) */
Pcode(sumall)    /* SUMMATION(STR) */
Pcode(system)    /* SYSTEM(ARG) */
Pcode(tconv)     /* TCONV(SRC, CONV, OCONV) */
Pcode(trans)     /* TRANS(FILE.NAME, ID.LIST, FIELD.NO, CODE) */
Pcode(ttyget)    /* TTYGET() */
Pcode(ttyset)    /* TTYSET(MODES) */
Pcode(voc_cat)   /* VOC.CAT(VOC.ID, PATHNAME) */
Pcode(voc_ref)   /* VOC.REF(VOC.ID, FIELD.NO, RESULT) */
Pcode(writev)    /* WRITEV(STRING, FILE, ID, FIELD.NO, LOCK) */

/* END-CODE */

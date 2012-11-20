/* OP_STR3.C
 * String handling opcodes
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
 * 15 Aug 07  2.6-0 Reworked remove pointers.
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 18 Apr 06  2.4-1 0475 Operators names (EG, MATCHES, etc) should be case
 *                  insensitive.
 * 15 Mar 06  2.3-8 0465 A<B><C treated >< as an operator.
 * 15 Apr 05  2.1-12 0344 Modified look-ahead after underscore to work when at
 *                   end of string chunk.
 * 07 Dec 04  2.1-0 Added { and } handling.
 * 20 Sep 04  2.0-2 Use message handler.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *    op_rmv_tkn     RMVTKN
 *       Token codes:
 *         0  TKN_END         End of string
 *         1  TKN_NAME        Name token
 *         2  TKN_NUM         Number
 *         3  TKN_LABEL       Label   (Note alpha label is parsed as name
 *                                     possibly with following colon)
 *         4  TKN_STRING      String constant
 *         5  TKN_LSQBR       [
 *         6  TKN_RSQBR       ]
 *         7  TKN_LBR         (
 *         8  TKN_RBR         )
 *         9  TKN_FIELD_REF   Pseudo operator for field reference
 *        10  TKN_COMMA       Comma may be used in field reference
 *        11  TKN_FMT         Pseudo operator for SMA format (not used here)
 * --- Start of operator group - Must be contiguous. See compiler.
 *        12  TKN_LT          <
 *        13  TKN_LTX         LT
 *        14  TKN_GT          >
 *        15  TKN_GTX         GT
 *        16  TKN_EQ          =     EQ
 *        17  TKN_NE          #     <>     NE
 *        18  TKN_NEX         ><
 *        19  TKN_LE          <=    =<     LE
 *        20  TKN_GE          >=
 *        21  TKN_GEX         =>     GE
 *        22  TKN_AND         &     AND
 *        23  TKN_OR          !     OR
 *        24  TKN_PLUS        +
 *        25  TKN_MINUS       -
 *        26  TKN_DIV         /
 *        27  TKN_IDIV        //
 *        28  TKN_MULT        *
 *        29  TKN_PWR         **    ^
 *        30  TKN_COLON       :     CAT
 *        31  TKN_MATCHES     MATCHES MATCH
 * --- End of operator group
 *        32  TKN_ADDEQ       +=
 *        33  TKN_SUBEQ       -=
 *        34  TKN_SEMICOLON   ;
 *        35  TKN_NAME_LBR    Name immediately followed by left bracket
 *        36  TKN_CATEQ       :=
 *        37  TKN_DOLLAR      $
 *        38  TKN_AT          @
 *        39  TKN_UNDERSCORE
 *        40  TKN_MULTEQ      *=
 *        41  TKN_DIVEQ       /=
 *        42  TKN_FLOAT
 *        43  TKN_AT_NAME     @ followed by letter with no intervening space
 *        44  TKN_LCBR        {
 *        45  TKN_RCBR        }
 *        46  TKN_HEXNUM      Hexadecimal number (0xnnn)
 *        47  TKN_OBJREF      -> (Object reference)
 *        62  TKN_UNCLOSED    Unclosed string
 *        63  TKN_UNKNOWN     Unrecognised token
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"

#define TKN_END         0
#define TKN_NAME        1
#define TKN_NUM         2
#define TKN_LABEL       3
#define TKN_STRING      4
#define TKN_LSQBR       5
#define TKN_RSQBR       6
#define TKN_LBR         7
#define TKN_RBR         8
#define TKN_FIELD_REF   9
#define TKN_COMMA      10
#define TKN_FMT        11
#define TKN_LT         12
#define TKN_LTX        13
#define TKN_GT         14
#define TKN_GTX        15
#define TKN_EQ         16
#define TKN_NE         17
#define TKN_NEX        18
#define TKN_LE         19
#define TKN_GE         20
#define TKN_GEX        21
#define TKN_AND        22
#define TKN_OR         23
#define TKN_PLUS       24
#define TKN_MINUS      25
#define TKN_DIV        26
#define TKN_IDIV       27
#define TKN_MULT       28
#define TKN_PWR        29
#define TKN_COLON      30
#define TKN_MATCHES    31
#define TKN_ADDEQ      32
#define TKN_SUBEQ      33
#define TKN_SEMICOLON  34
#define TKN_NAME_LBR   35
#define TKN_CATEQ      36
#define TKN_DOLLAR     37
#define TKN_AT         38
#define TKN_UNDERSCORE 39
#define TKN_MULTEQ     40
#define TKN_DIVEQ      41
#define TKN_FLOAT      42
#define TKN_AT_NAME    43
#define TKN_LCBR       44
#define TKN_RCBR       45
#define TKN_HEXNUM     46
#define TKN_OBJREF     47

#define TKN_UNCLOSED   62
#define TKN_UNKNOWN    63

#define COPY_CHAR  0x80
#define DONE_AFTER 0x40
#define TOKEN_MASK 0x3F
Private u_char token_table[96] = {
TKN_UNKNOWN   | 0         | 0             ,  /* Space */
TKN_OR        | COPY_CHAR | DONE_AFTER    ,  /* ! */
TKN_STRING    | 0         | 0             ,  /* " */
TKN_NE        | COPY_CHAR | DONE_AFTER    ,  /* # */
TKN_DOLLAR    | COPY_CHAR | DONE_AFTER    ,  /* $ */
TKN_UNKNOWN   | 0         | DONE_AFTER    ,  /* % */
TKN_AND       | COPY_CHAR | DONE_AFTER    ,  /* & */
TKN_STRING    | 0         | 0             ,  /* ' */
TKN_LBR       | COPY_CHAR | DONE_AFTER    ,  /* ( */
TKN_RBR       | COPY_CHAR | DONE_AFTER    ,  /* ) */
TKN_MULT      | COPY_CHAR | 0             ,  /* * */
TKN_PLUS      | COPY_CHAR | 0             ,  /* + */
TKN_COMMA     | COPY_CHAR | DONE_AFTER    ,  /* , */
TKN_MINUS     | COPY_CHAR | 0             ,  /* - */
TKN_FLOAT     | COPY_CHAR | 0             ,  /* . */
TKN_DIV       | COPY_CHAR | 0             ,  /* / */
TKN_NUM       | COPY_CHAR | 0             ,  /* 0 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 1 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 2 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 3 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 4 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 5 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 6 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 7 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 8 */
TKN_NUM       | COPY_CHAR | 0             ,  /* 9 */
TKN_COLON     | COPY_CHAR | 0             ,  /* : */
TKN_SEMICOLON | COPY_CHAR | DONE_AFTER    ,  /* ; */
TKN_LT        | COPY_CHAR | 0             ,  /* < */
TKN_EQ        | COPY_CHAR | 0             ,  /* = */
TKN_GT        | COPY_CHAR | 0             ,  /* > */
TKN_UNKNOWN   | 0         | DONE_AFTER    ,  /* ? */
TKN_AT        | COPY_CHAR | 0             ,  /* @ */
TKN_NAME      | COPY_CHAR | 0             ,  /* A */
TKN_NAME      | COPY_CHAR | 0             ,  /* B */
TKN_NAME      | COPY_CHAR | 0             ,  /* C */
TKN_NAME      | COPY_CHAR | 0             ,  /* D */
TKN_NAME      | COPY_CHAR | 0             ,  /* E */
TKN_NAME      | COPY_CHAR | 0             ,  /* F */
TKN_NAME      | COPY_CHAR | 0             ,  /* G */
TKN_NAME      | COPY_CHAR | 0             ,  /* H */
TKN_NAME      | COPY_CHAR | 0             ,  /* I */
TKN_NAME      | COPY_CHAR | 0             ,  /* J */
TKN_NAME      | COPY_CHAR | 0             ,  /* K */
TKN_NAME      | COPY_CHAR | 0             ,  /* L */
TKN_NAME      | COPY_CHAR | 0             ,  /* M */
TKN_NAME      | COPY_CHAR | 0             ,  /* N */
TKN_NAME      | COPY_CHAR | 0             ,  /* O */
TKN_NAME      | COPY_CHAR | 0             ,  /* P */
TKN_NAME      | COPY_CHAR | 0             ,  /* Q */
TKN_NAME      | COPY_CHAR | 0             ,  /* R */
TKN_NAME      | COPY_CHAR | 0             ,  /* S */
TKN_NAME      | COPY_CHAR | 0             ,  /* T */
TKN_NAME      | COPY_CHAR | 0             ,  /* U */
TKN_NAME      | COPY_CHAR | 0             ,  /* V */
TKN_NAME      | COPY_CHAR | 0             ,  /* W */
TKN_NAME      | COPY_CHAR | 0             ,  /* X */
TKN_NAME      | COPY_CHAR | 0             ,  /* Y */
TKN_NAME      | COPY_CHAR | 0             ,  /* Z */
TKN_LSQBR     | COPY_CHAR | DONE_AFTER    ,  /* [ */
TKN_STRING    | 0         | 0             ,  /* \ */
TKN_RSQBR     | COPY_CHAR | DONE_AFTER    ,  /* ] */
TKN_PWR       | COPY_CHAR | DONE_AFTER    ,  /* ^ */
TKN_UNDERSCORE| COPY_CHAR | DONE_AFTER    ,  /* _ */
TKN_UNKNOWN   | 0         | DONE_AFTER    ,  /* ` */
TKN_NAME      | COPY_CHAR | 0             ,  /* a */
TKN_NAME      | COPY_CHAR | 0             ,  /* b */
TKN_NAME      | COPY_CHAR | 0             ,  /* c */
TKN_NAME      | COPY_CHAR | 0             ,  /* d */
TKN_NAME      | COPY_CHAR | 0             ,  /* e */
TKN_NAME      | COPY_CHAR | 0             ,  /* f */
TKN_NAME      | COPY_CHAR | 0             ,  /* g */
TKN_NAME      | COPY_CHAR | 0             ,  /* h */
TKN_NAME      | COPY_CHAR | 0             ,  /* i */
TKN_NAME      | COPY_CHAR | 0             ,  /* j */
TKN_NAME      | COPY_CHAR | 0             ,  /* k */
TKN_NAME      | COPY_CHAR | 0             ,  /* l */
TKN_NAME      | COPY_CHAR | 0             ,  /* m */
TKN_NAME      | COPY_CHAR | 0             ,  /* n */
TKN_NAME      | COPY_CHAR | 0             ,  /* o */
TKN_NAME      | COPY_CHAR | 0             ,  /* p */
TKN_NAME      | COPY_CHAR | 0             ,  /* q */
TKN_NAME      | COPY_CHAR | 0             ,  /* r */
TKN_NAME      | COPY_CHAR | 0             ,  /* s */
TKN_NAME      | COPY_CHAR | 0             ,  /* t */
TKN_NAME      | COPY_CHAR | 0             ,  /* u */
TKN_NAME      | COPY_CHAR | 0             ,  /* v */
TKN_NAME      | COPY_CHAR | 0             ,  /* w */
TKN_NAME      | COPY_CHAR | 0             ,  /* x */
TKN_NAME      | COPY_CHAR | 0             ,  /* y */
TKN_NAME      | COPY_CHAR | 0             ,  /* z */
TKN_LCBR      | COPY_CHAR | DONE_AFTER    ,  /* { */
TKN_UNKNOWN   | 0         | DONE_AFTER    ,  /* | */
TKN_RCBR      | COPY_CHAR | DONE_AFTER    ,  /* } */
TKN_UNKNOWN   | 0         | DONE_AFTER    ,  /* ~ */
TKN_UNKNOWN   | 0         | DONE_AFTER       /* Del */
};

Private char * opname_strings[] = {
"CAT", "LT", "GT", "EQ", "NE", "LE", "GE", "AND", "OR", "MATCH", "MATCHES"};
Private short int opname_codes[] = {
TKN_COLON, TKN_LTX, TKN_GTX, TKN_EQ, TKN_NE, TKN_LE, TKN_GEX, TKN_AND, TKN_OR,
TKN_MATCHES, TKN_MATCHES};
#define NUM_OPCODE_NAMES 11
#define LONGEST_OPNAME 7

Private char label_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.%$";
bool case_sensitive = FALSE;   /* Set by op_kernel() */

/* ======================================================================
   op_rmvtkn()  -  Remove token substring from dynamic array              */

void op_rmvtkn()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  ADDR to variable to        | Substring                   |
     |  receive token type code    |                             |
     |-----------------------------|-----------------------------| 
     |  ADDR to source string      |                             |
     |=============================|=============================|
 */

 DESCRIPTOR * type_descr;       /* Descriptor to receive token type code */
 DESCRIPTOR * src_descr;        /* Ptr to source string descriptor */
 short int offset;              /* Offset into current source chunk */
 STRING_CHUNK * str_hdr;
 short int bytes_remaining;
 STRING_CHUNK * tgt_str;
 STRING_CHUNK * new_tgt;
 short int tgt_bytes_remaining;
 char * tgt;
 short int token_type;
 char * p;
 char c;
 char x;
 long int bytes;
 bool done;              /* End of token - current char is next token */
 bool done_after;        /* End of token - current char part of this token */
 char delimiter;         /* String constant delimiter */
 bool copy_char;
 short int j;
 long int alloc_size = 32;  /* Size to request from s_alloc() */
 short int actual_size;     /* Actual size allocated */

 /* Follow ADDR chain to leave delim_descr pointing to descriptor to
    receive the token type code.                                       */

 type_descr = e_stack - 1;
 do {
     type_descr = type_descr->data.d_addr;
    } while(type_descr->type == ADDR);
 k_release(type_descr);

 InitDescr(type_descr, INTEGER);
 type_descr->data.value = TKN_END;     /* For end of string paths */

 /* Get source string. For this opcode, we must have a pointer to the actual
    string descriptor rather than a copy of it as we may need to set up the
    remove pointer. The source descriptor is always an ADDR; literals and
    expressions are not allowed with RMVTKN.                                 */
 
 src_descr = e_stack - 2;
 do {src_descr = src_descr->data.d_addr;} while(src_descr->type == ADDR);

 k_pop(2);   /* Dismiss both ADDRs */

 /* Ensure that it is a string */
 if (src_descr->type != STRING) k_get_string(src_descr);

 /* Set up a string descriptor to receive the result */

 InitDescr(e_stack, STRING);
 e_stack->data.str.saddr = NULL;

 if (src_descr->data.str.saddr == NULL) goto exit_rmvtkn;  /* Null source */


 if ((src_descr->flags & DF_REMOVE)) /* Remove pointer exists */
  {
   offset = src_descr->n1;
   str_hdr = src_descr->data.str.rmv_saddr;
  }
 else /* No remove pointer */
  {
   str_hdr = src_descr->data.str.saddr;
   offset = 0;
   src_descr->flags |= DF_REMOVE;
  }

 bytes_remaining = str_hdr->bytes - offset;
 if ((bytes_remaining == 0) && (str_hdr->next == NULL))
  {
   goto update_remove_table; /* End of string */
  }

 token_type = TKN_UNKNOWN;

 /* Copy next token to the result string. We do this in chunks of up to the
    size of the current source chunk.                                       */

 bytes = 0;
 done = FALSE;
 done_after = FALSE;
 tgt_bytes_remaining = 0;
 copy_char = TRUE;

 do {
     /* Is there any data left in this chunk? */

     if (bytes_remaining > 0)
      {
       p = str_hdr->data + offset;

       while((bytes_remaining-- > 0) && !done_after)
        {
         copy_char = TRUE;

         /* Examine next character */
       
         c = *(p++);

         if (token_type == TKN_UNKNOWN)
          {
           if (c >= 0x20)
            {
             delimiter = c;
             j = token_table[c - 0x20];
             token_type = j & TOKEN_MASK;
             copy_char = (j & COPY_CHAR) != 0;
             done_after = (j & DONE_AFTER) != 0;
            }
           else
            {
             if ((c & 0x60) == 0) /* Control character */
              {
               copy_char = FALSE; /* Ignore it */
              }
             else /* Unrecognised token */
              {
               done_after = TRUE;   /* 0168 */
              }
            }
          }
         else /* Token type is already known */
          {
           switch(token_type)
            {
             case TKN_NAME:
                if (c == '_')
                 {
                  /* 1.2-16 Special case for an underscore. This is valid
                     but not as the last character in the name so we need to
                     look at the next character to see what it is.           */

                  if (bytes_remaining) x = *p;   /* 0344 */
                  else if (str_hdr->next != NULL) x = str_hdr->next->data[0];
                  else x = '\0';

                  done = (!IsAlnum(x)) && (x != '.') && (x != '%') && (x != '$');
                 }
                else
                 {
                  done = (!IsAlnum(c)) && (c != '.') && (c != '%') && (c != '$');
                 }

                if (done)
                 {
                  /* Check for reserved names corresponding to operators */

                  if (bytes <= LONGEST_OPNAME) /* Could be an operator name */
                   {
                    /* Add a null terminator to the target string without
                       incrementing its length counter. There will always
                       be space to do this as we allocated a big chunk and
                       we know that we only found a short string.          */

                    *tgt = '\0';
                    for(j = 0; j < NUM_OPCODE_NAMES; j++)
                     {
                      if (stricmp(tgt_str->data, opname_strings[j]) == 0) /* 0475 */
                       {
                        token_type = opname_codes[j];
                        goto token_substituted;
                       }
                     }
                   }

                  /* We need to distinguish between
                         xyz(             Eg N(5) = 99
                     and
                         xyz (            Eg INS (I + 3) * 2 BEFORE S<1>
                     To do this, we return TKN_NAME_LBR if the name is
                     immediately followed by a left bracket.              */

                  if (c == '(') token_type = TKN_NAME_LBR;
                 }

token_substituted:
                break;

             case TKN_NUM:
                if (!IsDigit(c))
                 {
                  if (c == '.') token_type = TKN_FLOAT;
                  else if ((UpperCase(c) == 'X') && (bytes == 1) && (tgt_str->data[0] == '0'))
                   {
                    /* It's a hex number
                       We will ulimately end up with a stray leading zero
                       but that should cause no problems.                 */
                    token_type = TKN_HEXNUM;
                    copy_char = FALSE;
                   }
                  else done = TRUE;
                 }
                break;

             case TKN_LABEL:
                if (c == ':') done_after = TRUE;
                else if (strchr(label_chars, c) == NULL) done = TRUE;
                break;

             case TKN_STRING:
                if (c == delimiter)
                 {
                  copy_char = FALSE;    /* Do not transcribe to target string */
                  done_after = TRUE;
                 }
                break;

             case TKN_FLOAT:
                if (c == '.') token_type = TKN_LABEL;
                else if (!IsDigit(c)) done = TRUE;
                break;

             case TKN_LT:
                switch(c)
                 {
                  case '>':         /* <> */
                     token_type = TKN_NE;
                     done_after = TRUE;
                     break;

                  case '=':         /* <= */
                     token_type = TKN_LE;
                     done_after = TRUE;
                     break;

                  default:
                     done = TRUE;
                 }
                break;

             case TKN_GT:
                switch(c)
                 {
                  case '<':         /* >< */
                     /* 0465 We might have a nasty construct such as
                           A<B><>C  or A<B><=C
                        To handle this, we need to look ahead one further
                        character. If it is > or =, treat this as < followed
                        by <> or <=.                                         */

                     if (bytes_remaining) x = *p;
                     else if (str_hdr->next != NULL) x = str_hdr->next->data[0];
                     else x = '\0';

                     if ((x == '>') || (x == '='))
                      {
                       done = TRUE;
                      }
                     else
                      {
                       token_type = TKN_NEX;
                       done_after = TRUE;
                      }
                     break;

                  case '=':         /* >= */
                     token_type = TKN_GE;
                     done_after = TRUE;
                     break;

                  default:
                     done = TRUE;
                 }
                break;

             case TKN_EQ:
                switch(c)
                 {
                  case '<':         /* =< */
                     token_type = TKN_LE;
                     done_after = TRUE;
                     break;

                  case '>':         /* => */
                     token_type = TKN_GEX;
                     done_after = TRUE;
                     break;

                  default:
                     done = TRUE;
                 }
                break;

             case TKN_PLUS:
                if (c == '=')       /* += */
                 {
                  token_type = TKN_ADDEQ;
                  done_after = TRUE;
                 }
                else done = TRUE;
                break;

             case TKN_MINUS:
                if (c == '=')       /* -= */
                 {
                  token_type = TKN_SUBEQ;
                  done_after = TRUE;
                 }
                else if (c == '>')       /* -> */
                 {
                  token_type = TKN_OBJREF;
                  done_after = TRUE;
                 }
                else done = TRUE;
                break;

             case TKN_DIV:
                switch(c)
                 {
                  case '=':       /* /= */
                     token_type = TKN_DIVEQ;
                     done_after = TRUE;
                     break;
                  case '/':       /* // */
                     token_type = TKN_IDIV;
                     done_after = TRUE;
                     break;
                  default:
                     done = TRUE;
                     break;
                 }
                break;

             case TKN_COLON:
                if (c == '=')       /* := */
                 {
                  token_type = TKN_CATEQ;
                  done_after = TRUE;
                 }
                else done = TRUE;
                break;

             case TKN_MULT:
                switch(c)
                 {
                  case '*':       /* ** */
                     token_type = TKN_PWR;
                     done_after = TRUE;
                     break;
                  case '=':       /* *= */
                     token_type = TKN_MULTEQ;
                     done_after = TRUE;
                     break;
                  default:
                     done = TRUE;
                 }
                break;

             case TKN_AT:
                if (IsAlpha(c)) token_type = TKN_AT_NAME;
                done = TRUE;
                break;

             case TKN_HEXNUM:
                if (strchr("0123456789ABCDEF", UpperCase(c)) == NULL)
                 {
                  done = TRUE;
                 }
                break;
            }
          }


         if (done) break;

         if (copy_char)
          {
           if (tgt_bytes_remaining == 0)
            {
             new_tgt = s_alloc((long)alloc_size, &actual_size);
             tgt_bytes_remaining = actual_size;
             alloc_size = actual_size * 2; /* Next time try twice as big */

             if (bytes == 0) /* First chunk */
              {
               e_stack->data.str.saddr = new_tgt;
              }
             else /* Appending subsequent chunk */
              {
               tgt_str->next = new_tgt;
              }
   
             tgt_str = new_tgt;
             tgt = tgt_str->data;
            }

           if ((token_type == TKN_NAME) && !case_sensitive) c = UpperCase(c);
           *(tgt++) = c;
           tgt_bytes_remaining--;
           tgt_str->bytes++;
           bytes++;
          }
   
         offset++;
        } /* while((bytes_remaining-- > 0) && !done_after) */

       if (done_after) break;
      }

     if (!done) /* Find next chunk */
      {
       if (str_hdr->next == NULL)
        {
         switch(token_type)
          {
           case TKN_UNKNOWN:
              token_type = TKN_END;
              break;

           case TKN_STRING:
              /* We have run off the end of the source while collecting a
                 string. Set the token type as TKN_UNCLOSED.               */
              token_type = TKN_UNCLOSED;
              break;
          }

         offset = str_hdr->bytes; /* Leave at end of string */
         done = TRUE;
        }
       else
        {
         str_hdr = str_hdr->next;
         bytes_remaining = str_hdr->bytes;
         offset = 0;
        }
      }
    } while(!done);

 /* Go back and set total string length in first chunk of result */

 if (bytes > 0)
  {
   tgt_str = e_stack->data.str.saddr;
   tgt_str->string_len = bytes;
   tgt_str->ref_ct = 1;
  }

 type_descr->data.value = token_type;

update_remove_table:

 /* Update remove pointer */

 src_descr->n1 = offset;
 src_descr->data.str.rmv_saddr = str_hdr;

exit_rmvtkn:

 e_stack++;
}

/* END-CODE */


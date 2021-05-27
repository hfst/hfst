%option 8Bit batch yylineno nounput noyywrap prefix="sfst"
/*header-file="sfst-scanner.h"*/

/* the "incl" state is used to pick up the name of an include file */
%x incl

%{
/*******************************************************************/
/*                                                                 */
/*  FILE     scanner.ll                                            */
/*  MODULE   scanner                                               */
/*  PROGRAM  HFST                                                  */
/*                                                                 */
/*******************************************************************/

#include <string.h>

#include "SfstCompiler.h"
#include "sfst-compiler.hh"
#include "SfstBasic.h"
#include "SfstUtf8.h"
#include "HfstTransducer.h"

extern void sfsterror(const char*);

#undef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) sfsterror(msg);

using namespace hfst;
using namespace sfst_basic;

namespace hfst
{
  extern SfstCompiler * sfst_compiler;
}

#define MAX_INCLUDE_DEPTH 10
  
int Include_Stack_Ptr = 0;
YY_BUFFER_STATE Include_Stack[MAX_INCLUDE_DEPTH];
char *Name_Stack[MAX_INCLUDE_DEPTH];
int  Lineno_Stack[MAX_INCLUDE_DEPTH];

bool UTF8=true;

static char *unquote(char *string, bool del_quote=true) {
  char *s=string, *result=string;
  if (del_quote)
    string++;

  while (*string) {
    if (*string == '\\')
      string++;
    *(s++) = *(string++);
  }

  if (del_quote)
    s--;
  *s = '\0';

  return fst_strdup(result);
}

static void print_lineno() {
  if (!sfst_compiler->Verbose)
    return;
  fputc('\r',stderr);
  for( int i=0; i<Include_Stack_Ptr; i++ )
    fputs("  ", stderr);
  fprintf(stderr,"%s: %d", sfst_compiler->filename.c_str(), sfstlineno);
}

%}

CC	[\x80-\xbf]
C1	[A-Za-z0-9._/\-]
C2	[A-Za-z0-9._/\-&()+,=?\^|~]
C3	[A-Za-z0-9._/\-&()+,=?\^|~#<>]
C4	[A-Za-z0-9._/\-&()+,=?\^|~$<>]
C5	[\!-;\?-\[\]-\~=]
FN	[A-Za-z0-9._/\-*+]

%%

^[ \t]*\#use[ \t]*hopcroft[ \t]*\n { hfst::set_minimization_algorithm(hfst::HOPCROFT);};
^[ \t]*\#use[ \t]*default[ \t]*\n { hfst::set_minimization_algorithm(hfst::BRZOZOWSKI);};


#include           BEGIN(incl);
<incl>[ \t]*       /* eat the whitespace */
<incl>{FN}+        { sfst_compiler->error2("Missing quotes",sfsttext); }
<incl>\"{FN}+\"    { /* got the include file name */
                     FILE *file;
                     char *name=fst_strdup(sfsttext+1);
		     name[strlen(name)-1] = 0;
                     if ( Include_Stack_Ptr >= MAX_INCLUDE_DEPTH ) {
		       fprintf( stderr, "Includes nested too deeply" );
                       HFST_THROW(HfstException);
		     }
		     if (sfst_compiler->Verbose) fputc('\n', stderr);
		     file = fopen( name, "rt" );
		     if (!file)
                       sfst_compiler->error2("Can't open include file", name);
                     else {
                       Name_Stack[Include_Stack_Ptr] = strdup(sfst_compiler->filename.c_str());
                       sfst_compiler->set_filename(std::string(name));
                       Lineno_Stack[Include_Stack_Ptr] = sfstlineno;
		       sfstlineno = 1;
		       Include_Stack[Include_Stack_Ptr++]=YY_CURRENT_BUFFER;
		       sfst_switch_to_buffer(sfst_create_buffer(sfstin, YY_BUF_SIZE));
                       sfstin = file;
		       print_lineno();
		       BEGIN(INITIAL);
                     }
                  }
<<EOF>>           {
                     if (sfst_compiler->Verbose)
		       fputc('\n', stderr);
                     if ( --Include_Stack_Ptr < 0 )
		       yyterminate();
		     else {
                       //free(FileName);
                       sfst_compiler->set_filename(std::string(Name_Stack[Include_Stack_Ptr]));
                       sfstlineno = Lineno_Stack[Include_Stack_Ptr];
		       sfst_delete_buffer( YY_CURRENT_BUFFER );
		       sfst_switch_to_buffer(Include_Stack[Include_Stack_Ptr]);
                     }
                  }


^[ \t]*\%.*\r?\n  { print_lineno();  /* ignore comments */ }

\%.*\\[ \t]*\r?\n { print_lineno();  /* ignore comments */ }

\%.*              { /* ignore comments */ }


^[ \t]*ALPHABET[ \t]*= { return ALPHA; }

\|\|              { return COMPOSE; }
"<=>"             { sfstlval.type = twol_both; return ARROW; }
"=>"              { sfstlval.type = twol_right;return ARROW; }
"<="              { sfstlval.type = twol_left; return ARROW; }
"^->"             { sfstlval.rtype = repl_up;   return REPLACE; }
"_->"             { sfstlval.rtype = repl_down; return REPLACE; }
"/->"             { sfstlval.rtype = repl_right;return REPLACE; }
"\\->"            { sfstlval.rtype = repl_left; return REPLACE; }
">>"              { return PRINT; }
"<<"              { return INSERT; }
"<<<"             { return SUBSTITUTE; }
"__"              { return POS; }
"^_"              { return SWITCH; }

[.,{}\[\]()&!?|*+:=_\^\-] { return sfsttext[0]; }

\$=({C3}|(\\.))+\$ { sfstlval.name = fst_strdup(sfsttext); return RVAR; }

\$({C3}|(\\.))+\$ { sfstlval.name = fst_strdup(sfsttext); return VAR; }

#=({C4}|(\\.))+# { sfstlval.name = fst_strdup(sfsttext); return RSVAR; }

#({C4}|(\\.))+# { sfstlval.name = fst_strdup(sfsttext); return SVAR; }

\<({C5}|\\.)*\>   { sfstlval.name = unquote(sfsttext,false); return SYMBOL; }

\"<{FN}+>\" {
                    sfstlval.value = fst_strdup(sfsttext+2);
		    sfstlval.value[strlen(sfstlval.value)-2] = 0;
                    return STRING2;
                  }

\"{FN}+\" {
                    sfstlval.value = fst_strdup(sfsttext+1);
		    sfstlval.value[strlen(sfstlval.value)-1] = 0;
                    return STRING;
                  }

[ \t]             { /* ignored */ }
\\[ \t]*([ \t]\%.*)?\r?\n { print_lineno(); /* ignored */ }
\r?\n             { print_lineno(); return NEWLINE; }

\\[0-9]+          { long l=atol(sfsttext+1);
		    if (l <= 1114112) { sfstlval.value=fst_strdup(sfst_utf8::int2utf8((unsigned)l)); return UTF8CHAR; }
		    sfsterror(strdup("invalid expression"));
                  }


\\.                { sfstlval.value=fst_strdup(sfsttext+1); return UTF8CHAR; }
[\x00-\x7f]        { sfstlval.value=fst_strdup(sfsttext); return UTF8CHAR; }
[\xc0-\xdf]{CC}    { sfstlval.value=fst_strdup(sfsttext); return UTF8CHAR; }
[\xe0-\xef]{CC}{2} { sfstlval.value=fst_strdup(sfsttext); return UTF8CHAR; }
[\xf0-\xff]{CC}{3} { sfstlval.value=fst_strdup(sfsttext); return UTF8CHAR; }

%%

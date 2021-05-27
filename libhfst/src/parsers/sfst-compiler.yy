%{
/*******************************************************************/
/*                                                                 */
/*  FILE     sfst-compiler.yy                                       */
/*  MODULE   sfst-compiler                                          */
/*  PROGRAM  HFST                                                  */
/*                                                                 */
/*******************************************************************/

#ifdef WINDOWS
#include <io.h>
#endif

#include "SfstCompiler.h"
#include "HfstTransducer.h"

extern int  sfstlineno;
extern char *sfsttext;
int sfstlex( void );

using namespace hfst;
using std::cerr;

namespace hfst
{
  extern SfstCompiler * sfst_compiler;
}

bool DEBUG = false;

void sfsterror(const char *text)

{
  cerr << "\n" << sfst_compiler->filename << ":" << sfstlineno << ": " << text << " at: ";
  cerr << sfsttext << "\naborted.\n";
  HFST_THROW(HfstException);
}

void warn(const char *text)

{
  cerr << "\n" << sfst_compiler->filename << ":" << sfstlineno << ": warning: " << text << "!\n";
}

void warn2(const char *text, const char *text2)

{
  cerr << "\n" << sfst_compiler->filename << ":" << sfstlineno << ": warning: " << text << ": ";
  cerr << text2 << "\n";
}

%}

%name-prefix="sfst"

%union {
  int        number;
  hfst::Twol_Type  type;
  hfst::Repl_Type  rtype;
  char       *name;
  char       *value;
  unsigned char uchar;
  unsigned int  longchar;
  hfst::Character  character;
  hfst::HfstTransducer   *expression;
  hfst::Range      *range;
  hfst::Ranges     *ranges;
  hfst::Contexts   *contexts;
}

%token <number> NEWLINE ALPHA COMPOSE PRINT POS INSERT SUBSTITUTE SWITCH
%token <type>   ARROW
%token <rtype>  REPLACE
%token <name>   SYMBOL VAR SVAR RVAR RSVAR
%token <value>  STRING STRING2 UTF8CHAR
%token <uchar>  CHARACTER

%type  <uchar>      SCHAR
%type  <longchar>   LCHAR
%type  <character>  CODE
%type  <expression> RE
%type  <range>      RANGE VALUE VALUES
%type  <ranges>     RANGES
%type  <contexts>   CONTEXT CONTEXT2 CONTEXTS CONTEXTS2

%left PRINT INSERT SUBSTITUTE
%left ARROW REPLACE
%left COMPOSE
%left '|'
%left '-'
%left '&'
%left SEQ
%left '!' '^' '_'
%left '*' '+'
%%

ALL:        ASSIGNMENTS RE NEWLINES { sfst_compiler->set_result(sfst_compiler->result($2, sfst_compiler->switch_)); }
          ;

ASSIGNMENTS: ASSIGNMENTS ASSIGNMENT {}
          | ASSIGNMENTS NEWLINE     {}
          | /* nothing */           {}
          ;

ASSIGNMENT: VAR '=' RE              { if (DEBUG) { printf("defining transducer variable \"%s\"..\n", $1); }; if (sfst_compiler->def_var($1,$3)) warn2("assignment of empty transducer to",$1); if(DEBUG) printf("done\n"); }
          | RVAR '=' RE             { if (DEBUG) { printf("defining agreement transducer variable \"%s\"..\n", $1); }; if (sfst_compiler->def_rvar($1,$3)) warn2("assignment of empty transducer to",$1); }
          | SVAR '=' VALUES         { if (DEBUG) { printf("defining range variable \"%s\"..\n", $1); }; if (sfst_compiler->def_svar($1,$3)) warn2("assignment of empty symbol range to",$1); }
          | RSVAR '=' VALUES        { if (DEBUG) { printf("defining agreement range variable \"%s\"..\n", $1); }; if (sfst_compiler->def_svar($1,$3)) warn2("assignment of empty symbol range to",$1); }
          | RE PRINT STRING         { sfst_compiler->write_to_file($1, sfst_compiler->foldername.c_str(), $3); }
          | ALPHA RE                { if (DEBUG) { printf("defining alphabet..\n"); }; sfst_compiler->def_alphabet($2); delete $2; }
          ;

RE:         RE ARROW CONTEXTS2      { $$ = sfst_compiler->restriction($1,$2,$3,0); }
	  | RE '^' ARROW CONTEXTS2  { $$ = sfst_compiler->restriction($1,$3,$4,1); }
	  | RE '_' ARROW CONTEXTS2  { $$ = sfst_compiler->restriction($1,$3,$4,-1); }
          | RE REPLACE CONTEXT2     { $1 = sfst_compiler->explode($1); $1->minimize(); $$ = sfst_compiler->replace_in_context($1, $2, $3, false); }
          | RE REPLACE '?' CONTEXT2 { $1 = sfst_compiler->explode($1); $1->minimize(); $$ = sfst_compiler->replace_in_context($1, $2, $4, true); }
          | RE REPLACE '(' ')'      { $1 = sfst_compiler->explode($1); $1->minimize(); $$ = sfst_compiler->replace($1, $2, false); }
          | RE REPLACE '?' '(' ')'  { $1 = sfst_compiler->explode($1); $1->minimize(); $$ = sfst_compiler->replace($1, $2, true); }
          | RE RANGE ARROW RANGE RE { $$ = sfst_compiler->make_rule($1,$2,$3,$4,$5, sfst_compiler->compiler_type); }
          | RE RANGE ARROW RANGE    { $$ = sfst_compiler->make_rule($1,$2,$3,$4,NULL, sfst_compiler->compiler_type); }
          | RANGE ARROW RANGE RE    { $$ = sfst_compiler->make_rule(NULL,$1,$2,$3,$4, sfst_compiler->compiler_type); }
          | RANGE ARROW RANGE       { $$ = sfst_compiler->make_rule(NULL,$1,$2,$3,NULL, sfst_compiler->compiler_type); }
          | RE COMPOSE RE    { $1->compose(*$3); delete $3; $$ = $1; }
          | '{' RANGES '}' ':' '{' RANGES '}' { $$ = sfst_compiler->make_mapping($2,$6,sfst_compiler->compiler_type); }
          | RANGE ':' '{' RANGES '}' { $$ = sfst_compiler->make_mapping(sfst_compiler->add_range($1,NULL),$4,sfst_compiler->compiler_type); }
          | '{' RANGES '}' ':' RANGE { $$ = sfst_compiler->make_mapping($2,sfst_compiler->add_range($5,NULL),sfst_compiler->compiler_type); }
          | RE INSERT CODE ':' CODE  { $$ = sfst_compiler->insert_freely($1,$3,$5); }
          | RE INSERT CODE           { $$ = sfst_compiler->insert_freely($1,$3,$3); }
	  | RE SUBSTITUTE CODE ':' CODE  { $$ = sfst_compiler->substitute($1,$3,$5); }
	  | RE SUBSTITUTE CODE ':' CODE ':' CODE ':' CODE { $$ = sfst_compiler->substitute($1,$3,$5,$7,$9); }
	  | RE SUBSTITUTE CODE ':' CODE '(' RE ')' { $$ = sfst_compiler->substitute($1,$3,$5,$7); }
          | RANGE ':' RANGE  { $$ = sfst_compiler->new_transducer($1,$3,sfst_compiler->compiler_type); }
          | RANGE            { $$ = sfst_compiler->new_transducer($1,$1,sfst_compiler->compiler_type); }
          | VAR              { if (DEBUG) { printf("calling transducer variable \"%s\"\n", $1); }; $$ = sfst_compiler->var_value($1); }
          | RVAR             { if (DEBUG) { printf("calling agreement transducer variable \"%s\"\n", $1); }; $$ = sfst_compiler->rvar_value($1,sfst_compiler->compiler_type); }
          | RE '*'           { $1->repeat_star(); $$ = $1; }
          | RE '+'           { $1->repeat_plus(); $$ = $1; }
          | RE '?'           { $1->optionalize(); $$ = $1; }
          | RE RE %prec SEQ  { $1->concatenate(*$2); delete $2; $$ = $1; }
          | '!' RE           { $$ = sfst_compiler->negation($2); }
          | SWITCH RE        { $2->invert(); $$ = $2; }
          | '^' RE           { $2->output_project(); $$ = $2; }
          | '_' RE           { $2->input_project(); $$ = $2; }
          | RE '&' RE        { $1->intersect(*$3); delete $3; $$ = $1; }
          | RE '-' RE        { $1->subtract(*$3); delete $3; $$ = $1; }
          | RE '|' RE        { $1->disjunct(*$3); delete $3; $$ = $1; }
          | '(' RE ')'       { $$ = $2; }
          | STRING           { $$ = sfst_compiler->read_words(sfst_compiler->foldername.c_str(), $1, sfst_compiler->compiler_type); }
          | STRING2          { try { $$ = sfst_compiler->read_transducer(sfst_compiler->foldername.c_str(), $1, sfst_compiler->compiler_type); } catch (HfstException e) { printf("\nAn error happened when reading file \"%s\"\n", $1);   HFST_THROW(HfstException); } }
          ;

RANGES:     RANGE RANGES     { $$ = sfst_compiler->add_range($1,$2); }
          |                  { $$ = NULL; }
          ;

RANGE:      '[' VALUES ']'   { $$=$2; }
          | '[' '^' VALUES ']' { $$=sfst_compiler->complement_range($3); }
          | '[' RSVAR ']'    { if (DEBUG) { printf("calling agreement range variable \"%s\"\n", $2); }; $$=sfst_compiler->rsvar_value($2); }
          | '.'              { $$=NULL; }
          | CODE             { $$=sfst_compiler->add_value($1,NULL); }
          ;

CONTEXTS2:  CONTEXTS               { $$ = $1; }
          | '(' CONTEXTS ')'       { $$ = $2; }
          ;

CONTEXTS:   CONTEXT ',' CONTEXTS   { $$ = sfst_compiler->add_context($1,$3); }
          | CONTEXT                { $$ = $1; }
          ;

CONTEXT2:   CONTEXT                { $$ = $1; }
          | '(' CONTEXT ')'        { $$ = $2; }
          ;

CONTEXT :   RE POS RE              { $$ = sfst_compiler->make_context($1, $3); }
          |    POS RE              { $$ = sfst_compiler->make_context(NULL, $2); }
          | RE POS                 { $$ = sfst_compiler->make_context($1, NULL); }
          ;

VALUES:     VALUE VALUES           { $$=sfst_compiler->append_values($1,$2); }
          | VALUE                  { $$ = $1; }
          ;

VALUE:      LCHAR '-' LCHAR	   { $$=sfst_compiler->add_values($1,$3,NULL); }
          | SVAR                   { if (DEBUG) { printf("calling range variable \"%s\"", $1); }; $$=sfst_compiler->svar_value($1); }
          | LCHAR  	           { $$=sfst_compiler->add_value(sfst_compiler->character_code($1),NULL); }
          | CODE		   { $$=sfst_compiler->add_value($1,NULL); }
	  | SCHAR		   { $$=sfst_compiler->add_value($1,NULL); }
          ;

LCHAR:      CHARACTER	{ $$=$1; }
          | UTF8CHAR	{ $$=sfst_compiler->utf8toint($1); free($1); }
	  | SCHAR       { $$=$1; }
          ;

CODE:       CHARACTER	{ $$=sfst_compiler->character_code($1); }
          | UTF8CHAR	{ $$=sfst_compiler->symbol_code($1); }
          | SYMBOL	{ $$=sfst_compiler->symbol_code($1); }
          ;

SCHAR:      '.'		{ $$=(unsigned char)sfst_compiler->character_code('.'); }
          | '!'		{ $$=(unsigned char)sfst_compiler->character_code('!'); }
          | '?'		{ $$=(unsigned char)sfst_compiler->character_code('?'); }
          | '{'		{ $$=(unsigned char)sfst_compiler->character_code('{'); }
          | '}'		{ $$=(unsigned char)sfst_compiler->character_code('}'); }
          | ')'		{ $$=(unsigned char)sfst_compiler->character_code(')'); }
          | '('		{ $$=(unsigned char)sfst_compiler->character_code('('); }
          | '&'		{ $$=(unsigned char)sfst_compiler->character_code('&'); }
          | '|'		{ $$=(unsigned char)sfst_compiler->character_code('|'); }
          | '*'		{ $$=(unsigned char)sfst_compiler->character_code('*'); }
          | '+'		{ $$=(unsigned char)sfst_compiler->character_code('+'); }
          | ':'		{ $$=(unsigned char)sfst_compiler->character_code(':'); }
          | ','		{ $$=(unsigned char)sfst_compiler->character_code(','); }
          | '='		{ $$=(unsigned char)sfst_compiler->character_code('='); }
          | '_'		{ $$=(unsigned char)sfst_compiler->character_code('_'); }
          | '^'		{ $$=(unsigned char)sfst_compiler->character_code('^'); }
          | '-'		{ $$=(unsigned char)sfst_compiler->character_code('-'); }
          ;

NEWLINES:   NEWLINE NEWLINES     {}
          | /* nothing */        {}
          ;

%%

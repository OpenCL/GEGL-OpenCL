%{
    /* This version has just statements, expressions, variables, blocks */ 
#include "gilsymboltable.h"
#include "gilast.h"
#include "gilinterpret.h"
extern GilSymbolTable * table;
#define DO_ACTIONS
    
%}

%union {
  gint intVal;
  gfloat floatVal;
  gchar *idVal;
  gint opVal;
  GNode * nodeVal;
};

%token <idVal> IDENTIFIER 
%token <intVal> INT_CONSTANT
%token <floatVal> FLOAT_CONSTANT
%token FLOAT INT

%type <opVal> unary_operator 
%type <nodeVal> primary_expr assignment_expr unary_expr multiplicative_expr additive_expr 
%type <nodeVal> compound_statement expression_statement statement_list statement 

%start file
%%

primary_expr
    : IDENTIFIER
      {
        $$ = gil_node_id_new($1); 
      }
    | INT_CONSTANT
      {
        $$ = gil_node_int_constant_new($1); 
      }
    | FLOAT_CONSTANT
      {
        $$ = gil_node_float_constant_new($1); 
      }
    | '(' assignment_expr ')'
      {
        $$ = $2; 
      }
    ;

unary_expr
    : primary_expr
    | unary_operator unary_expr
      {
        $$ = gil_node_op_new($1,1,$2); 
      }
    ;

unary_operator
    : '+'
      {
        $$ = (gint)'+';
      }
    | '-'
      {
        $$ = (gint)'-';
      }
    | '!'
      {
        $$ = (gint)'!';
      }
    ;

multiplicative_expr
    : unary_expr
    | multiplicative_expr '*' unary_expr
      {
        $$ = gil_node_op_new('*',2,$1,$3); 
      }
    | multiplicative_expr '/' unary_expr
      {
        $$ = gil_node_op_new('/',2,$1,$3); 
      }
    ;

additive_expr
    : multiplicative_expr
    | additive_expr '+' multiplicative_expr
      {
        $$ = gil_node_op_new('+',2,$1,$3); 
      }
    | additive_expr '-' multiplicative_expr
      {
        $$ = gil_node_op_new('-',2,$1,$3); 
      }
    ;

assignment_expr
    : additive_expr 
    | unary_expr '=' assignment_expr
      {
        $$ = gil_node_op_new('=',2,$1,$3); 
      }
    ;

statement
	: compound_statement
	| expression_statement
	;

compound_statement
	: '{' '}'
    {
      $$ = gil_node_block_new(); 
    }
	| '{' statement_list '}'
    {
      $$ = gil_node_block_new();
      g_node_append($$,$2);
    }
	;

statement_list
	: statement
    {
      $$ = gil_node_statement_list_new();
      g_node_append($$,$1);
    }
	| statement_list statement
    {
      g_node_append($1,$2);
      $$ = $1;
    }
	;

expression_statement
	: ';'
	  {
      $$ = gil_node_op_new(';',0);
	  }
	| assignment_expr ';'
    {
      $$=$1;
    } 
	;

file
	: compound_statement 
    {
      printf("\n");
      gil_print_ast($1,0);
    }
	;

%%

extern char yytext[];
extern int column;

yyerror(s)
char *s;
{
	fflush(stdout);
	printf("\n%*s\n%*s\n", column, "^", column, s);
}

int main()
{
	int yyparse();
  int val = yyparse();

	return(val);
}

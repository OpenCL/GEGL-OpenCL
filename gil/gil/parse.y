%{
    /* This version has just statements, expressions, variables, blocks */ 
#include "gil.h"
#define DO_ACTIONS
    
%}

%union {
  gint intVal;
  gfloat floatVal;
  gchar *variable;
  GilType type;
  GilBinaryOpType binary_op;
  GilUnaryOpType unary_op;
  GilNode *node;
};

%token <variable> IDENTIFIER 
%token <intVal> INT_CONSTANT
%token <floatVal> FLOAT_CONSTANT
%token FLOAT INT

%type <unary_op> unary_operator 
%type <node> primary_expr assignment_expr unary_expr multiplicative_expr additive_expr 
%type <node> compound_statement expression_statement statement_list statement 

%start file
%%

primary_expr
    : IDENTIFIER
      {
        $$ = g_object_new(GIL_TYPE_VARIABLE, "variable", $1, NULL); 
      }
    | INT_CONSTANT
      {
        $$ = g_object_new(GIL_TYPE_CONSTANT, "int", $1, NULL); 
      }
    | FLOAT_CONSTANT
      {
        $$ = g_object_new(GIL_TYPE_CONSTANT, "float", $1, NULL); 
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
        $$ = g_object_new(GIL_TYPE_UNARY_OP, "op", $1, "operand", $2, NULL); 
      }
    ;

unary_operator
    : '+'
      {
        $$ = GIL_UNARY_PLUS;
      }
    | '-'
      {
        $$ = GIL_UNARY_MINUS;
      }
    | '!'
      {
        $$ = GIL_UNARY_NEG;
      }
    ;

multiplicative_expr
    : unary_expr
    | multiplicative_expr '*' unary_expr
      {
        $$ = g_object_new(GIL_TYPE_BINARY_OP, 
                          "op", GIL_MULT, 
                          "left_operand", $1, 
                          "right_operand", $3, 
                          NULL); 
      }
    | multiplicative_expr '/' unary_expr
      {
        $$ = g_object_new(GIL_TYPE_BINARY_OP, 
                          "op", GIL_DIV, 
                          "left_operand", $1, 
                          "right_operand", $3, 
                          NULL); 
      }
    ;

additive_expr
    : multiplicative_expr
    | additive_expr '+' multiplicative_expr
      {
        $$ = g_object_new(GIL_TYPE_BINARY_OP, 
                          "op", GIL_ADD, 
                          "left_operand", $1, 
                          "right_operand", $3, 
                          NULL); 
      }
    | additive_expr '-' multiplicative_expr
      {
        $$ = g_object_new(GIL_TYPE_BINARY_OP, 
                          "op", GIL_MINUS, 
                          "left_operand", $1, 
                          "right_operand", $3, 
                          NULL); 
      }
    ;

assignment_expr
    : additive_expr 
    | unary_expr '=' assignment_expr
      {
        $$ = g_object_new(GIL_TYPE_EXPR_STATEMENT, 
                          "left_operand", $1, 
                          "right_operand", $3, 
                          NULL); 
      }
    ;

statement
	: compound_statement
	| expression_statement
	;

compound_statement
	: '{' '}'
    {
      $$ = g_object_new(GIL_TYPE_BLOCK, NULL);
    }
	| '{' statement_list '}'
    {
      $$ = g_object_new(GIL_TYPE_BLOCK, NULL);
      gil_node
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

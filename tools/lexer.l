%{
#include <stdio.h>
#include <unistd.h>

    FILE *output_file, *input_file; 
%}
%%
"\t"	{
	fprintf (output_file, "       ");
	}
"\n"	{
	fprintf (output_file, "\n");
	}
" "	{
    	fprintf (output_file, " "); 
    	}
.	{
	fprintf (output_file, "%s", yytext);  
	}
%%

int
main (int argc, char **argv)
{


  char output_name[100];

  sprintf (output_name, "new_%s", argv[1]);

  input_file = fopen (argv[1], "r");
	
  output_file = fopen (output_name, "w");

  yyin = input_file;
  
  yylex (); 

  fclose (input_file);
  fclose (output_file);
  
  execl ("/bin/mv", "/bin/mv", output_name, argv[1], (char*)0); 

  return 0;

}


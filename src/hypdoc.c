#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <exec/types.h>


#define ASCII_DOC	"SDK:doc/csh.doc"
#define HYPER_GUIDE	"SDK:Help/csh-tutorial.guide"
#define HYPER_INDEX	"SDK:Help/csh.guide"


char indx[300][20];


int main(int argc, char *argv[])
{
	char Line[256];
	FILE *in,*out1,*out2;
	int index_mode = 0;
	int indx_i = 0;
	int indx_w = 0;
	char *p;

	if (!(in=fopen(ASCII_DOC,"r"))) {
		exit(5);
	}

	if (!(out1=fopen(HYPER_GUIDE,"w"))) {
		fclose(in);
		exit(5);
	}

	if (!(out2=fopen(HYPER_INDEX,"w"))) {
		fclose(in);
		fclose(out1);
		exit(5);
	}

	fprintf(out2,	"@database csh-reference\n\n"
			"##\n"
			"## Use AmigaGuide to view this document.\n"
			"##\n\n"
			"@node main \"C-Shell documentation\"\n"
			"@width 77\n\n"
			"  C-SHELL 5.20+ REFERENCE,  Copyright 1991-1994 by the authors.\n\n"
			"  @{\"   Commands    \" link Commands  }\n"
			"  @{\"   Variables   \" link Variables }\n"
			"  @{\"   Functions   \" link Functions }\n"
			"\n@endnode\n\n"
		);

	while (fgets(Line,255,in)) {
		if (strcmp(Line,"LIST OF COMMANDS:\n")==0) {
			index_mode = 1;
			indx_i = indx_w = 0;
			fgets(Line,255,in);
		}
		else if (strcmp(Line,"LIST OF VARIABLES:\n")==0) {
			index_mode = 2;
			indx_i = indx_w = 0;
			fgets(Line,255,in);
		}
		else if (strcmp(Line,"LIST OF FUNCTIONS:\n")==0) {
			index_mode = 3;
			indx_i = indx_w = 0;
			fgets(Line,255,in);
		}

		else if (index_mode) {
			if (Line[0]!=' ' && Line[0]!='\t' && Line[0]!='\n' && Line[0]!='\0') {
				char cmd_lower[80];
				int i;
				fprintf(out2,"@endnode\n\n");
				switch (index_mode) {
				  case 1: fprintf(out2,"@node Commands\n\n"); break;
				  case 2: fprintf(out2,"@node Variables\n\n"); break;
				  case 3: fprintf(out2,"@node Functions\n\n"); break;
				}
				for (i=1; i<=indx_i; i++) {
					strcpy(cmd_lower,indx[i]);
					strlwr(cmd_lower);
					fprintf(out2,"  @{\"%-*.*s\" Link %-*.*s}",indx_w,indx_w,cmd_lower,indx_w,indx_w,indx[i]);
					/* AG-Window (topaz8, 640x200) is 76 chars wide */
					if (i% (76/(indx_w+3)) == 0 || i==indx_i)
						fprintf(out2,"\n");
				}
				fprintf(out2,"@endnode\n\n");
				fprintf(out1,"%s",Line);
				index_mode = 0;
			}
			else if (strncmp(Line,"    ",4)==0 && Line[4]!=' ' && Line[4]!='\0' && Line[4]!='\n') {
				int i;
				strncpy(indx[++indx_i],Line+4,19);
				indx[indx_i][20] = 0;
				if (p=strchr(indx[indx_i],'\n'))
					*p = 0;
				if (p=strchr(indx[indx_i],'('))
					*p = 0;
				if (indx_i>1)
					fprintf(out2,"@endnode\n");
				if ((i=strlen(indx[indx_i]))>indx_w)
					indx_w = i;
				fprintf(out2,"@node %s\n\n%s\n",indx[indx_i],Line);
			}
			else {
				fprintf(out2,"%s",Line);
			}
		}
		else {
			fprintf(out1,"%s",Line);
		}
	}

	fclose(in);
	fclose(out1);
	fclose(out2);

	exit(0);
}


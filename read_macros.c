#include <stdlib.h>
#include <stdio.h>

#define MACRO_FILE "/home/pj/test.txt"
#define OUT_TMP_FILE "tmp.txt"
#define MAX_MACRO_LENGTH 5

struct _full_macro {
	int size;
	int keycodes[40];
	int status[40];
};

struct _macro {
    int keycodes[MAX_MACRO_LENGTH];
	int len;
	struct _full_macro keys;
	struct _macro *next;
} *macrosHead = NULL, *macrosTail = NULL, *curr_macro = NULL;

static int print_macro(FILE* outfile, struct _macro* macro) {
	for(int i=0;i<macro->len;i++)
		fprintf(outfile,"%d,",macro->keycodes[i]);
	fprintf(outfile," ");

	for(int i=0;i<macro->keys.size;i++) {
		fprintf(outfile,"%d:%d ",macro->keys.keycodes[i], macro->keys.status[i]);
	}
	fprintf(outfile,"\n");
}
static struct _macro* new_macro(void) {
	struct _macro* tmp = malloc(sizeof(struct _macro));

	for(int i=0; i<MAX_MACRO_LENGTH; i++)
		tmp->keycodes[i] = -1;
	tmp->len = 0;
	tmp->next = NULL;
	tmp->keys.size = 0;

	return tmp;
}


static int read_macros(void)
{
	int cnt = 0;
	FILE* mac_file,*outfile;

	mac_file = fopen(MACRO_FILE,"r");
	outfile = fopen(OUT_TMP_FILE, "w");
	if(mac_file == NULL) {
		return 0;
	}
	curr_macro = new_macro();
	while(fread(curr_macro, sizeof(struct _macro), 1, mac_file))
	{
		print_macro(outfile,curr_macro);
	}
	free(curr_macro);
	return 0;
}

int main() {
	read_macros();
}
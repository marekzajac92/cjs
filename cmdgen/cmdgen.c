#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

int main(int argc, char *argv[]) {
	FILE *file = fopen("cmd.txt", "wb");

	int d; 
	while(1) {
		scanf("%d", &d);
		if(d == -1) break;
		fprintf(file, "%c", (char) d);	
	}

	fflush(file);
	fclose(file);

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "holdall.h"
#include <locale.h>
#include <ctype.h>

#define BUFF_SIZE_MIN 100

#define INPUT_DIRECT "-"
#define INPUT_FILE_NEXT "--"

int rfree(void *p);

size_t str_hashfun(const char *s);

int display(const char *str);

FILE* get_input_type(char **av, int i);

int main(int ac, char **av) {
	int r = EXIT_SUCCESS;
	if (ac <= 1) {
		return r;
	}
	setlocale(LC_COLLATE, "");
  hashtable *ht = hashtable_empty((int (*)(const void *, const void *))strcoll,
      (size_t (*)(const void *))str_hashfun, 1.0);
 	holdall *ha = holdall_empty();
 	if (ht == NULL || ha == NULL) {
 		goto error_capacity;
 	}
 	//buff : buffer qui stocke les lettres lus
 	char *buff = malloc(BUFF_SIZE_MIN + 1);
 	if (buff == NULL) {
 		goto error_capacity;
 	}
 	//current_arg : argument entrain d'être traité
	int current_arg = 1;
 	while (current_arg < ac) {
 		FILE* f = get_input_type(av, current_arg);
 		if (f == NULL) {
 			goto error_read;
 		}
 		if (f == stdin) {
 			printf("--- starts reading for #%d FILE\n", current_arg);
 		}
 		else { 
	 		if (strcmp(av[current_arg], INPUT_FILE_NEXT) == 0) {
	 			++current_arg;
	 			if (current_arg >= ac) {
	        fprintf(stderr, "*** Missing file argument\n");
	 				goto error;
	 			}
	 		}
	 		printf("--- starts reading for \"%s\"\n", av[current_arg]);
	 	}
	 	//i : indice du buffer
	 	size_t i = 0;
	 	//c : caractère lu
	 	int c = 0;
	 	while (c != EOF) {
	 		c = fgetc(f);
	 		//si c'est la fin du mot, on alloue et on envoie dans le holdall & HT
	 		if (isspace(c) || c == EOF) {
	 			//on passe jusqu'au prochain mot
	 			while (isspace(c) && c != EOF) {
	 				c = fgetc(f);
	 			}
	 			//i > 0 assure que les chaines allouées ne sont pas vides
	 			if (i > 0) {
		 			buff[i] = '\0';
		 			//allocation de ce qui va être envoyé
		 			char *t = malloc(i + 1);
		 			if (t == NULL) {
		 				goto error_capacity;
		 			}
		 			//copie dans t
		 			strcpy(t, buff);
		 			if (holdall_put(ha, t) != 0) {
		 				goto error_capacity;
		 			}
		 			//reinitialisation du buffer
		 			memset(buff, 0, i);
		 			i = 0;
		 		}
	 		}
		 	buff[i] = (char) c;
		 	++i;
	 	}
	 	if (f != stdin) {
	 		//test si on a bien tout lu
      if (!feof(f)) {
        goto error_read;
      }
      //ferme le fichier
	 		if (fclose(f) != 0) {
        fprintf(stderr, "*** Failed to close the file : %s\n", av[current_arg]);
        goto error;
	 		}
	 	}
	 	else {
	 		clearerr(f);
      printf("--- ends reading for #%d FILE\n", current_arg);
	 	}
	 	//passe au prochain fichier
	 	++current_arg;
	}
	//affiche les mots qui ont été lu
 	holdall_apply(ha, (int (*)(void *))display);
 	free(buff);
 	goto dispose;

	error_read:
	  fprintf(stderr, "*** Error: A read error occurs\n");
	  goto error;
	/*error_write:
	  fprintf(stderr, "*** Error: A write error occurs\n");
	  goto error;
	*/
	error_capacity:
	  fprintf(stderr, "*** Error: Not enough memory\n");
	  goto error;
	error:
	  r = EXIT_FAILURE;
	  goto dispose;
	dispose:
	  hashtable_dispose(&ht);
	  if (ha != NULL) {
	    holdall_apply(ha, rfree);
	  }
	  holdall_dispose(&ha);
	  return r;
}

FILE* get_input_type(char **av, int i) {
	if (strcmp(av[i], INPUT_DIRECT) == 0) {
		return stdin;
	}
	//si un fichier suit, incremente, et ouvre le fichier
	if (strcmp(av[i], INPUT_FILE_NEXT) == 0) {
		++i;
	}
	FILE* f = fopen(av[i], "r");
	if (f == NULL) {
     fprintf(stderr, "*** Failed to open the file : %s\n", av[i]);
     return NULL;
	}
	return f;
}

int display(const char *str) {
	printf("%s\n", str);
	return 0;
}

size_t str_hashfun(const char *s) {
  size_t h = 0;
  for (const unsigned char *p = (const unsigned char *) s; *p != '\0'; ++p) {
    h = 37 * h + *p;
  }
  return h;
}

int rfree(void *p) {
	free(p);
	return 0;
}

/*
	on lit les fichiers, on stocke chaque mot dans un fourretout & HT
*/
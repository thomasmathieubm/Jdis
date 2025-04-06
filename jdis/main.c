#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "holdall.h"
#include <locale.h>
#include <ctype.h>

#define BUFF_SIZE_MIN 100

#define MAX_FILE_NUM 100

#define INPUT_DIRECT "-"
#define INPUT_FILE_NEXT "--"

#define CAPACITY_MIN 2

typedef struct file_info file_info;
struct file_info {
	int *file_nums;
	size_t capacity;
	size_t card;
};

int rfree(void *p);

int file_info_dispose(void *p);

int file_info_dispose(void *p);

size_t str_hashfun(const char *s);

int display(const char *str);

FILE* get_input_type(char **av, int i);

bool is_word_from_diff_file(file_info *p, int current_file);

int display_graph_option(const char *str, file_info *p);

int main(int ac, char **av) {
	int r = EXIT_SUCCESS;
	if (ac <= 1) {
		return r;
	}
	setlocale(LC_COLLATE, "");
  hashtable *ht = hashtable_empty((int (*)(const void *, const void *))strcoll,
      (size_t (*)(const void *))str_hashfun, 1.0);
 	holdall *ha = holdall_empty();
 	holdall *ha2 = holdall_empty();
 	if (ht == NULL || ha == NULL || ha2 == NULL) {
 		goto error_capacity;
 	}
 	//buff : buffer qui stocke les lettres lus
 	char *buff = malloc(BUFF_SIZE_MIN + 1);
 	if (buff == NULL) {
 		goto error_capacity;
 	}
 	//file_num : représente le numero du fichier lu, l'adresse du début du tableau
 	//		est inutilisé pour simplifier les calculs d'indices
 	// char file_num[MAX_FILE_NUM + 1];
 	//current_file : fichier entrain d'être traité
 	//		current_file = ac - 1 - le nombre d'options
 	int current_file = 1;
 	//current_arg : argument entrain d'être traité
	int current_arg = 1;
	float _intersect = 0;
	float _union = 0;
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
		 			//clé = str, valeur = numero de fichier
		 			//si la string n'est pas dans la ht, l'ajoute et union ++
		 			file_info *p = hashtable_search(ht, buff);
		 			if (p == NULL) {
		 				p = malloc(sizeof *p);
		 				if (p == NULL) {
		 					goto error_capacity;
		 				}
		 				p->file_nums = malloc(sizeof *p->file_nums * CAPACITY_MIN);
		 				if (p->file_nums == NULL) {
		 					goto error_capacity;
		 				}
		 				p->file_nums[0] = current_file;
		 				p->card = 1;
		 				p->capacity = CAPACITY_MIN;
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
			 			if (holdall_put(ha2, p) != 0) {
			 				goto error_capacity;
			 			}
			 			if (hashtable_add(ht, t, p) == NULL) {
			 				goto error_capacity;
			 			}
			 			++_union;
			 		}
			 		else {
			 				//si le mot est dans la HT, on regarde si il provient du même
			 				//	fichier, si non, inter++ et on alloue une cellule
			 				if (is_word_from_diff_file(p, current_file)) {
			 					++_intersect;
			 					p->file_nums[p->card] = current_file;
			 					p->card += 1;
			 					hashtable_add(ht, buff, p);
			 				}
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
	 	++current_file;
	}

 	printf("\t");
 	for (int i = 1; i < ac; ++i) {
 		printf("%s\t", av[i]);
 	}
 	printf("\n");

	//affiche les mots qui ont été lu
 	// holdall_apply(ha, (int (*)(void *))display);
 	if (holdall_apply_context(ha, ht,
    (void *(*)(void *, void *))hashtable_search,
    (int (*)(void *, void *))display_graph_option) != 0) {
 		goto dispose;
	}

	 	printf("intersection : %f\n", _intersect);
 	printf("union : %f\n", _union);
 	printf("distance de jacquard : %f\n", (1.0 - (_intersect / _union)));
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
		if (ha2 != NULL) {
	    holdall_apply(ha2, file_info_dispose);
	  }
	  holdall_dispose(&ha);
	  holdall_dispose(&ha2);
	  return r;
}

int file_info_dispose(void *p) {
	file_info *q = p;
	if (q != NULL) {
		free(q->file_nums);
		free(q);
	}
	return 0;
}

int display_graph_option(const char *str, file_info *p) {
	printf("%s :\t", str);
	for (size_t i = 0; i < p->card; ++i) {
		for (int j = 1; j < p->file_nums[i]; ++j) {
			printf("\t");
		}
		printf("%d", p->file_nums[i]);
	}
	printf("\n");
	return 0;
}

bool is_word_from_diff_file(file_info *p, int current_file) {
	for (size_t i = 0; i < p->card; ++i) {
		if (p->file_nums[i] == current_file) {
			return false;
		}
	}
	return true;
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
	si le mot n'est pas dans la HT : 
		union++
		on stocke dans la HT : (mot, numero du fichier)
	sinon
		si le mot est dans la HT : 
				intersection++
			on l'ajoute pas à la HT

	exemple : 
		lecture de "a b c a d" et "b f d a"
		fichier		mot  	union		intersection 	ht
		1					a 		1 			0							a	
		1					b 		2 			0							a b
		1					c 		3 			0							a b c
		1					a 		3 			0							a b c
		1 				d 		4 			0 						a b c d
		2 				b 		4 			1 						a b c d
		2 				f 		5 			1 						a b c d f
		2 				d 		5 			2 						a b c d f
		2 				a 		5 			3 						a b c d f

		distance de jacquard : 1 - (inter / union) = 1 - (3 / 5) = 0,4


	erreur dans l'algo :
		le programme doit pouvoir afficher quel mot apartient à quel fichiers
		donc il faut sauvegarder le mot et chaque fichiers dans le quel il apparait

		idée :
			-un tableau de bool en guise de valeur : ou chaque case correspond à un 
			fichier. mais c'est couteux, ça veux dire dans chaque mot -> chaque case
			de la HT y'aura un tableau du nombre de fichier ? ça fait bcp .. genre pour
			les misérables c'est pas possible quoi

			-une liste chainée ou chaque cellule contient un numero de fichier
			je pense que c'est la bonne solution



IDEE : si l'option -g activé on active la structure avec tableau dynamique, sinon
non bah on met rien et on economise de la place
	
*/
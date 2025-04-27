#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <ctype.h>
#include "hashtable.h"
#include "holdall.h"

#define BUFF_SIZE 100

#define INPUT_DIRECT "-"
#define INPUT_FILE_NEXT "--"

#define CAPACITY_MIN 2
#define CAPACITY_MUL 2

typedef struct file_info file_info;
struct file_info {
  int *file_nums;
  size_t capacity;
  size_t card;
};

//jpair : stock les info utiles pour le calcul de la distance
//    de chaque paire de combinaison de fichiers
typedef struct jpair jpair;
struct jpair {
  int un;
  int in;
  float jdist;
};

//jpair_control sert à stocker le tableau des combinaisons + sa taille
//    (utile pour la passer en parametre à la fonction calc_jdist car on peu
//      pas changer le prototype de la fonction parce-qu'elle est utilisé
//      dans holdall_applycontext2)
typedef struct jpair_control jpair_control;
struct jpair_control {
  jpair *arr;
  int nfiles;
  int card;
};

//  -----------------------------MANQUANT--------------------------------------
//FAIT- Option -iVALUE : fixe la longueure maximale d'un mot à VALUE
//FAIT- Option -g : affiche le résultat graphique sous forme d'un tableau
//    - Affichage : affiche "-" pour l'absence d'un mot dans un fichier
//    - Option -h : affichage de l'aide
//    - Option -p : Affecte au symboles de ponctuation le même rôle que les
//                  caractères de la classe isspace

//    - Faire en sorte que le mot soit coupé si on atteint BUFF_SIZE
//      on doit couper et continuer de lire dans un nouveau mot

//    - Regler l'entrée au clavier ça bug
//  ----------------------- Spécifications & déclarations  --------------------

//  rfree : libère le pointeur associé à p.
int rfree(void *p);

// file_info_dispose : sans effet si p vaut nullptr, libère sinon les ressources
//    allouées à la structure des caractéristiques des mots
int file_info_dispose(void *p);

//  str_hashfun : l'une des fonctions de pré-hachage conseillées par Kernighan
//    et Pike pour les chaines de caractères.
size_t str_hashfun(const char *s);

// get_input_type : renvoie un pointeur vers l'entrée standard si l'argument
// est "-", vers un fichier si l'argument est un fichier valide, le pointeur nul
// dans le cas contraire.
FILE *get_input_type(char **av, int i);

//  renvoie vrai si l'entier current_file est présent dans le tableau de numéros
//  de fichiers du pointeur associé à p, faux sinon.
bool is_in_file(file_info *p, int current_file);

//  display_graph_option : affiche sur l'entrée standart le nom du fichier
//  associé à str ainsi que l'appartenance ou non aux différents fichiers lus et
//  renvoie 0
int display_graph_option(const char *str, file_info *p);

//  calc_jdist : calcul pour chaque combinaisons de paires de fichiers lus,
//    leur intersections, unions ainsi que le calcul de la distance de jacquard
//    renvoie 
int calc_jdist(jpair_control *q, const char *str, file_info *p);

int main(int ac, char **av) {
  int r = EXIT_SUCCESS;
  if (ac <= 2) {
    return r;
  }
  bool graph_option = false;
  long int max_word_length = -1;
  // setlocale : définit la règle de comparaison des chaînes de caractères
  // utilisée par strcoll.
  setlocale(LC_COLLATE, "");
  hashtable *ht = hashtable_empty((int (*)(const void *, const void *)) strcoll,
        (size_t (*)(const void *)) str_hashfun, 1.0);
  holdall *ha0 = holdall_empty();
  holdall *ha1 = holdall_empty();
  if (ht == nullptr || ha0 == nullptr || ha1 == nullptr) {
    goto error_capacity;
  }
  //buff : buffer qui stocke progressivement les lettres lus
  char *buff = malloc(BUFF_SIZE + 1);
  if (buff == nullptr) {
    goto error_capacity;
  }
  // current_file : représente le fichier entrain d'être traité
  // current_file = ac - 1 - le nombre d'options
  // current_arg :  représente l'argument entrain d'être traité
  int param_count = 0;
  int current_file = 1;
  int current_arg = 1;
  for (int i = 1; i < ac; i++) {
    if (strcmp(av[i], "-g") == 0) {
      graph_option = true;
      current_arg++;
      param_count++;
    } else if (strncmp(av[i], "-i", 2) == 0) {
      char *endptr;
      max_word_length = strtol(av[i] + 2, &endptr, 10);
      current_arg++;
      param_count++;
    }
  }
  while (current_arg < ac) {
    FILE *f = get_input_type(av, current_arg);
    if (f == nullptr) {
      goto error_read;
    }
    if (f == stdin) {
      printf("--- starts reading for #%d FILE\n", current_arg - param_count);
    } else {
      if (strcmp(av[current_arg], INPUT_FILE_NEXT) == 0) {
        ++current_arg;
        if (current_arg >= ac) {
          fprintf(stderr, "*** Missing file argument\n");
          goto error;
        }
      }
    }
    //i : indice du buffer
    size_t i = 0;
    //c : caractère lu
    int c = 0;
    while (c != EOF) {
      c = fgetc(f);
      // Si c'est la fin du mot, on alloue et on envoie dans le holdall & HT
      if (isspace(c) || c == EOF) {
        // On avance jusqu'au début du mot suivant
        while (isspace(c) && c != EOF) {
          c = fgetc(f);
        }
        // i > 0 assure que les chaines allouées ne sont pas vides
        if (i > 0) {
          buff[i] = '\0';
          // Si l'option -i est renseignée alors max_word_lenght est différent
          //    de -1 :
          //        - on coupe donc le mot à la longueur max_word_length
          //        - on met à jour la taille du mot coupé
          if (max_word_length != -1 && (int)i > max_word_length) {
            buff[max_word_length] = '\0';
            i = (size_t) max_word_length;
          }
          file_info *p = hashtable_search(ht, buff);
          if (p == nullptr) {
            p = malloc(sizeof *p);
            if (p == nullptr) {
              goto error_capacity;
            }
            // file_nums : tableau d'entier pour sauvegarder les fichiers
            //  dans lesquels le mot apparait
            p->file_nums = malloc(sizeof *p->file_nums * CAPACITY_MIN);
            if (p->file_nums == nullptr) {
              goto error_capacity;
            }
            p->file_nums[0] = current_file;
            p->card = 1;
            p->capacity = CAPACITY_MIN;
            char *t = malloc(i + 1);
            if (t == nullptr) {
              goto error_capacity;
            }
            strcpy(t, buff);
            if (holdall_put(ha0, t) != 0) {
              goto error_capacity;
            }
            if (holdall_put(ha1, p) != 0) {
              goto error_capacity;
            }
            if (hashtable_add(ht, t, p) == NULL) {
              goto error_capacity;
            }
          } else {
            if (p->card == p->capacity) {
              if (p->capacity * sizeof *p->file_nums > SIZE_MAX / CAPACITY_MUL) {
                goto dispose;
              }
              int *q = realloc(p->file_nums, 
                p->capacity * CAPACITY_MUL * sizeof *p->file_nums);
              if (q == NULL) {
                goto dispose;
              }
              p->file_nums = q;
              p->capacity *= CAPACITY_MUL;
            }
            //si on a déjà lu ce mot dans ce fichier, on fait rien
            //sinon, on ajoute le nouveau fichier dans lequel il apparait
            if (p->file_nums[p->card - 1] != current_file) {
              p->file_nums[p->card] = current_file;
              p->card += 1;
              hashtable_add(ht, buff, p);
            }
          }
          // Réinitialisation du buffer
          memset(buff, 0, i);
          i = 0;
        }
      }
      buff[i] = (char) c;
      ++i;
    }
    if (f != stdin) {
      if (!feof(f)) {
        goto error_read;
      }
      if (fclose(f) != 0) {
        fprintf(stderr, "*** Failed to close the file : %s\n", av[current_arg]);
        goto error;
      }
    } else {
      clearerr(f);
      printf("--- ends reading for #%d FILE\n", current_arg - param_count);
    }
    // Avance jusqu'à la prochaine entrée
    ++current_arg;
    ++current_file;
  }
  // Affichage des mots lus pour l'option graph
  if (graph_option) {
    printf("\t");
    for (int i = 2; i < ac; ++i) {
      if (strncmp(av[i], "-i", 2) == 0) {
        ++i;
      }
      printf("%s\t", av[i]);
    }
    printf("\n");
    if (holdall_apply_context(ha0, ht,
          (void *(*)(void *, void *)) hashtable_search,
          (int (*)(void *, void *)) display_graph_option) != 0) {
      goto dispose;
    }
  }
  jpair_control *jc = malloc(sizeof *jc);
  if (jc == NULL) {
    goto dispose;
  }
  //nombre de combinaison pour n fichiers = binome de newton (n(n - 1))/2
  // 2 < ac 
  jc->nfiles = ac ;
  jc->card = ((ac-1) * (ac-2))/2;
  //calloc = alloue et initialise à O toutes les cases
  jc->arr = calloc((size_t)jc->card, sizeof *jc->arr);
  if (jc->arr == NULL) {
      goto dispose;
  }
  holdall_apply_context2(ha0, ht, (void *(*)(void *, void *)) hashtable_search,
    jc, (int (*)(void *, void *, void *)) calc_jdist);
  size_t k = 0;
  for (int i = 1; i < ac; ++i) {
    for(int j = i + 1; j < ac; ++j) {
      printf("%.4f\t%s\t%s\n", jc->arr[k].jdist, av[i], av[j]);
      ++k;
    }
  }
  free(jc->arr);
  free(jc);
  free(buff);
  goto dispose;
error_read:
  fprintf(stderr, "*** Error: A read error occurs\n");
  goto error;
  /*error_write:
   * fprintf(stderr, "*** Error: A write error occurs\n");
   * goto error;
   */
error_capacity:
  fprintf(stderr, "*** Error: Not enough memory\n");
  goto error;
error:
  r = EXIT_FAILURE;
  goto dispose;
dispose:
  hashtable_dispose(&ht);
  if (ha0 != nullptr) {
    holdall_apply(ha0, rfree);
  }
  if (ha1 != nullptr) {
    holdall_apply(ha1, file_info_dispose);
  }
  holdall_dispose(&ha0);
  holdall_dispose(&ha1);
  return r;
}

//  ---------------------- Définitions --------------------------------------


int file_info_dispose(void *p) {
  file_info *q = p;
  if (q != nullptr) {
    free(q->file_nums);
    free(q);
  }
  return 0;
}

int calc_jdist(jpair_control *q, const char *str, file_info *p) {
  (void)str;
  //mettre l'option display graph ici
  size_t k = 0;
  for (int i = 1; i < (int)q->nfiles; ++i) {
    for (int j = i+1; j < (int)q->nfiles; ++j) {
      if (is_in_file(p, i)) {
        if (is_in_file(p, j)) {
            q->arr[k].in +=1;
        }
        q->arr[k].un +=1;
      }
      else if (is_in_file(p, j)) {
        q->arr[k].un += 1;
      }
      q->arr[k].jdist = 1 - ((float)q->arr[k].in/(float)q->arr[k].un);
      ++k;
    }
  }
  return 0;
}


int display_graph_option(const char *str, file_info *p) {
  printf("%s :\t", str);
  for (size_t i = 0; i < p->card; ++i) {
    for (int j = 1; j < p->file_nums[i]; ++j) {
      printf("\t");
    }
    printf("x");
  }
  printf("\n");
  return 0;
}

bool is_in_file(file_info *p, int current_file) {
  for (size_t i = 0; i < p->card; ++i) {
    if (p->file_nums[i] == current_file) {
      return true;
    }
  }
  return false;
}


FILE *get_input_type(char **av, int i) {
  // Dans le cas du tiret simple ouvre l'entrée standard.
  if (strcmp(av[i], INPUT_DIRECT) == 0) {
    return stdin;
  }
  //si un fichier suit, incremente, et ouvre le fichier
  if (strcmp(av[i], INPUT_FILE_NEXT) == 0) {
    ++i;
  }
  FILE *f = fopen(av[i], "r");
  if (f == nullptr) {
    fprintf(stderr, "*** Failed to open the file : %s\n", av[i]);
    return nullptr;
  }
  return f;
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
 * si le mot n'est pas dans la HT :
 *  union++
 *  on stocke dans la HT : (mot, numero du fichier)
 * sinon
 *  si le mot est dans la HT :
 *      intersection++
 *    on l'ajoute pas à la HT
 *
 * exemple :
 *  lecture de "a b c a d" et "b f d a"
 *  fichier   mot   union   intersection  ht
 *  1         a     1       0             a
 *  1         b     2       0             a b
 *  1         c     3       0             a b c
 *  1         a     3       0             a b c
 *  1         d     4       0             a b c d
 *  2         b     4       1             a b c d
 *  2         f     5       1             a b c d f
 *  2         d     5       2             a b c d f
 *  2         a     5       3             a b c d f
 *
 *  distance de jacquard : 1 - (inter / union) = 1 - (3 / 5) = 0,4
 *
 *
 */

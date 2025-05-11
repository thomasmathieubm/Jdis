/*---------------------------FONCTIONNEMENT GENERAL-----------------------------
  Les fichiers sont lus un à un avec fgetc, les lettres sont stockés dans un
  buffer initialement de taille 2, à chaque fois que le buffer est plein, 
  sa taille double, assurant la lecture de mots de taille non définis.
  Quand un espace (ou la fin du fichier) est lu, le mot est considéré comme
  terminé. 

  On cherche alors dans la table de hashage si il est déjà présent, 
  si il l'est on regarde si il provient du fichier qu'on est entrain de lire
  si c'est le cas on ne fait rien, 
  sinon on ajoute ce nouveau fichier au tableau file_nums, qui sauvegarde tout 
  les numéros de fichiers dans lesquels le mot apparait, assurant alors que 
  chaque mot est stocké de manière unique.
  Quand la fin du fichier est lu, le programme ferme ce dernier et passe au 
  prochain si il y en a.

  Une fois tout les fichiers lu, la distance de jacquard de chacunes des paires
  de fichiers est calculé : en parcourant le fourretout des mots, on recherche
  chaque mot dans la table de hashage et on regarde si ils sont present dans
  les fichiers lu.
  Les resultats sont sauvegardés dans un tableau (jc->arr) , ou chaque case
  represente une combinaison de fichiers.
  Ce tableau est ensuite parcouru pour afficher chacunes des distances.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include "hashtable.h"
#include "holdall.h"

#define EXENAME (av[0] + 2)

#define ILLEGAL_FILENAME ""

// NPAIRS : nombre de combinaisons de paires de fichiers pour n fichiers
#define N_PAIRS(n) ((n) * ((n) - 1)) / 2;

#define FILE_MAX_COUNT 64
#define FILE_MIN_COUNT 2

#define BUFF_SIZE_MIN 2
#define BUFF_SIZE_MUL 2

#define CAPACITY_MIN 2
#define CAPACITY_MUL 2

#define INPUT_DIRECT "-"
#define INPUT_FILE_NEXT "--"
#define INPUT_DIRECT_NAME_FILE "\"\""

#define OPT_SHORT "-"
#define OPT_LONG OPT_SHORT OPT_SHORT
#define OPT_HELP OPT_SHORT "?"
#define OPT_USAGE OPT_LONG "usage"
#define OPT_VERSION OPT_LONG "version"
#define OPT_GRAPH OPT_SHORT "g"
#define OPT_I OPT_SHORT "i"
#define OPT_PUNCT OPT_SHORT "p"

//file_info : enregistre les numéros des fichiers dans lesquel chaques mots
//    apparaissent dans le tableau file_nums
typedef struct file_info file_info;
struct file_info {
  int *file_nums;
  size_t capacity;
  size_t card;
};

//jpair : enregistre les informations utiles pour le calcul de la distance
//    de chaque paire de combinaison de fichiers
typedef struct jpair jpair;
struct jpair {
  float un;
  float in;
  float jdist;
};

//jpair_control : les informations de chaque paires de fichiers sont stocké dans
//  arr, nfiles correspond au nombre total de fichiers lu, card au nombre de 
//  combinaisons pour nfiles fichiers
typedef struct jpair_control jpair_control;
struct jpair_control {
  jpair *arr;
  int nfiles;
  int card;
};

//  ------------------- Spécifications & déclarations  ------------------------

//  rfree : libère le pointeur associé à p.
int rfree(void *p);

// file_info_dispose : sans effet si p vaut nullptr, libère sinon les ressources
//    allouées à la structure des caractéristiques des mots
int file_info_dispose(void *p);

//  str_hashfun : l'une des fonctions de pré-hachage conseillées par Kernighan
//    et Pike pour les chaines de caractères.
size_t str_hashfun(const char *s);

//  renvoie vrai si l'entier current_file est présent dans le tableau de numéros
//  de fichiers du pointeur associé à p, faux sinon.
bool is_in_file(file_info *p, int current_file);

//  compute_jdist : calcul pour chaque combinaisons de paires de fichiers lus,
//    leur intersections, unions, ainsi que la distance de jacquard, renvoie 0
int compute_jdist(jpair_control *q, const char *str, file_info *p);

//  display_graph_option : affiche sur l'entrée standard le nom du fichier
//  associé à str ainsi que l'appartenance ou non aux différents fichiers lus et
//  renvoie 0
int display_graph_option(int *nfiles, const char *str, file_info *p);

//  print_help : affiche sur l'entrée standard l'aide du programme.
void print_help(const char *execname);

//  print_usage : affiche sur l'entrée standard un message court sur 
//    l'utilisation du programme.
void print_usage(const char *execname);

//  print_version : affiche sur l'entrée standard la version du programme.
void print_version(const char *execname);

//  --------------------------------------------------------------------------

int main(int ac, char **av) {
  int r = EXIT_SUCCESS;
  setlocale(LC_COLLATE, "");
  const char **file_list = NULL;
  char *buff = NULL;
  hashtable *ht = hashtable_empty((int (*)(const void *, const void *))strcoll,
      (size_t (*)(const void *))str_hashfun, 1.0);
  holdall *ha0 = holdall_empty();
  holdall *ha1 = holdall_empty();
  if (ht == nullptr || ha0 == nullptr || ha1 == nullptr) {
    goto error_capacity;
  }
  bool g_opt = false;
  bool p_opt = false;
  long int i_opt = 0;
  int nfiles = 0;
  file_list = malloc((size_t) (ac - 1) * sizeof *file_list);
  if (file_list == nullptr) {
    goto error_capacity;
  }
  // -----detection des options & fichiers -----//
  for (int i = 1; i < ac; ++i) {
    const char *arg = av[i];
    if (strcmp(arg, ILLEGAL_FILENAME) == 0) {
      fprintf(stderr, "%s: Illegal filename ''.\n", EXENAME);
      goto error_arg;
    } else if (strncmp(arg, OPT_SHORT, 1) == 0) {
      if (strcmp(arg, OPT_GRAPH) == 0) {
        g_opt = true;
      } else if (strcmp(arg, OPT_PUNCT) == 0) {
        p_opt = true;
      } else if (strcmp(arg, OPT_HELP) == 0) {
        print_help(EXENAME);
        goto dispose;
      } else if (strncmp(arg, OPT_I, strlen(OPT_I)) == 0) {
        const char *num_str = arg + strlen(OPT_I);
        errno = 0;
        i_opt = strtol(num_str, nullptr, 10);
        if (*num_str == '\0' || i_opt < 0 || errno != 0) {
          if (errno == ERANGE) {
            fprintf(stderr, "%s: Overflowing argument '%s'.\n", EXENAME, arg);
          }
          else {
            fprintf(stderr, "%s: Invalid argument '%s'.\n", EXENAME, arg);
          }
          goto error_arg;
        }
      } else if (strcmp(arg, OPT_USAGE) == 0) {
        print_usage(EXENAME);
        goto dispose;
      } else if (strcmp(arg, OPT_VERSION) == 0) {
        print_version(EXENAME);
        goto dispose;
      } else if (strcmp(arg, INPUT_FILE_NEXT) == 0) {
        if (i + 1 >= ac) {
          fprintf(stderr, "%s: Missing filename after '%s'.\n", EXENAME,
              INPUT_FILE_NEXT);
          goto error_arg;
        }
        ++i;
        file_list[nfiles] = av[i];
        ++nfiles;
      } else if (strcmp(arg, INPUT_DIRECT) == 0) {
        file_list[nfiles] = INPUT_DIRECT_NAME_FILE;
        ++nfiles;
      } else {
        fprintf(stderr, "%s: Unrecognized option in expression '%s'.\n",
            EXENAME, arg);
        goto error_arg;
      }
    } else {
      file_list[nfiles] = arg;
      ++nfiles;
    }
  }
  if (nfiles < FILE_MIN_COUNT) {
    fprintf(stderr, "%s: Missing operand.\n", EXENAME);
    goto error_arg;
  }
  if (nfiles > FILE_MAX_COUNT) {
    fprintf(stderr, "%s: Too many operands.\n",
        EXENAME);
    goto error_arg;
  }
  buff = malloc(BUFF_SIZE_MIN + 1);
  if (buff == nullptr) {
    goto error_capacity;
  }
  size_t buffsize = BUFF_SIZE_MIN;
  for (int file_idx = 0; file_idx < nfiles; ++file_idx) {
    int nline = 1;
    FILE *f = nullptr;
    if (strcmp(file_list[file_idx], INPUT_DIRECT_NAME_FILE) == 0) {
      f = stdin;
      printf("--- starts reading for #%d FILE\n", file_idx + 1);
    } else {
      f = fopen(file_list[file_idx], "r");
      if (f == nullptr) {
        fprintf(stderr, "%s: Can't open for reading file '%s'\n",
            EXENAME, file_list[file_idx]);
        goto error;
      }
    }
    size_t i = 0;
    int c = 0;
    //-----lectures des fichiers-----//
    while (c != EOF) {
      if (i == buffsize) {
        if (buffsize * sizeof *buff > SIZE_MAX
            / BUFF_SIZE_MUL) {
          goto error_capacity;
        }
        char *b = realloc(buff, buffsize * BUFF_SIZE_MUL);
        if (b == nullptr) {
          goto error_capacity;
        }
        buff = b;
        buffsize *= BUFF_SIZE_MUL;
      }
      c = fgetc(f);
      // Si c'est la fin du mot, on alloue et on envoie dans le holdall & HT
      if (isspace(c) || c == EOF || (p_opt && ispunct(c))
          || (i_opt > 0 && (int) i == i_opt)) {
        buff[i] = '\0';
        if (i_opt > 0 && (int) i == i_opt) {
          if (!isspace(c) && c != EOF) {
            fprintf(stderr, "%s: Word from file '%s' at line %d cut: '%s...'\n",
                EXENAME, file_list[file_idx], nline, buff);
          }
          //si on a coupé le mot : on passe jusqu'a la fin de ce dernier
          while (!isspace(c) && c != EOF) {
            if (p_opt && ispunct(c)) {
              break;
            }
            c = fgetc(f);
          }
        }
        // On avance jusqu'au début du mot suivant
        while ((isspace(c) || (p_opt && ispunct(c))) && c != EOF) {
          if (c == '\n') {
            ++nline;
          }
          c = fgetc(f);
        }
        // i > 0 assure que les chaines allouées ne sont pas vides
        if (i > 0) {
          file_info *p = hashtable_search(ht, buff);
          if (p == nullptr) {
            p = malloc(sizeof *p);
            if (p == nullptr) {
              goto error_capacity;
            }
            // file_nums : tableau d'entiers pour sauvegarder les fichiers
            //  dans lesquels le mot apparait
            p->file_nums = malloc(sizeof *p->file_nums * CAPACITY_MIN);
            if (p->file_nums == nullptr) {
              goto error_capacity;
            }
            p->file_nums[0] = file_idx + 1;
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
            if (hashtable_add(ht, t, p) == nullptr) {
              goto error_capacity;
            }
          } else {
            if (p->card == p->capacity) {
              if (p->capacity * sizeof *p->file_nums > SIZE_MAX
                  / CAPACITY_MUL) {
                goto dispose;
              }
              int *q = realloc(p->file_nums,
                  p->capacity * CAPACITY_MUL * sizeof *p->file_nums);
              if (q == nullptr) {
                goto dispose;
              }
              p->file_nums = q;
              p->capacity *= CAPACITY_MUL;
            }
            //si on a déjà lu ce mot dans ce fichier, on fait rien
            //sinon, on ajoute le nouveau fichier dans lequel il apparait
            if (p->file_nums[p->card - 1] != file_idx + 1) {
              p->file_nums[p->card] = file_idx + 1;
              p->card += 1;
              hashtable_add(ht, buff, p);
            }
          }
          // Réinitialisation du buffer
          memset(buff, 0, i);
          i = 0;
        }
      }
      buff[i] = (char)c;
      ++i;
    }
    if (!feof(f)) {
      goto error_read;
    }
    if (f != stdin) {
      if (fclose(f) != 0) {
        fprintf(stderr, "%s: Failed to close the file : %s\n",
            EXENAME, file_list[file_idx]);
        goto error;
      }
    }
    else {
      clearerr(f);
      printf("--- ends reading for #%d FILE\n", file_idx + 1);
    }
  }
  // Affichage des mots lus pour l'option graph
  if (g_opt) {
    printf("\t");
    for (int i = 0; i < nfiles; i++) {
      printf("%s\t", file_list[i]);
    }
    printf("\n");
    holdall_sort(ha0, (int (*)(const void *, const void *))strcoll);
    if (holdall_apply_context2(ha0, ht,
        (void *(*)(void *, void *))hashtable_search,
        &nfiles, (int (*)(void *, void *, void *))display_graph_option) != 0) {
      goto error_write;
    }
  }
  // ----- calcul et affichage de la distance -----
  else {
    jpair_control *jc = malloc(sizeof *jc);
    if (jc == nullptr) {
      goto error_capacity;
    }
    //jc->nfiles = nombre de fichiers qu'on a a traiter
    //nfiles + 1 car nfiles est initialisé à 0
    jc->nfiles = nfiles + 1;
    //jc->card = nombre de combinaisons de paires de fichiers
    jc->card = N_PAIRS(nfiles);
    jc->arr = calloc((size_t) jc->card, sizeof *jc->arr);
    if (jc->arr == nullptr) {
      goto error_capacity;
    }
    if (holdall_apply_context2(ha0, ht, (void *(*)(void *, void *))hashtable_search,
        jc, (int (*)(void *, void *, void *))compute_jdist) != 0) {
      goto error_write;
    }
      size_t k = 0;
      for (int i = 1; i <= nfiles; i++) {
        for (int j = i + 1; j <= nfiles; j++) {
          printf("%.4f\t%s\t%s\n", jc->arr[k].jdist, file_list[i - 1],
              file_list[j - 1]);
          ++k;
        }
      }
    free(jc->arr);
    free(jc);
  }
  goto dispose;
error_arg:
  fprintf(stderr, "Try '%s %s' for more information.\n", EXENAME, OPT_HELP);
  goto error;
error_read:
  fprintf(stderr, "%s: A read error occurs.\n", EXENAME);
  goto error;
error_write:
  fprintf(stderr, "%s: A write error occurs.\n", EXENAME);
  goto error;
error_capacity:
  fprintf(stderr, "%s: Not enough memory.\n", EXENAME);
  goto error;
error:
  r = EXIT_FAILURE;
  goto dispose;
dispose:
  if (buff != NULL) {
    free(buff);
  }
  if (file_list != NULL) {
    free(file_list);
  }
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
//------------------------- autres fonctions -----------------------------

int rfree(void *p) {
  free(p);
  return 0;
}

int file_info_dispose(void *p) {
  file_info *q = p;
  if (q != nullptr) {
    free(q->file_nums);
    free(q);
  }
  return 0;
}

size_t str_hashfun(const char *s) {
  size_t h = 0;
  for (const unsigned char *p = (const unsigned char *) s; *p != '\0'; ++p) {
    h = 37 * h + *p;
  }
  return h;
}

bool is_in_file(file_info *p, int current_file) {
  for (size_t i = 0; i < p->card; ++i) {
    if (p->file_nums[i] == current_file) {
      return true;
    }
  }
  return false;
}

int display_graph_option(int *nfiles, const char *str, file_info *p) {
  printf("%s\t", str);
  for (int i = 0; i < *nfiles; ++i) {
    is_in_file(p, i + 1) ? printf("x\t") : printf("-\t");
  }
  printf("\n");
  return 0;
}

int compute_jdist(jpair_control *q, [[maybe_unused]]const char *str, 
  file_info *p) {
  size_t k = 0;
  for (int i = 1; i < q->nfiles; ++i) {
    for (int j = i + 1; j < q->nfiles; ++j) {
      if (is_in_file(p, i)) {
        if (is_in_file(p, j)) {
          q->arr[k].in += 1;
        }
        q->arr[k].un += 1;
      } else if (is_in_file(p, j)) {
        q->arr[k].un += 1;
      }
      if (q->arr[k].un == 0) {
        q->arr[k].jdist = 0;
      } else {
        q->arr[k].jdist = 1 - (q->arr[k].in /  q->arr[k].un);
      }
      ++k;
    }
  }
  return 0;
}

void print_help(const char *execname) {
  printf("Usage: ");
  print_usage(execname);
  printf("\n"
      "Computes Jaccard dissimilarities of sets of words in FILEs.\n\n"
      "For any pair of FILEs, dissimilarity is displayed first to four decimal "
      "places,\n"
      "followed by the two FILEs in the pair. A word is, by default, a maximum "
      "length\n"
      "sequence of characters that do not belong to the white-space characters "
      "set.\n\n"
      "Read the standard input for any FILE that is '%s' on command line. The "
      "standard\n"
      "input is displayed as a pair of double quotation marks in productions."
      "\n\n"
      "Program Information\n"
      "  %s\n"
      "        Print this help message and exit.\n\n"
      "  %s\n"
      "        Print a short usage message and exit.\n\n"
      "  %s\n"
      "        Print version information.\n\n"
      "Input Control\n"
      "  %s VALUE\n"
      "        Set the maximal number of significant initial letters for words "
      "to\n"
      "        VALUE. 0 means without limitation. Default is 0.\n\n"
      "  %s\n"
      "        Make the punctuation characters play the same role as white-"
      "space\n"
      "        characters in the meaning of words.\n\n"
      "Output Control\n"
      "  %s\n"
      "        Suppress normal output. Instead, for each word found in any "
      "FILE, jdis\n"
      "        lists the FILEs in which it does or does not appear. A header "
      "line\n"
      "        indicates the FILE names: the name of the first FILE appears in "
      "the\n"
      "        second column, that of the second in the third, and so on. For "
      "the\n"
      "        subsequent lines, a word appears in the first column, followed "
      "by\n"
      "        appearance marks: 'x' for yes, '-' for no. The list is "
      "lexicographically\n"
      "        sorted. The locale specified by the environment affects the sort "
      "order.\n"
      "        Set 'LC_ALL=C' or 'LC_COLLATE=C' to get the traditional sort "
      "order that\n"
      "        uses native byte values.\n\n"
      "White-space and punctuation characters conform to the standard. At most "
      "64 FILEs\n"
      "are supported.\n"
      , INPUT_DIRECT, OPT_HELP, OPT_USAGE, OPT_VERSION, OPT_I, OPT_PUNCT, 
      OPT_GRAPH);
}

void print_usage(const char *execname) {
  printf("%s [OPTION]... FILE1 FILE2 [FILE]...\n", execname);
}

void print_version(const char *execname) {
  printf("%s - 06.05.1\nThis is a freeware: you can redistribute it. There"
      "is NO WARRANTY.\n", execname);
}

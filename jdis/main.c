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
#define SHORT_OPT "-"
#define HELP SHORT_OPT "?"
#define GRAPHIC_OPTION SHORT_OPT "g"
#define I_OPTION SHORT_OPT "i"
#define PUNCT_OPTION SHORT_OPT "p"


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
//    - Affichage : affiche "-" pour l'absence d'un mot dans un fichier
//    - Option -p : Affecte au symboles de ponctuation le même rôle que les
//                  caractères de la classe isspace
// buff size ne doit pas être limité il doit être dynamique

// Pour tester faire diff sur les sorties avec l'executable du prof
//  ----------------------- Spécifications & déclarations  --------------------

//  rfree : libère le pointeur associé à p.
int rfree(void *p) {
  free(p);
  return 0;
}

// file_info_dispose : sans effet si p vaut nullptr, libère sinon les ressources
//    allouées à la structure des caractéristiques des mots
int file_info_dispose(void *p) {
  file_info *q = p;
  if (q != nullptr) {
    free(q->file_nums);
    free(q);
  }
  return 0;
}

//  str_hashfun : l'une des fonctions de pré-hachage conseillées par Kernighan
//    et Pike pour les chaines de caractères.
size_t str_hashfun(const char *s) {
  size_t h = 0;
  for (const unsigned char *p = (const unsigned char *) s; *p != '\0'; ++p) {
    h = 37 * h + *p;
  }
  return h;
}

// get_input_type : renvoie un pointeur vers l'entrée standard si l'argument
// est "-", vers un fichier si l'argument est un fichier valide, le pointeur nul
// dans le cas contraire.
/*FILE *get_input_type(char **av, int i) {
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
}*/

//  renvoie vrai si l'entier current_file est présent dans le tableau de numéros
//  de fichiers du pointeur associé à p, faux sinon.
bool is_in_file(file_info *p, int current_file) {
  for (size_t i = 0; i < p->card; ++i) {
    if (p->file_nums[i] == current_file) {
      return true;
    }
  }
  return false;
}

//  display_graph_option : affiche sur l'entrée standart le nom du fichier
//  associé à str ainsi que l'appartenance ou non aux différents fichiers lus et
//  renvoie 0
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

//  calc_jdist : calcul pour chaque combinaisons de paires de fichiers lus,
//    leur intersections, unions ainsi que le calcul de la distance de jacquard
//    renvoie
int calc_jdist(jpair_control *q, const char *str, file_info *p) {
  (void) str;
  //mettre l'option display graph ici
  size_t k = 0;
  for (int i = 1; i < (int) q->nfiles; ++i) {
    for (int j = i + 1; j < (int) q->nfiles; ++j) {
      if (is_in_file(p, i)) {
        if (is_in_file(p, j)) {
          q->arr[k].in += 1;
        }
        q->arr[k].un += 1;
      } else if (is_in_file(p, j)) {
        q->arr[k].un += 1;
      }
      q->arr[k].jdist = 1 - ((float) q->arr[k].in / (float) q->arr[k].un);
      ++k;
    }
  }
  return 0;
}

void print_help(void) {
    printf("Usage: jdis [OPTION]... FILE1 FILE2 [FILE]...\n\n");
    printf("Computes Jaccard dissimilarities of sets of words in FILEs.\n\n");
    printf("For any pair of FILEs, dissimilarity is displayed first to four decimal places,\n");
    printf("followed by the two FILEs in the pair. A word is, by default, a maximum length\n");
    printf("sequence of characters that do not belong to the white-space characters set.\n\n");
    printf("Read the standard input for any FILE that is '-' on command line. The standard\n");
    printf("input is displayed as a pair of double quotation marks in productions.\n\n");
    printf("Program Information\n");
    printf("  -?, --help\n");
    printf("        Print this help message and exit.\n\n");
    printf("  --usage\n");
    printf("        Print a short usage message and exit.\n\n");
    printf("  --version\n");
    printf("        Print version information.\n\n");
    printf("Input Control\n");
    printf("  -i VALUE, --initial=VALUE\n");
    printf("        Set the maximal number of significant initial letters for words to\n");
    printf("        VALUE. 0 means without limitation. Default is 0.\n\n");
    printf("  -p, --punctuation-like-space\n");
    printf("        Make the punctuation characters play the same role as white-space\n");
    printf("        characters in the meaning of words.\n\n");
    printf("Output Control\n");
    printf("  -g, --graph\n");
    printf("        Suppress normal output. Instead, for each word found in any FILE, jdis\n");
    printf("        lists the FILEs in which it does or does not appear. A header line\n");
    printf("        indicates the FILE names: the name of the first FILE appears in the\n");
    printf("        second column, that of the second in the third, and so on. For the\n");
    printf("        subsequent lines, a word appears in the first column, followed by\n");
    printf("        appearance marks: 'x' for yes, '-' for no. The list is lexicographically\n");
    printf("        sorted. The locale specified by the environment affects the sort order.\n");
    printf("        Set 'LC_ALL=C' or 'LC_COLLATE=C' to get the traditional sort order that\n");
    printf("        uses native byte values.\n\n");
    printf("File Selection\n");
    printf("  -P LIST, --path=LIST\n");
    printf("        Specify a list of directories in which to search for any FILE if it is\n");
    printf("        not present in the current directory. In LIST, directory names are\n");
    printf("        separated by colons. The order in which directories are listed is the\n");
    printf("        order followed by jdis in its search.\n\n");
    printf("White-space and punctuation characters conform to the standard. At most 64 FILEs\n");
    printf("are supported.\n");
}

int main(int ac, char **av) {
  int r = EXIT_SUCCESS;
  if (ac < 2) {
    fprintf(stderr, "***Invalid number of input argument \n");
    fprintf(stderr, "Try 'jdis -h' for more information.\n");
    return r;
  }

  setlocale(LC_COLLATE, "");

  //  g_opt : true ou false selon que l'option graphique est détéctée ou non
  //  i_opt : -1 ou entier strictement positif selon que l'option i soit détéctée
  //    ou non.
  bool g_opt = false;
  bool p_opt = false;
  long int i_opt = -1;

  //  Tableaux pour stocker uniquement les fichiers à traiter, ac - 1 est la
  //    taille maximale car on retire le nom de l'executable de la liste.
  int nfiles = 0;
  char **file_list = malloc((size_t)(ac - 1) * sizeof *file_list);
  if (file_list == nullptr) {
    goto error_capacity;
  }

  //  Traitement des eventuelles options et des fichiers
  bool input_file_next = false;
  for (int i = 1; i < ac; i++) {
    //  Cas spécifique suivant un "--" :
    if (input_file_next) {
      file_list[nfiles++] = av[i];
      input_file_next = false;
    } else if (av[i][0] == '-') {
      if (av[i][1] == '\0') {
        //  Si il n'y a pas de "--" devant alors le "-" unique est traité
        //    tel un fichier.
        file_list[nfiles++] = av[i];
      } else if (av[i][1] == '-' && av[i][2] == '\0') {
        //  Si on rencontre "--" on active un booleen qui permet de signaler
        //    sur le prochain tour de boucle qu'il faut lire en mode fichier
        input_file_next = true;
      } else {
        //  Sinon c'est une option
        //  Option -h
        if (strcmp(av[i], "HELP") == 0) {
          print_help();
          return r;
          //  Option -g
        } else if (strcmp(av[i], GRAPHIC_OPTION) == 0) {
          g_opt = true;
          //  Option -p
        } else if (strcmp(av[i], PUNCT_OPTION) == 0) {
          p_opt = true;
          //  Option -iVALUE
        } else if (strncmp(av[i], I_OPTION, 2) == 0) {
          char *endptr;
          i_opt = strtol(av[i] + 2, &endptr, 10);
          if (*endptr != '\0') {
            fprintf(stderr, "Invalid value for -i option: %s\n", av[i]);
            fprintf(stderr, "Try 'jdis --help' for more information.\n");
            return r;
          }
        } else {
          // Option non reconnue
          fprintf(stderr, "jdis: Unrecognized option '%s'.\n", av[i]);
          fprintf(stderr, "Try 'jdis -h' for more information.\n");
          return r;
        }
      }
    } else {
      //  Si ca ne suit pas un "--" et que ca ne commence pas par "-" alors
      //    c'est un fichier.
      file_list[nfiles++] = av[i];
    }
  }

  //  On teste si le nombre de fichiers entrés est bien supérieur à 2 car le
  //    seul test sur le nombre d'agrument est insuffisant.
  if (nfiles < 2) {
    fprintf(stderr, "Number of input files must be upper than 2\n");
    return r;
  }

  // Initialisation des structures et du buffer seulement après avoir traité
  //    les options et les fichiers afin d'éviter des déclarations inutiles en
  //    en cas d'erreur en amont.
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


  for (int file_idx = 0; file_idx < nfiles; file_idx++) {
    FILE *f = nullptr;
    if (strcmp(file_list[file_idx], INPUT_DIRECT) == 0) {
      f = stdin;
      printf("--- starts reading for #%d FILE\n", file_idx + 1);
    } else {
      f = fopen(file_list[file_idx], "r");
      if (f == nullptr) {
        fprintf(stderr, "*** Failed to open the file : %s\n",
                file_list[file_idx]);
        goto error_read;
      }
    }

    //  i : indice du buffer
    size_t i = 0;
    int c = 0;
    while (c != EOF) {
      c = fgetc(f);
      // Si c'est la fin du mot, on alloue et on envoie dans le holdall & HT
      if (isspace(c) || c == EOF || (p_opt && ispunct(c))) {
        // On avance jusqu'au début du mot suivant
        while ((isspace(c) && c != EOF) || (p_opt && ispunct(c))) {
          c = fgetc(f);
        }
        // i > 0 assure que les chaines allouées ne sont pas vides
        if (i > 0) {
          // Si l'option -i est renseignée alors max_word_lenght est différent
          //    de -1 :
          //        - on coupe donc le mot à la longueur max_word_length
          //        - on met à jour la taille du mot coupé
          if (i_opt != -1 && (int)i > i_opt) {
            buff[i_opt] = '\0';
            i = (size_t) i_opt;
          } else {
            buff[i] = '\0';
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
              if (p->capacity * sizeof *p->file_nums > SIZE_MAX /
                CAPACITY_MUL) {
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
              p->file_nums[p->card++] = file_idx + 1;
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

    if (f != stdin) {
      if (!feof(f)) {
        goto error_read;
      }
      if (fclose(f) != 0) {
        fprintf(stderr, "*** Failed to close the file : %s\n",
          file_list[file_idx]);
        goto error;
      }
    } else {
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
    if (holdall_apply_context(ha0, ht,
          (void *(*)(void *, void *)) hashtable_search,
          (int (*)(void *, void *)) display_graph_option) != 0) {
      goto dispose;
    }
  }

  jpair_control *jc = malloc(sizeof *jc);
  if (jc == nullptr) {
    goto dispose;
  }
  jc->nfiles = nfiles + 1;
  jc->card = (nfiles * (nfiles - 1)) / 2;
  jc->arr = calloc((size_t)jc->card, sizeof *jc->arr);
  if (jc->arr == nullptr) {
    goto dispose;
  }

  holdall_apply_context2(ha0, ht, (void *(*)(void *, void *)) hashtable_search,
    jc, (int (*)(void *, void *, void *)) calc_jdist);

  if (!g_opt) {
    size_t k = 0;
    for (int i = 1; i <= nfiles; i++) {
      for (int j = i + 1; j <= nfiles; j++) {
        printf("%.4f\t%s\t%s\n", jc->arr[k].jdist, file_list[i-1], file_list[j-1]);
        ++k;
      }
    }
  }

// Désallocation :

  free(jc->arr);
  free(jc);
  free(file_list);
  free(buff);
  goto dispose;

error_read:
  fprintf(stderr, "*** Error: A read error occurs\n");
  goto error;

error_capacity:
  fprintf(stderr, "*** Error: Not enough memory\n");
  goto error;

error:
  r = EXIT_FAILURE;
  goto dispose;
dispose:
  hashtable_dispose(&ht);
  if (ha0 != NULL) {
    holdall_apply(ha0, rfree);
  }
  if (ha1 != NULL) {
    holdall_apply(ha1, file_info_dispose);
  }
  holdall_dispose(&ha0);
  holdall_dispose(&ha1);
  return r;
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

#!/bin/bash

EXEC="./jdis"
EXEC2="./jdisprof"
FICHIERS_EXISTANTS=(toto0.txt toto1.txt toto2.txt toto3.txt)
FICHIERS_FAUX=("")
OPTIONS_VALIDES=("-g" "-p" "-i1" "-i42")
OPTIONS_FAUSSES=("-i-2" "-i" "--i")

# Crée les fichiers valides s'ils n'existent pas
for f in "${FICHIERS_EXISTANTS[@]}"; do
  echo "Contenu test pour $f" > "$f"
done

# Paramètre du timeout en secondes (ici 5 secondes)
TIMEOUT=1

for run in {1..20}; do
  echo "Test #$run"

  CMD="$EXEC"
  CMD2="$EXEC2"

  # Ajouter un mélange aléatoire d'options valides et invalides
  for i in $(seq 1 $((RANDOM % 3 + 1))); do
    if (( RANDOM % 2 == 0 )); then
      opt="${OPTIONS_VALIDES[RANDOM % ${#OPTIONS_VALIDES[@]}]}"
    else
      opt="${OPTIONS_FAUSSES[RANDOM % ${#OPTIONS_FAUSSES[@]}]}"
    fi
    CMD+=" $opt"
    CMD2+=" $opt"
  done

  # Choisir un nombre aléatoire de fichiers, entre 2 et 5
  NB_FILES=$((RANDOM % 4 + 2))

  for i in $(seq 1 $NB_FILES); do
    if (( RANDOM % 2 == 0 )); then
      file="${FICHIERS_EXISTANTS[RANDOM % ${#FICHIERS_EXISTANTS[@]}]}"
    else
      file="${FICHIERS_FAUX[RANDOM % ${#FICHIERS_FAUX[@]}]}"
    fi
    CMD+=" $file"
    CMD2+=" $file"
  done

  echo "Commande: $CMD"

  # Lancer la commande avec un timeout de 5 secondes
  timeout $TIMEOUT $CMD
  echo " "
  timeout $TIMEOUT $CMD2


  echo "--------------------------"
done

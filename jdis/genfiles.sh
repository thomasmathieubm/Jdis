#!/bin/bash

# Dossier de destination (optionnel)
mkdir -p test_files
cd test_files || exit 1

# Liste de mots à piocher (tu peux l'enrichir)
WORD_LIST=("chat" "chien" "voiture" "ordinateur" "maison" "bouteille" "musique" "livre" "soleil" "lune" "mer" "montagne" "arbre" "oiseau" "cheval" "route" "école" "pluie" "neige" "feu" "clé" "porte" "fleur" "ciel" "étoile" "vent" "lampe" "papier" "stylo" "téléphone")

# Génération des fichiers
for i in $(seq 0 70); do
  FILE="toto${i}.txt"
  # Mélange et sélection de 20 mots aléatoires uniques
  shuf -e "${WORD_LIST[@]}" | head -n 20 | tr '\n' ' ' > "$FILE"
done

echo "30 fichiers générés dans $(pwd)"

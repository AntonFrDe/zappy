# Politique de tests

L'objectif n'est pas d'atteindre 100% de couverture, mais qu'aucune
régression ne passe inaperçue sur `main`. En pratique : toute fonction qui
prend une décision (un `if`, un cas d'erreur) doit avoir au moins un test
qui vérifie son comportement.

## Qui teste quoi

| Fichier | Tests | Couvre |
|---|---|---|
| `server/test_parser.c` | 35 | parser CLI (`-p -x -y -n -c -f`, limites, erreurs) |
| `server/test_client.c` | 9 | buffers de réception/envoi, file de commandes |
| `server/test_error.c` | 9 | construction/libération des erreurs (`fail`, `free_error`) |
| `server/test_handshake.c` | 5 | poignée de main réseau de bout en bout (vrai socket) |
| `server/test_map.c` | 15 | grille torique (wrapping), déplacements, spawn des ressources |
| `server/test_command.c` | 3 | coût des commandes (`command_cost`) |
| `server/test_scheduler.c` | 3 | ordonnancement des commandes (délai, réponse) |
| `gui/test_command_parser.cpp` | 25 | parseur des commandes serveur → GUI (`msz`, `bct`, `pnw`...) |
| `ai/test_main.py` | 125 | logique de décision du bot (priorités, incantation, survie) |

`tests/server` (C, Criterion), `tests/gui` (C++, Criterion) et `tests/ai`
(Python, unittest) restent trois suites indépendantes — voir "Avant de
merger" pour les lancer.

## Règles

- **Un test par comportement.** Le nom décrit ce qui est vérifié, pas
  l'implémentation — `port_zero_fails`, pas `test3`.
- **Couvrir les cas limites, pas seulement le cas nominal.** Une valeur à
  0, négative, vide, en double ou manquante révèle souvent plus de bugs
  qu'un cas standard qui fonctionne déjà.
- **Pas de vraie I/O dans un test unitaire.** Les fonctions qui parlent au
  réseau sont mockées (`patch.object` côté Python), ou la logique est
  testée isolément. Exception assumée : `test_handshake.c`, qui ouvre un
  vrai socket loopback car c'est le seul moyen de vérifier la poignée de
  main de bout en bout.
- **Chaque test doit être indépendant** — pas de port fixe partagé, pas
  d'ordre d'exécution requis. Il doit pouvoir tourner seul.

## Avant de merger

Ces deux commandes doivent passer sans erreur :

```
make fclean && make tests_run
python3 -m unittest tests.ai.test_main -v
```

Un test cassé par un changement se corrige — il ne se supprime ni ne se
commente pour faire passer la CI.

## Ce qui n'est volontairement pas testé

- Les échecs de `malloc` : non simulables sans mock d'allocateur, et peu
  utiles à couvrir pour ce projet.
- `main()` dans chaque programme : c'est de l'orchestration, tout ce qu'il
  appelle est déjà testé séparément.

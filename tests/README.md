# Politique de tests

L'objectif n'est pas d'atteindre 100% de couverture, mais qu'aucune
régression ne passe inaperçue sur `main`. En pratique : toute fonction qui
prend une décision (un `if`, un cas d'erreur) doit avoir au moins un test
qui vérifie son comportement.

## Qui teste quoi

- `tests/server` (C, Criterion) : parser CLI, buffers client, gestion
  d'erreurs, protocole réseau.
- `tests/gui` (C++, Criterion) : parseur des commandes envoyées par le
  serveur.
- `tests/ai` (Python, unittest) : logique de décision du bot.

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

# Serveur

Le serveur porte l'état du jeu : la carte, les clients connectés, et le
rythme auquel les actions se déroulent. Il ne dessine rien et ne décide
rien pour un joueur — il applique les règles et répond aux commandes.

## Démarrage

`main.c` enchaîne trois étapes : lire les arguments (`parser.c`), créer le
serveur (`server_init`), puis lancer la boucle réseau (`server_run`). Si
une étape échoue, le serveur affiche l'erreur et quitte avec le code `84`.

Les arguments (`-p -x -y -n -c -f`) sont validés avant de démarrer quoi que
ce soit : port dans une plage valide, dimensions et nombre de clients
positifs, équipe `GRAPHIC` refusée (réservée à la GUI). Voir
`tests/server/test_parser.c` pour la liste exacte des cas acceptés/rejetés.

## Boucle réseau

Une seule boucle `poll()` gère toutes les connexions (`network.c`) : elle
accepte les nouveaux clients, lit ce qu'ils envoient ligne par ligne, et
écrit les réponses en attente. Un client doit d'abord annoncer son équipe
(ou `GRAPHIC` pour la GUI) avant que ses commandes de jeu soient prises en
compte — c'est la poignée de main initiale.

## Commandes et rythme

Chaque commande a un coût en unités de temps (`command.c`, ex. `Avance` :
7, `Incantation` : 300). Le serveur ne répond pas immédiatement : il calcule
quand la réponse doit partir en fonction de ce coût et de la fréquence du
jeu (`-f`), puis un ordonnanceur (`scheduler_tick`) vérifie à chaque tour si
une commande en cours est arrivée à échéance.

## Carte

La carte est une grille torique (`map.c`) : sortir d'un bord ramène de
l'autre côté, aussi bien pour les déplacements que pour la vision. Les
ressources sont réparties aléatoirement au démarrage selon des densités
fixes par type (ex. beaucoup de nourriture, très peu de thystame).

## Pour aller plus loin

Le détail des cas testés (limites, erreurs, comportements attendus) est
dans [`tests/README.md`](../tests/README.md) et directement dans les
fichiers `tests/server/*.c`.

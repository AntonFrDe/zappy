# Serveur

_Le monde, les règles, et personne pour jouer à la place des clients._

Le serveur porte l'état du jeu : la carte, les clients connectés, et le rythme auquel les actions se déroulent. Il ne dessine rien et ne décide rien pour un joueur — il applique les règles et répond aux commandes.

## Fichiers clés

| Fichier | Rôle |
|---|---|
| `parser.c` | Lecture et validation des arguments CLI |
| `network.c` | Boucle `poll()`, acceptation des clients, poignée de main |
| `client.c` | Buffers de réception/envoi, file de commandes par client |
| `command.c` | Coût des commandes, ordonnancement (`scheduler_tick`) |
| `map.c` | Grille torique, déplacements, spawn des ressources |
| `error.c` | Construction/libération des erreurs (`retour_t`) |

## Démarrage

```bash
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
```

`main.c` enchaîne trois étapes : parser les arguments, initialiser le serveur, lancer la boucle réseau. Toute étape en échec fait quitter avec le code `84`.

## Rythme des commandes

Chaque commande a un coût en unités de temps (ex. `Avance` : 7, `Incantation` : 300). Le serveur calcule quand répondre en fonction de ce coût et de la fréquence (`-f`), puis un ordonnanceur vérifie à chaque tour si une commande est arrivée à échéance.

## Carte

La carte est torique : sortir d'un bord ramène de l'autre côté. Les ressources sont réparties aléatoirement au démarrage selon des densités fixes par type.

## Documentation liée

- Détail des cas testés : [`tests/README.md`](../tests/README.md)
- Protocole serveur ↔ clients : voir les documents fournis par Epitech

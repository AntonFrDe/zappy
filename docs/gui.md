# GUI
_Un spectateur qui note tout ce qu'il voit, sans jamais y participer._

La GUI ne prend aucune décision de jeu : elle se connecte au serveur avec le rôle spécial `GRAPHIC`, reçoit un flux de commandes texte décrivant ce qui se passe sur la carte, et maintient une représentation à jour de l'état du jeu à partir de ce flux.

## Fichiers clés

| Fichier | Rôle |
|---|---|
| `parser.cpp` | Lecture et validation des arguments CLI |
| `NetworkClient.hpp` / `.cpp` | Connexion TCP au serveur, handshake, réception et découpage du flux en lignes |
| `CommandParser.cpp` | Traduction des commandes serveur en mises à jour d'état |
| `GameState.hpp` | Seule source de vérité : carte, joueurs, équipes, œufs, incantations |
| `main.cpp` | Point d'entrée : connexion, handshake, boucle de réception |

## Démarrage

```bash
./zappy_gui -p port -h machine
```

## Connexion au serveur

La connexion se déroule en trois étapes, gérées par `NetworkClient` :

1. **`connectToServer(host, port)`** — résolution DNS (`getaddrinfo`, IPv4/IPv6) puis connexion TCP.
2. **`performHandshake()`** — attend la ligne `WELCOME` envoyée par le serveur, puis répond `GRAPHIC` pour s'annoncer comme client spectateur.
3. **`readInitialMapSize(width, height)`** — lit la commande officielle
   `msz <width> <height>` envoyée par le serveur juste après le handshake.
   `GameState::resizeMap` est appelé directement avec ces valeurs, puis les
   autres commandes initiales (`sgt`, `tna`, `bct`, `pnw`...) sont traitées
   par `CommandParser`.

Ensuite, `main.cpp` entre dans une boucle qui appelle `NetworkClient::receiveLines` en continu ; chaque ligne reçue est découpée sur `\n` (buffer interne géré en interne, donc une commande coupée entre deux paquets TCP est recollée automatiquement) puis transmise à `CommandParser::dispatch`.

> **Limitation connue du serveur actuel** : après l'envoi de la taille de map, le serveur (`scheduler_tick` dans `command.c`) ignore complètement les clients `is_graphic` et ne leur envoie plus jamais rien — ni déplacements de joueurs, ni ressources, ni incantations. Le flux `msz`/`bct`/`pnw`/... ci-dessous correspond au protocole GUI standard du sujet Zappy, que `CommandParser` sait déjà traiter, mais tant que le serveur n'est pas étendu pour diffuser ces événements aux clients graphiques, la GUI n'affichera qu'une carte vide de la bonne taille.

## Commandes reconnues

| Commande | Effet |
|---|---|
| `msz` | Redimensionne la carte |
| `bct` | Met à jour le contenu d'une tuile |
| `pnw` / `ppo` / `pdi` | Apparition / déplacement / mort d'un joueur |
| `pgt` / `pdr` | Ramassage / dépôt de ressource |
| `pic` / `pie` | Début / fin d'incantation |
| `enw` / `ebo` / `edi` | Apparition / éclosion / disparition d'un œuf |
| `seg` | Fin de partie |

Une commande inconnue, vide ou incomplète est ignorée sans faire planter le parseur (log `[parser] commande inconnue ignoree` sur `stderr`).

## Tester sans IA

Comme le serveur actuel ne notifie pas les clients GUI des événements de jeu, tester avec des IA connectées n'apporte rien de plus que tester sans elles : la GUI reçoit exactement le même flux (handshake + taille de map) dans les deux cas.

```bash
./zappy_server -p 4242 -x 10 -y 10 -n TeamA TeamB -c 5 -f 100
./zappy_gui -p 4242 -h localhost
```

Sortie attendue :
```
Connexion a localhost:4242...
Connecte au serveur, handshake reussi.
Taille de la map: 10x10
```

Cas à vérifier en plus :
- **Serveur injoignable / mauvais port** : la GUI doit afficher une erreur de connexion claire, sans planter.
- **Fermeture du serveur en cours de route** : `network.receiveLines` doit renvoyer `false` avec `"Le serveur a ferme la connexion"`, et la GUI doit se terminer proprement (pas de boucle infinie ni de crash).
- **Plusieurs GUI simultanées** : chaque instance doit recevoir sa propre taille de map indépendamment des autres.
- **Sans le vrai serveur** : un petit script Python peut simuler exactement la séquence `WELCOME` → `GRAPHIC` → `0\n<W> <H>\n` pour tester la GUI de façon isolée et reproductible (voir `tests/` pour des exemples).

## Documentation liée

- Détail des cas testés : [`tests/README.md`](../tests/README.md) et `tests/gui/test_command_parser.cpp`
- Protocole GUI ↔ serveur : voir les documents fournis par Epitech (protocole standard, actuellement partiellement implémenté côté serveur — voir la limitation connue ci-dessus)

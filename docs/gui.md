# GUI

_Un spectateur qui note tout ce qu'il voit, sans jamais y participer._

La GUI ne prend aucune décision de jeu : elle se connecte au serveur avec le rôle spécial `GRAPHIC`, reçoit un flux de commandes texte décrivant ce qui se passe sur la carte, et maintient une représentation à jour de l'état du jeu à partir de ce flux.

## Fichiers clés

| Fichier | Rôle |
|---|---|
| `parser.cpp` | Lecture et validation des arguments CLI |
| `CommandParser.cpp` | Traduction des commandes serveur en mises à jour d'état |
| `GameState.hpp` | Seule source de vérité : carte, joueurs, équipes, œufs, incantations |
| `main.cpp` | Point d'entrée, connexion au serveur |

## Démarrage

```bash
./zappy_gui -p port -h machine
```

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

Une commande inconnue, vide ou incomplète est ignorée sans faire planter le parseur.

## Documentation liée

- Détail des cas testés : [`tests/README.md`](../tests/README.md) et `tests/gui/test_command_parser.cpp`
- Protocole GUI ↔ serveur : voir les documents fournis par Epitech

# Architecture

_Un serveur au centre, tout le monde d'autre lui parle — jamais entre eux._

Zappy tourne comme trois programmes séparés qui ne communiquent qu'à
travers le serveur, chacun par sa propre connexion TCP. Ni la GUI ni un
client IA ne se parlent directement.

## Qui se connecte à qui

| Connexion | Rôle |
|---|---|
| Client IA → serveur | Un joueur : envoie des commandes de jeu, reçoit leurs résultats |
| GUI → serveur | Un spectateur privilégié (rôle `GRAPHIC`) : reçoit tous les événements de la partie |
| Serveur | Le seul point central — il ne se connecte à personne, il accepte |

## Ce que chacun sait

- Le **serveur** est la seule source de vérité : c'est lui qui décide de
  ce qui est valide et qui fait autorité sur l'état du jeu.
- La **GUI** reconstruit son propre état à partir de ce que le serveur lui
  envoie (voir [`docs/gui.md`](gui.md)) — elle ne devine jamais rien.
- Chaque **client IA** ne voit que ce que son joueur voit : sa propre
  tuile, son inventaire, le résultat de ses propres commandes.

## Pourquoi ça compte pour les tests

Comme les trois programmes ne partagent aucun état en mémoire, ils peuvent
être testés indépendamment : le serveur avec un vrai socket loopback
(`tests/server/test_handshake.c`), la GUI avec de fausses lignes de
protocole (`tests/gui/test_command_parser.cpp`), et l'IA avec une connexion
simulée (`unittest.mock`, `tests/ai/test_main.py`). Voir
[`tests/README.md`](../tests/README.md) pour le détail.

## Documentation liée

- [`docs/server.md`](server.md), [`docs/gui.md`](gui.md), [`docs/ai.md`](ai.md)
- Protocole détaillé (format exact des commandes) : voir les documents
  fournis par Epitech

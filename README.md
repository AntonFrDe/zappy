# Zappy

_A tribute to Zaphod Beeblebrox_

Zappy est un jeu réseau où plusieurs équipes s'affrontent sur une carte de tuiles contenant des ressources. L'équipe gagnante est la première à faire atteindre le niveau d'élévation maximum (niveau 8) à au moins 6 de ses joueurs.

## Binaires

Le projet produit trois exécutables :

| Binaire | Rôle | Langage |
|---|---|---|
| `zappy_server` | Serveur de jeu (monde, règles, réseau) | C / C++ / Rust |
| `zappy_gui` | Client graphique de visualisation | C++ |
| `zappy_ai` | Client IA pilotant un joueur | Libre |

## Build

```bash
make            # compile les 3 binaires
make zappy_server
make zappy_gui
make zappy_ai
make clean fclean re
```

## Utilisation

```bash
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
./zappy_gui -p port -h machine
./zappy_ai -p port -n name -h machine
```

| Option | Description |
|---|---|
| `-p port` | Port d'écoute / de connexion |
| `-x width` / `-y height` | Dimensions de la carte |
| `-n name1 name2 ...` | Noms des équipes |
| `-c clientsNb` | Nombre de clients initiaux par équipe |
| `-f freq` | Fréquence (inverse de l'unité de temps), défaut `100` |
| `-h machine` | Adresse du serveur, défaut `localhost` |

## Équipe

| Membre | Rôle |
|---|---|
| Anton | Serveur / Architecture |
| Nikita | IA |
| Luca | GUI |
| Amine | Tests |

## Suivi du projet

L'avancement des tâches est suivi sur le [GitHub Project (Kanban)](https://github.com/orgs/EpitechPGE2-2025/projects/892/views/1).

## Documentation

- Sujet complet du projet et protocole GUI : voir les documents fournis par Epitech.
- Le protocole réseau client IA ↔ serveur ainsi que le protocole GUI ↔ serveur sont détaillés dans le sujet.

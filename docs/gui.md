# GUI
_Un spectateur qui note tout ce qu'il voit, sans jamais y participer._

La GUI ne prend aucune décision de jeu : elle se connecte au serveur avec le rôle spécial `GRAPHIC`, reçoit un flux de commandes texte décrivant ce qui se passe sur la carte, et maintient une représentation à jour de l'état du jeu à partir de ce flux, affichée avec SFML (thème Pac-Man).

## Fichiers clés

| Fichier | Rôle |
|---|---|
| `parser.cpp` | Lecture et validation des arguments CLI |
| `NetworkClient.hpp` / `.cpp` | Connexion TCP au serveur, handshake, réception et découpage du flux en lignes |
| `CommandParser.cpp` | Traduction des commandes serveur en mises à jour d'état |
| `GameState.hpp` | Seule source de vérité : carte, joueurs, équipes, œufs, incantations, effets ponctuels, messages |
| `RenderTheme.hpp` / `.cpp` | Calculs purs du thème visuel (couleurs, angles, animations), sans dépendance SFML — testables unitairement |
| `Renderer.hpp` / `.cpp` | Rendu SFML : carte, HUD, caméra, animations |
| `main.cpp` | Point d'entrée : connexion, handshake, boucle de réception/rendu |

## Démarrage

```bash
./zappy_gui -p port -h machine
```

## Connexion au serveur

La connexion se déroule en trois étapes, gérées par `NetworkClient` :

1. **`connectToServer(host, port)`** — résolution DNS (`getaddrinfo`, IPv4/IPv6) puis connexion TCP.
2. **`performHandshake()`** — attend la ligne `WELCOME` envoyée par le serveur, puis répond `GRAPHIC` pour s'annoncer comme client spectateur. Un timeout global (10s) évite d'attendre indéfiniment un serveur muet, et l'envoi de `GRAPHIC\n` gère un éventuel envoi partiel (`send()` n'est pas tenu d'écrire tout le buffer d'un coup).
3. **`readInitialMapSize(width, height)`** — lit la première ligne envoyée par le serveur juste après le handshake, qui doit être `msz <width> <height>` (voir `gui_send_snapshot` côté serveur, `src/server/gui_protocol.c`). `GameState::resizeMap` est appelé avec ces valeurs, puis le reste du snapshot initial (`sgt`, `tna`, `bct`, `pnw`/`pin`, `enw`...) est traité par `CommandParser`.

Ensuite, `main.cpp` entre dans une boucle qui appelle `NetworkClient::receiveLines` en continu ; chaque ligne reçue est découpée sur `\n` (buffer interne géré en interne, donc une commande coupée entre deux paquets TCP est recollée automatiquement) puis transmise à `CommandParser::dispatch`. `receiveLines` draine aussi tout ce qui est déjà disponible sur le socket dans la même frame, pour ne pas prendre de retard sur une rafale d'événements (ex: le snapshot initial).

Le serveur notifie désormais les clients graphiques en temps réel après le snapshot (déplacements, ramassage/dépôt de ressources, incantations, pontes/éclosions d'œufs, morts, victoire — voir les appels `gui_notify_*` dans `src/server/commands/*.c` et `src/server/command.c`). Deux lacunes connues restent côté serveur, pas compensées côté GUI :
- la réapparition périodique des ressources (`spawn_resources_if_ready` dans `command.c`) ne déclenche pas de `bct` de mise à jour, donc les quantités affichées peuvent devenir périmées avec le temps ;
- aucun `pin` n'est envoyé quand la faim d'un joueur diminue naturellement (seuls `pgt`/`pdr` mettent à jour l'inventaire affiché).

## Commandes reconnues

| Commande | Effet |
|---|---|
| `msz` | Redimensionne la carte |
| `bct` | Met à jour le contenu d'une tuile |
| `tna` | Enregistre une équipe (couleur assignée par ordre d'apparition) |
| `pnw` / `ppo` / `plv` / `pin` / `pdi` | Apparition / déplacement (animé) / niveau / inventaire / mort d'un joueur |
| `pgt` / `pdr` | Ramassage / dépôt de ressource (avec effet visuel) |
| `pex` / `pbc` / `pfk` | Expulsion / broadcast / ponte d'œuf (effets visuels + message pour `pbc`) |
| `pic` / `pie` | Début / fin d'incantation (animation complète : anneau rituel, halos, succès/échec) |
| `enw` / `ebo` / `edi` | Apparition / éclosion / disparition d'un œuf |
| `sgt` / `sst` | Fréquence du serveur (initiale / modifiée), affichée dans le HUD |
| `seg` | Fin de partie (bandeau à l'écran) |
| `smg` / `suc` / `sbp` | Message serveur / commande inconnue / mauvais nombre de paramètres, affichés dans la fenêtre |

Une commande inconnue, vide ou incomplète est ignorée sans faire planter le parseur (`requireCompleteLine` rejette toute ligne dont l'extraction d'un paramètre échoue, avant que la valeur ne soit utilisée ; log `[parser] ...` sur `stdout`/`stderr`).

## Fonctionnalités du rendu

- **Joueurs** : Pac-Man orienté selon la direction (protocole 1=N/2=E/3=S/4=W), déplacement interpolé (avec passage toroïdal aux bords), plusieurs joueurs sur une même case répartis en cercle.
- **Œufs** : fantômes roses, animation de ponte/éclosion.
- **Ressources** : une couleur par type, taille proportionnelle à la quantité.
- **Incantations** : anneau rituel animé, halo par participant, niveau visé affiché, explosion succès/échec.
- **HUD** (panneau fixe à droite, indépendant du zoom) : équipes/couleurs, joueurs vivants par équipe, fréquence serveur, légende des ressources, sélection à la clic (joueur ou case) pour voir l'inventaire/le contenu détaillé.
- **Caméra** : zoom (molette), déplacement (flèches), letterbox qui conserve le ratio d'aspect de la carte au lieu de l'étirer au redimensionnement de fenêtre.
- **Nettoyage d'état** : joueurs morts, œufs éclos et incantations terminées sont retirés après leur animation, pas gardés indéfiniment.

## Tester

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
suivi des logs `[parser] ...` au fil des événements reçus (équipes, joueurs, ressources...).

Cas à vérifier en plus :
- **Serveur injoignable / mauvais port** : la GUI doit afficher une erreur de connexion claire, sans planter.
- **Fermeture du serveur en cours de route** : `network.receiveLines` doit renvoyer `false` avec `"Le serveur a ferme la connexion"`, et la GUI doit se terminer proprement (pas de boucle infinie ni de crash).
- **Plusieurs GUI simultanées** : chaque instance doit recevoir sa propre taille de map indépendamment des autres.
- **Sans le vrai serveur** : un petit script (Python ou autre) peut simuler la séquence `WELCOME` → `GRAPHIC` → `msz <W> <H>\n` (+ éventuellement `tna`/`pnw`/... ) pour tester la GUI de façon isolée et reproductible.

## Documentation liée

- Détail des cas testés : [`tests/README.md`](../tests/README.md), `tests/gui/test_command_parser.cpp` et `tests/gui/test_render_theme.cpp`
- Protocole GUI ↔ serveur : voir les documents fournis par Epitech (protocole standard), et `src/server/gui_protocol.c` côté serveur pour le detail des messages effectivement envoyés

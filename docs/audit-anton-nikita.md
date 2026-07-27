# Audit — ce qu'il reste à faire (serveur + IA)

_Audit en lecture seule, dans le même esprit que l'audit GUI de Nikita : factuel, priorisé, avec fichier:ligne. Rien n'a été modifié dans le code par cet audit lui-même (le fix TEAM_GOAL mentionné plus bas a été fait puis annulé à la demande)._

Date : 2026-07-25. Basé sur `origin/main` au moment de l'audit.

---

## Serveur (Anton)

### Priorité 1 — bloquant

1. **La suite de tests serveur est actuellement rouge.**
   `make tests_run` sur `origin/main` donne **112/124 (12 échecs)**, exit code ≠ 0.
   **10 des 12** partagent une seule cause : `tests/server/test_gui_protocol.c` (9 tests, ex. ligne 95) et `tests/server/test_handshake.c:147` (`graphic_gets_client_num_zero_and_map_size`) attendent encore l'ancien handshake GUI (`"0"` puis `"W H"` bruts), alors que `handle_team_name`/`gui_send_snapshot` envoient désormais `"msz W H"` directement dès la première ligne — comportement déjà confirmé compatible avec la GUI actuelle.
   **Fix probable : mettre à jour les ~10 assertions de test**, pas le code serveur.

2. **Œufs initiaux jamais créés.**
   `egg_list_add` n'est appelé que depuis `src/server/commands/fork.c:9`. `server_init` (`src/server/network.c:35-64`) ne peuple `server->eggs` d'aucun slot initial (`clients_nb × teams_nb`). Confirmé toujours vrai sur `origin/main`.
   **Impact** : la GUI n'affiche jamais d'œuf avant le premier `Fork` — les joueurs qui rejoignent au lancement de la partie n'éclosent pas visuellement d'un œuf.

3. **Requêtes entrantes d'un client `GRAPHIC` jamais traitées.**
   `process_client_input` (`network.c:178-191`) pousse toute ligne — y compris d'un client `is_graphic` — dans `client_push_cmd`, mais `scheduler_tick` (`command.c:163-165`) fait :
   ```c
   if (!client->handshake_done || client->is_graphic) continue;
   ```
   La queue n'est **jamais dépilée** pour un client graphique. Une ébauche existe sur la branche `origin/23-réception-requêtes-gui` (non mergée, très en retard sur `main` : diff -3532/+662 lignes), mais `gui_dispatch_request` (`src/server/gui_requests.c`) est un **stub TODO** qui répond toujours `"suc\n"`, et n'est appelé nulle part.
   **Recommandation : à refaire depuis le `main` actuel plutôt qu'à merger telle quelle.**

4. **`test_resource_respawn::respawns_again_once_twenty_time_units_have_elapsed` échoue** (`tests/server/test_resource_respawn.c:65`).
   Le test attend un doublement de quantité après 20 unités de temps, mais `map_spawn_resources` (`src/server/map.c:95-113`) ne fait que recompléter jusqu'à la densité cible (`missing = target - current`) — sans consommation entre-temps, `missing = 0`.
   Probablement une hypothèse fausse du test plutôt qu'un bug d'implémentation, mais échoue réellement aujourd'hui : **à trancher entre vous.**

5. **`test_takeset::take_succeeds_when_resource_present_on_tile` échoue** sur `server_init(...) == 0` (`tests/server/test_takeset.c:42`) — probable collision de port ou thread zombie (voir point 11).

### Priorité 2 — fonctionnalités manquantes

6. **Pas de `bct` après la réapparition périodique des ressources** — `spawn_resources_if_ready` (`command.c:154-160`) n'appelle aucun `gui_notify_*`. Les quantités affichées côté GUI deviennent périmées avec le temps.

7. **Pas de `pin` sur la décroissance naturelle de la faim** — `check_food` (`command.c:135-150`) décrémente `inventory[RESOURCE_FOOD]` sans notifier ; seuls `pgt`/`pdr` mettent à jour l'affichage de l'inventaire.

8. Cf. point 3 : traitement des requêtes GUI à terminer/re-router (probablement en réponse synchrone directe via `client_queue_send`, pas via la queue de commandes IA existante).

9. **Aucun test pour la condition de victoire** — `src/server/victory.c` (34 lignes) n'a pas de `tests/server/test_victory.c`.

### Priorité 3 — stabilité / finition

10. `client_push_cmd` (`client.c:106-114`) peut échouer silencieusement (queue pleine, `CLIENT_MAX_QUEUED_CMDS=10`) — retour ignoré par `process_client_input`, commandes excédentaires perdues sans signal au client.

11. **Infra de tests d'intégration fragile** : plusieurs fichiers (`test_handshake.c`, `test_takeset.c`...) lancent un thread serveur détaché par `Test()` qui n'est **jamais arrêté** ni son port libéré (déjà admis en commentaire, `test_handshake.c:1-11`) — cause probable du point 5.

12. **Mémoire projet à purger** : "`4-parsing-arguments-serveur` ne compile pas (`free_rt` manquant)" est **obsolète** — `parser.c`/`main.c` compilent proprement aujourd'hui (0 warning), utilisent correctement `retour_t`/`free_error`/`flags_destroy`. Rien à corriger côté code.

### Déjà vérifié solide (rien à faire)

- Les 12 commandes standard (Forward/Right/Left/Look/Inventory/Broadcast/Connect_nbr/Fork/Eject/Take/Set/Incantation) sont toutes implémentées.
- Validation CLI (`parser.c`) complète (port/largeur/hauteur/clients/freq bornés, nom `GRAPHIC` réservé).
- Gestion mémoire client/egg propre (`client_destroy`, `egg_list_destroy`).
- Pas de dépassement de buffer trouvé dans la lecture socket.

---

## IA (Nikita)

### Priorité 1 — bug fonctionnel

1. **Taille d'équipe cible codée en dur à 6** (`src/ai/spawner.py:12`, `TEAM_GOAL = 6`, utilisé ligne 63/75 et dans `src/ai/main.py` pour borner `-i`), indépendante de la configuration serveur réelle. Le serveur renvoie pourtant le nombre de places restantes lors du handshake (`free_places` dans `src/ai/client.py:78-80`), mais cette valeur n'était validée que comme un digit puis jetée — jamais utilisée pour déduire le vrai `clients_nb`.
   **Impact concret** : si le serveur est lancé avec `-c` ≠ 6, l'IA tente de faire grandir l'équipe jusqu'à 6 quoi qu'il arrive (boucle Fork/spawn qui échoueront à rejoindre si `-c` < 6), ou arrête de grandir à 6 alors que plus de slots existent si `-c` > 6 — ce qui réduit directement les chances d'atteindre les niveaux 6-7 (qui demandent 6 joueurs au même endroit).

   **Statut** : corrigé (branche `fix-ai-team-goal`, commit `FIX taille d'equipe deduite du serveur au lieu d'une constante figee a 6`), puis **supprimé (local + remote) à la demande de l'utilisateur** pendant la session. Le fix consistait à :
   - faire retourner `int(free_places)` par `join_team()` (`client.py`) ;
   - ajouter `set_team_goal(team_goal)` dans `spawner.py` ;
   - dans `main.py`, calculer `player_number + free_places` juste après `join_team()` et appeler `set_team_goal(...)` avec ce résultat, avant `run_ai_loop` ;
   - découpler la validation CLI de `-i` de `TEAM_GOAL` (borne générique à la place, puisque la vraie taille n'est connue qu'après connexion).
   - 3 tests ajoutés (`TestJoinTeam`, `TestSetTeamGoal`).

   À refaire côté IA si vous voulez le garder — le diff exact est décrit ci-dessus, reproductible en quelques minutes.

### Découvert en marge du fix (nouveau, pas vu par l'audit initial)

2. **12 tests IA échouent déjà sur `main`** (`python3 -m unittest tests.ai.test_main`, exécuté directement — l'audit initial ne l'avait pas fait tourner). Tous dans `strategy.py`, logique "joueur présent sur la case" incohérente ou priorité d'exploration :
   - `TestChooseExplorationCommand::test_player_takes_priority`
   - `TestChooseExplorationCommand::test_empty_tile_picks_from_left_right`
   - `TestShouldAvoidBusyResourceTile` (3 sous-tests : deraumere/linemate/sibur-mendiane)
   - `TestShouldCheckInventory::test_player_without_food_returns_true`
   - `TestShouldTakeDeraumereFirst/MendianeFirst/SiburFirst/ThystameFirst::test_player_present_returns_false`
   - `TestShouldTurnOnBusyTile::test_player_and_more_than_two_items_returns_true`
   - `TestTurnOnEmptyTile::test_empty_tile_turns_right`

   Confirmé pré-existant (même échecs avant et après le fix TEAM_GOAL, sur le code déjà commité de `main`) — **pas introduit par le fix**, mais à vérifier : soit les tests ont une hypothèse fausse, soit `strategy.py` a un vrai bug de priorité (prendre une ressource sur une case où un adversaire est présent, ou mal choisir la direction d'exploration).

### Vérifié sain (pas de bug)

- Les besoins d'incantation (`strategy.py:10-16` `INCANTATION_STONES`, `incantation.py:11` `INCANTATION_PLAYERS`) correspondent **exactement** à la table serveur (`src/server/commands/incantation.c:9-23`), niveau par niveau.
- Coordination multi-bots pour les incantations groupées (`incantation.py`) : tie-break par id le plus petit (`get_call_to_follow`, ligne 123) désigne un seul leader — pas de risque de double-déclenchement.
- Une seule commande en vol à la fois (`send_command_to_server` attend toujours la réponse, `client.py:63-70`) — pas de désync commande/réponse possible.
- Ignorer la ligne `eject:` (`client.py:49-50`) est sans conséquence : l'IA recalcule sa position via `Look` à chaque itération, ne maintient pas d'état de position interne.
- Aucun TODO/FIXME/stub trouvé dans `src/ai/`.

### Priorité 3 — trous de test

- Toute la logique de coordination d'incantation (`src/ai/incantation.py`) — la partie la plus complexe et la plus exposée à des races potentielles — n'a **aucun test dédié** : `remember_incantation_call`, `save_incantation_call`, `save_ready_player`, `get_call_to_follow`, `count_ready_players`, `broadcast_incantation_help`, `say_ready_for_incantation`, `request_incantation_help`, `follow_incantation_call`, `try_current_incantation`, `has_incantation_help_requirements`.
- Gestion de process dans `spawner.py` non testée : `start_new_ai`, `child_is_running`, `can_start_child` (seul `try_create_team_slot` l'est, via mocks).

---

## Ordre recommandé

**Anton** : 1 (fixer les tests, rapide) → 2 (œufs initiaux, rejoint un vrai gap GUI déjà connu) → 3/8 (requêtes GUI, le plus gros morceau) → 6/7 → 9 → 10-12.

**Nikita** : 1 (refaire le fix TEAM_GOAL, diff déjà écrit ci-dessus) → 2 (comprendre/corriger les 12 tests qui échouent, potentiellement un vrai bug de jeu) → tests sur `incantation.py`/`spawner.py`.

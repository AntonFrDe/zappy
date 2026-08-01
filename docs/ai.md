# IA

_Un joueur autonome qui négocie ses priorités à chaque tour, sans jamais rien demander._

Le client IA pilote un seul joueur : il envoie des commandes texte au serveur, lit les réponses, et décide de la prochaine action à chaque itération.

## Fichiers clés

| Fichier | Rôle |
|---|---|
| `main.py` | Arguments et point d'entrée |
| `client.py` | Connexion réseau et protocole client/serveur |
| `bot.py` | Boucle principale (`run_ai_loop`) et actions du joueur |
| `coordination_state.py` | État partagé par les mécanismes d'élévation |
| `coordination.py` | Réception des broadcasts et suivi des sessions d'élévation |
| `incantation.py` | Dépôt des pierres et exécution du rituel |
| `spawner.py` | Création et relance du second joueur |
| `strategy.py` | Fonctions de décision pures (priorités, incantation, ramassage) |

## Déroulement d'une incantation

La coordination d'une élévation suit toujours le même chemin :

1. `client.py` transmet les broadcasts reçus à `coordination.py`.
2. `coordination.py` valide l'équipe, le niveau et la session de l'appel.
   Si les deux joueurs émettent un appel simultanément, le plus petit
   identifiant devient le chef.
3. Le partenaire suit la direction du broadcast, puis confirme son
   arrivée avec l'identifiant de session reçu.
4. `incantation.py` dépose les pierres et
   lance la commande `Incantation`.
5. `coordination.py` enregistre le nouveau niveau et ferme la session.

Toutes les données modifiables de ce mécanisme sont regroupées dans
l'objet `STATE` de `coordination_state.py`. Les autres modules lisent ou
mettent à jour cet objet au lieu de déclarer leurs propres variables
globales. Une session trop ancienne est abandonnée afin que les joueurs
puissent reprendre leur exploration.

## Démarrage

```bash
./zappy_ai -p port -n name -h machine
```

## Boucle de décision

À chaque tour, `run_ai_loop` évalue une liste de priorités dans l'ordre et agit dès qu'une condition correspond :

| Priorité | Vérifie |
|---|---|
| 1 | Survie — nourriture au sol ou déplacement pour en chercher |
| 2 | Suivi d'un appel d'incantation en cours |
| 3 | Création ou relance du second joueur |
| 4 | Pierres manquantes et incantation jusqu'au niveau 4 |
| 5 | Constitution d'une réserve de nourriture |
| 6 | Exploration par défaut si rien d'autre ne s'applique |

L'IA lance au maximum deux joueurs, le nombre exact nécessaire pour
atteindre le niveau 4. Le protocole de coordination est volontairement
limité à un chef et un partenaire. Chaque joueur annonce périodiquement
sa présence par `Broadcast`. Parmi les joueurs actifs, le plus petit
identifiant devient gestionnaire et utilise `Connect_nbr` puis, si
nécessaire, `Fork` pour créer le partenaire manquant. Si le gestionnaire
disparaît, sa présence expire et le survivant prend automatiquement sa
place. Plusieurs processus initiaux d'une même équipe élisent ainsi un
seul gestionnaire, et ceux qui dépassent la limite de deux joueurs se
retirent proprement. Aucune donnée de coordination ne passe hors du serveur.

Une fois le niveau 4 atteint, les joueurs ne cherchent plus de pierres et
se concentrent sur la nourriture et l'exploration.

## Communication réseau

Chaque commande part avec un `\n` final. Les fragments TCP sont accumulés
dans un buffer jusqu'au prochain `\n` et les lignes déjà reçues restent
disponibles pour la commande suivante. Une réponse `dead` fait quitter le
programme proprement (code `0`) ; une connexion fermée par le serveur fait
quitter avec le code `84`.

## Documentation liée

- Détail des cas testés : [`tests/README.md`](../tests/README.md) et `tests/ai/test_main.py`
- Protocole client IA ↔ serveur : voir les documents fournis par Epitech

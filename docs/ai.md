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
| `team.py` | Coéquipiers connus et groupe d'incantation courant |
| `incantation.py` | Dépôt des pierres et exécution du rituel |
| `spawner.py` | Création et relance du second joueur |
| `strategy.py` | Fonctions de décision pures (priorités, incantation, ramassage) |

## Déroulement d'une incantation

La coordination d'une élévation suit toujours le même chemin :

1. `client.py` transmet les broadcasts reçus à `coordination.py`.
2. `coordination.py` mémorise l'appel du chef et les joueurs prêts.
3. `team.py` détermine le chef et sélectionne exactement le nombre de
   joueurs requis par le niveau. Chaque joueur collecte les pierres
   requises sans partager son inventaire.
4. `incantation.py` rapproche les participants, dépose les pierres et
   lance la commande `Incantation`.
5. `coordination.py` enregistre le nouveau niveau et ferme la session.

Toutes les données modifiables de ce mécanisme sont regroupées dans
l'objet `STATE` de `coordination_state.py`. Les autres modules lisent ou
mettent à jour cet objet au lieu de déclarer leurs propres variables
globales.

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
| 4 | Signalement périodique du niveau aux coéquipiers |
| 5 | Pierres manquantes et incantation jusqu'au niveau 4 |
| 6 | Constitution d'une réserve de nourriture |
| 7 | Exploration par défaut si rien d'autre ne s'applique |

L'IA lance au maximum deux joueurs. Cela suffit pour atteindre le niveau
4 ; les élévations suivantes, qui demandent davantage de participants,
ne sont volontairement pas poursuivies. Le gestionnaire surveille un seul
processus partenaire et le relance après un délai s'il meurt.

Une fois le niveau 4 atteint, les joueurs ne cherchent plus de pierres et
se concentrent sur la nourriture et l'exploration.

## Communication réseau

Chaque commande part avec un `\n` final ; la réponse est lue octet par
octet jusqu'au prochain `\n`. Une réponse `dead` fait quitter le programme
proprement (code `0`) ; une connexion fermée par le serveur fait quitter
avec le code `84`.

## Documentation liée

- Détail des cas testés : [`tests/README.md`](../tests/README.md) et `tests/ai/test_main.py`
- Protocole client IA ↔ serveur : voir les documents fournis par Epitech

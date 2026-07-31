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
| `team.py` | Coéquipiers connus, cohorte et répartition des pierres |
| `incantation.py` | Dépôt des pierres et exécution du rituel |
| `spawner.py` | Création et lancement des nouveaux clients |
| `strategy.py` | Fonctions de décision pures (priorités, incantation, ramassage) |

## Déroulement d'une incantation

La coordination d'une élévation suit toujours le même chemin :

1. `client.py` transmet les broadcasts reçus à `coordination.py`.
2. `coordination.py` mémorise l'appel du chef et les joueurs prêts.
3. `team.py` détermine le chef, la cohorte et les pierres attribuées à
   chaque joueur.
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
| 2 | Conditions réunies pour tenter une incantation (niveau 1 ou 2) |
| 3 | Création d'un slot d'équipe (`Fork`) si aucun n'est disponible |
| 4 | Ramassage des ressources selon leur rareté et l'état de l'inventaire |
| 5 | Suivi d'un œuf, vérification de l'inventaire, évitement des tuiles trop chargées |
| 6 | Exploration par défaut si rien d'autre ne s'applique |

## Communication réseau

Chaque commande part avec un `\n` final ; la réponse est lue octet par
octet jusqu'au prochain `\n`. Une réponse `dead` fait quitter le programme
proprement (code `0`) ; une connexion fermée par le serveur fait quitter
avec le code `84`.

## Documentation liée

- Détail des cas testés : [`tests/README.md`](../tests/README.md) et `tests/ai/test_main.py`
- Protocole client IA ↔ serveur : voir les documents fournis par Epitech

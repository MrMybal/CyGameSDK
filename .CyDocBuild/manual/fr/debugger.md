---
id: debugger
title: Débogueur et comparaison
description: Débogueur et comparaison
section: Graphes
order: 12
sourceRevision: 1
parent: graphs
---

## Suivre un appel {#stepping}

Démarrez le débogage, posez des points d’arrêt et inspectez trace et variables. F10 passe l’appel, F11 y entre et Maj+F11 sort d’une fonction d’exécution. Continuer reprend jusqu’à une frontière ou un arrêt ; Stop termine la session. Les appels purs restent atomiques. Observez valeurs, arguments et variables locales dans le contexte de pile choisi.

Une modification sémantique invalide le programme en pause ; le débogueur devient périmé. Les points d’arrêt peuvent être actualisés en direct. Ne reprenez pas un ancien instantané comme s’il représentait le document modifié. Les macros conservent leur correspondance aux sources ; les identifiants d’objets concernent uniquement le runtime local courant.

## Recherche et diagnostics {#navigation}

Le navigateur recherche graphes racines, fonctions, commentaires, valeurs et variables. Signets par vue et reroutages typés facilitent les déplacements. Les diagnostics de graphes/shaders conservent leur gravité et pointent vers les nœuds. Les anciennes lignes de journal Core sont affichées au niveau information ; leur texte n’est pas un code d’erreur typé.

## Comparer à l’enregistrement {#compare}

**Comparer à l’enregistrement** compare le graphe actif à l’asset enregistré ou à la référence de session. Filtrez les changements, ignorez éventuellement positions/commentaires, puis examinez les détails côte à côte ou unifiés. La comparaison est en lecture seule : elle ne fusionne rien, ne modifie pas l’historique et ne recharge pas silencieusement un fichier changé sur disque. Les changements externes sont détectés. Le détail est borné sans perdre les comptes du résumé ; comparaison de révisions Git arbitraires et fusion visuelle ne sont pas implémentées.

```image
{"id": "diff", "src": "assets/screenshots/diff.png", "alt": "Comparaison en lecture seule des entrées actives et enregistrées", "controls": "minimal"}
```

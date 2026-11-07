---
id: json
title: Documents JSON et métadonnées
description: Documents JSON et métadonnées
section: Ressources
order: 25
sourceRevision: 1
parent: content-browser
---

## Ouvrir et modifier {#edit}

Ouvrez un asset JSON dans le navigateur. Dépliez objets/tableaux, filtrez les clés et modifiez les valeurs prises en charge ; ajoutez/retirez des entrées par les actions de l’arbre. Les modifications sont validées avant application et utilisent l’annulation partagée. Sauvegarder JSON est explicite et protège des changements externes. Copier JSON relève de la présentation, pas d’une sauvegarde.

Le profil courant est limité à 512 Kio et quatre documents chargés. Les longues chaînes sont en lecture seule dans l’interface. Préservez les champs inconnus des contrats neutres au lieu de les retirer avec un formulaire générique. Fermer un document enregistré le libère ; préservez les changements avant fermeture.

Cette vue n’est pas un parseur de configuration arbitraire de jeu ni un moteur de migration de schéma. Consultez les [métadonnées portables](doc:portable-assets) et le schéma exact avant une modification automatisée.

```image
{"id": "json", "src": "assets/screenshots/json.png", "alt": "Arbre JSON et édition des valeurs", "controls": "minimal"}
```

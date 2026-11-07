---
id: recipes
title: Enregistrer et rejouer des configurations
description: Enregistrer et rejouer des configurations
section: SDK
order: 52
sourceRevision: 1
parent: automation
---

## Enregistrer des valeurs, pas une exécution {#save}

Les vues Opérations/Options sauvegardent et chargent des recettes versionnées. Une recette décrit opération, paramètres typés, produit/version et métadonnées. Charger restaure les valeurs sans exécuter. Vérifiez-les avant soumission. La sauvegarde automatique facultative des paramètres n’autorise pas à lancer un job ou écraser une source.

Validez les relectures contre le manifeste/schéma courant. IDs inconnus, chemins invalides ou anciens types de paramètres sont des erreurs, pas une migration silencieuse. Les sauvegardes explicites sont persistantes ; les sorties workers suivent le stockage des tâches Core. Un timeout SDK peut survenir après validation d’une modification ; inspectez l’état avant de la rejouer.

---
id: materials
title: Matériaux et aperçus compilés
description: Matériaux et aperçus compilés
section: Matériaux
order: 30
sourceRevision: 1
---

## Construire une surface {#nodes}

Ouvrez un graphe de matériau. Reliez opérations scalaires/vecteurs/couleurs, paramètres, UV U/V, transformations UV, damier/dégradé, composition de couleur, sinus/fract/saturate et échantillonneurs pris en charge vers la sortie. Les fonctions pures conservent le contexte UV. Une branche UV inutilisée n’est pas évaluée seulement pour échantillonner une autre sortie. Un échantillon central en échec n’empêche pas à lui seul de compiler un programme GPU valide.

Les instances gardent les surcharges typées séparées de la topologie du parent. Résolvez les parents chargés en priorité, préservez leur état et vérifiez chemins, types et cycles. La chaîne prise en charge est bornée à huit liens ; l’actualisation disque est manuelle.

## Compiler et inspecter {#compile}

Choisissez **Compiler l’aperçu** avec Python/Core configuré. Plans natifs et génération Python partagent ordre des paramètres et empreintes. Le ProcessWorker officiel génère/compile le HLSL en DXBC ; un rapport contrôlé installe ensuite le shader dans sa vue DX11. Sphère/plan et paramètres uniformes en direct utilisent ce shader. Les défauts racines sont des uniformes ; les défauts internes aux fonctions sont des données du programme.

Progression, diagnostics et annulation relèvent du job Core. Signature périmée, empreinte incohérente, sortie absente ou expression non prise en charge refusent l’installation. Les objets GPU conservent leurs données après nettoyage des fichiers. N’envoyez pas de HLSL fourni par l’asset, d’inclusions arbitraires ou de commandes exécutables au compilateur. Déplacer la vue n’invalide pas le shader.

## Échantillonnage et limites {#sampling}

Les PNG/JPEG/BMP utilisent un décodage borné, la conversion sRGB, le filtrage bilinéaire et la répétition. L’alpha reste linéaire. Jusqu’à huit liaisons / 64 Mio sont prises en charge, avec chemins littéraux confinés. Le résultat reste un aperçu d’éditeur, ni codec/exporteur de matériau de jeu ni compilateur de moteur.

```image
{"id": "materials", "src": "assets/screenshots/materials.png", "alt": "Échantillonneur de texture alimentant le matériau DX11 compilé", "controls": "minimal"}
```

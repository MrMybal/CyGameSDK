---
id: images
title: Inspection des images
description: Inspection des images
section: Matériaux
order: 31
sourceRevision: 1
parent: materials
---

## Inspecter sans peindre {#view}

Double-cliquez un PNG, JPEG ou BMP pris en charge. La vue WIC présente métadonnées, pixels et canaux. Choisissez RGBA/RGB ou un canal, **Ajuster l’image**, **100 %**, zoom à la molette et déplacement au bouton central. RGB ignore l’alpha ; un canal seul est affiché en gris opaque. Rechargez explicitement pour lire des octets modifiés.

Le mode ajusté suit la taille du canevas. Zoom et déplacement quittent ce mode ; le zoom reste entre 5 % et 800 %. Ces réglages de présentation ne créent pas d’annulation du document. Ni peinture pixel ni décodeur de texture native d’un jeu ne sont fournis.

Le décodeur confine les chemins et vérifie 32 Mio/fichier, 8192 pixels/côté et 4 194 304 pixels décodés. Le cache est limité à huit entrées. L’import d’une texture est une [opération explicite sur source](doc:asset-import), distincte de son inspection.

```image
{"id": "texture", "src": "assets/screenshots/texture.png", "alt": "Canaux d’image et aperçu sur damier", "controls": "minimal"}
```

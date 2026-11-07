---
id: animation-tools
title: IK, mouvement racine et outils de clips
description: IK, mouvement racine et outils de clips
section: Ressources
order: 23
sourceRevision: 1
parent: assets-3d
---

## Essayer les outils de mouvement {#start}

Ouvrez `Examples3D/ChainMotion.cy3d`, puis les outils de mouvement. Choisissez clip source ou pose de référence. Le résultat crée et sélectionne un nouveau clip, avec une annulation du document et sauvegarde explicite. Source et données de liaison restent intactes. En IK, choisissez racine/extrémité, cible (1,1,0), pôle facultatif (0,0,1), puis créez le clip. Examinez erreur maximale et échantillons inaccessibles/non convergés.

## IK à racine fixe {#ik}

La chaîne FABRIK bornée comporte 3 à 32 nœuds directement reliés ; la racine doit précéder l’extrémité. Chaque échantillon garde position racine et longueurs source, y compris translations animées. Les branches latérales suivent les rotations. Le solveur vise une position, pas l’orientation finale, avec 1 à 128 itérations et tolérance de 0,000001 à 0,1 m. Une cible hors de portée n’étire pas les os ; portées minimale/maximale et convergence sont signalées. Une amorce déterministe traite les chaînes droites ; le pôle facultatif oriente les articulations internes.

Les coordonnées sont celles du monde de l’asset : repère droit +Y haut, mètres. Seul le TRS décomposé est accepté. Chaîne et ancêtres doivent avoir une échelle unitaire, animation comprise ; les segments nuls échouent. Le résultat est un nouveau clip de rotations LINEAR échantillonné à 1–120 FPS ; les autres pistes restent intactes. La pose de référence produit une image. Ni limites articulaires, contacts, évitement des collisions, verrouillage des pieds, contraintes multichaînes ou déduction d’humanoïde ne sont fournis.

## Paires de mouvement racine {#root-motion}

Sélectionnez une racine TRS au premier niveau. Choisissez axes de translation et extraction de rotation facultative : il s’agit du quaternion entier, pas du seul lacet. La première transformation échantillonnée donne le verrou. L’extraction crée un clip sur place et un clip de déplacement, en gardant source et pistes non racines. Le déplacement est une donnée de transport, pas un contrôleur de personnage.

Pour position source p/rotation q et valeurs sur place l/r, `Dq=q*inverse(r)` et `Dp=p-rotate(Dq,l)`. La reconstruction utilise `p=Dp+rotate(Dq,l)` et `q=Dq*r`. Décalage initial, axes mélangés et rotation sont ainsi préservés. `extensions["org.cygame.editor"].rootMotion` garde jusqu’à 32 paires avec UUID, indices de clips, racine, politique, FPS et empreintes de poses float32. Modifier une pose ou le parent racine refuse la reconstruction : extrayez une nouvelle paire. Réordonner/supprimer des clips doit migrer les indices atomiquement. La reconstruction respecte les échantillons, pas les courbes cubiques originales entre ceux-ci.

## Découper et rééchantillonner {#trim}

Choisissez une plage inclusive et une fréquence. Les deux extrémités sont incluses et le nouveau clip commence à zéro. Respectez les budgets de clés/document. Signalez les pertes au lieu de promettre une conservation exacte de l’interpolation non linéaire. Les résultats restent éditables/exportables par les jobs existants ; mouvement moteur, physique, réplication et adaptation du mouvement ne sont pas fournis.

```image
{"id": "animation", "src": "assets/screenshots/animation.png", "alt": "Choix du clip, images clés et timeline", "controls": "minimal"}
```

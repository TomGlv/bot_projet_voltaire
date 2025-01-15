# Projet Voltaire Helper
![Alt Text]("ressources\ProjetVoltaireHelper.ico")
## Description
Le **Projet Voltaire Helper** est un outil qui surveille le presse-papiers de Windows pour détecter tout nouveau texte. Lorsqu'un texte est copié, il est automatiquement envoyé à l'API de correction grammaticale de Reverso. Le programme renvoie ensuite la version corrigée du texte avec les corrections mises en surbrillance dans le terminal.

Ce projet peut être combiné avec des outils comme **PowerToys** pour extraire du texte à partir de captures d'écran, transformant ainsi cet outil en un véritable assistant pour des projets tels que le **Projet Voltaire**, qui aide à améliorer l'orthographe et la grammaire.

---

## Prérequis

1. **Windows 10/11** avec le terminal correctement configuré pour afficher l'UTF-8.
2. **PowerToys** (téléchargeable ici : [PowerToys GitHub](https://github.com/microsoft/PowerToys)) ou sur le microsoft store :
   - Utilisez la fonctionnalité **Text Extractor** pour capturer du texte directement à partir de l'écran.
3. **Visual Studio** ou tout autre IDE compatible avec C++.
4. **vcpkg** pour gérer les dépendances de bibliothèques comme `nlohmann/json` et `libcurl`.

---

## Installation

1. Clonez ce dépôt ou copiez les fichiers dans un nouveau projet Visual Studio.
2. Assurez-vous que les bibliothèques suivantes sont installées :
   - [nlohmann/json](https://github.com/nlohmann/json)
   - [libcurl](https://curl.se/libcurl/)
3. Configurez votre projet pour inclure et lier correctement ces bibliothèques.

---

## Utilisation

1. Démarrez le programme. Il surveillera automatiquement le presse-papiers.
2. Ouvrez PowerToys et activez l'outil **Text Extractor** :
   - Faites une capture de l'écran contenant du texte à corriger.
   - Le texte extrait sera copié automatiquement dans le presse-papiers.
3. Le programme Clipboard Spell Checker détectera le texte copié, l'enverra à Reverso et affichera le texte corrigé dans le terminal, avec les corrections en **rouge**.

---

## Exemple d'utilisation avec le Projet Voltaire

En combinant **PowerToys** pour extraire des textes de n'importe quel écran et cet outil, vous pouvez :
- Repérer rapidement des fautes dans des documents ou des images.
- Recevoir des corrections en temps réel.
- Améliorer vos compétences en orthographe et grammaire pour des examens ou certifications comme le Projet Voltaire.

---

## Avertissements

1. Ce projet utilise l'API publique de Reverso. Assurez-vous de ne pas dépasser les limites d'utilisation si vous l'utilisez intensivement.
2. L'affichage en couleurs (rouge pour les corrections) peut ne pas fonctionner sur certains terminaux qui ne supportent pas les séquences ANSI.

---

## Contribution

Les contributions sont les bienvenues !
- Ouvrez une issue pour signaler un problème ou proposer une fonctionnalité.
- Faites un fork du projet et créez une pull request pour soumettre vos améliorations.

---

## Licence
Ce projet est sous licence MIT. Consultez le fichier `LICENSE` pour plus de détails.


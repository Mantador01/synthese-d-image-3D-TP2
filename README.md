Le code est dans tp.cpp contient le projet du TP2 et les shaders sont dans data/rungholt, /!\ il faut décompresser le fichier rungholt.zip car le fichier obj étais trop gros.

Depuis le repertoire data/ :
unzip rungholt.zip

Pour compiler (depuis la racine de gkit):
premake4 gmake
make tp
bin/tp


Ce dépôt contient le projet du TP2.
Le code source principal se trouve dans `tp.cpp` et les shaders sont situés dans `data/rungholt/`.

## ⚠️ Important

La ville 3D (`.obj`) étant trop volumineux pour GitHub, les données ont été compressées.
**Il faut impérativement décompresser l'archive avant de lancer le programme.**

Dans le répertoire `data` :
   ```bash
   cd gkit2light/data/
   ```

Décompressez l'archive :
   ```bash
unzip rungholt.zip
   ```
Ensuite pour compiler :
   ```bash
premake4 gmake
make tp
bin/tp
```
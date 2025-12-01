Ce dépôt contient le projet du TP2.
Le code C++ se trouve dans `tp.cpp` et les shaders sont situés dans `data/rungholt/`.

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

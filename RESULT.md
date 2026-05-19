# Result
![alt text](result.png)
# Instanciando objetos na cena 3D
![alt text](result1.png)
## Comandos de Teclado (Como Usar)

- **W, A, S, D**: Move (translada) os cubos na tela nos eixos X e Z.
- **I, J**: Move (translada) os cubos no eixo Y.
- **[ e ]** (Colchetes): Altera a escala uniforme dos cubos (diminuir e aumentar o tamanho). *(Nota: dependendo do teclado ABNT2, podem corresponder às teclas de acento e colchete).*
- **X, Y, Z**: Ativa/desativa a rotação contínua dos cubos nos respectivos eixos.
- **ESC**: Fecha a aplicação.

# Carga de objetos OBJ/MTL com texturas

Esta etapa adiciona ao visualizador a leitura de arquivos `.OBJ` (vértices, coordenadas de textura, normais e referência ao `.MTL`), a leitura de `.MTL` (nome da textura via `map_Kd`) e o desenho dos objetos texturizados.

## O que foi implementado

- **`loadSimpleOBJ`** em [src/Hello3D.cpp](src/Hello3D.cpp): lê o `.OBJ` capturando posições (`v`), coordenadas de textura (`vt`), normais (`vn`), faces (`f`) e a referência `mtllib`. Monta VBO/VAO com **8 floats por vértice** — posição (3), cor (3) e coord. textura (2) — e devolve `nVertices` por referência.
- **`loadMTL`**: abre o `.MTL` indicado pelo OBJ e extrai o nome da textura difusa (`map_Kd`).
- **`loadTexture`**: usa `stb_image` para carregar a textura referenciada pelo `.MTL` e devolver o ID OpenGL.
- **Shaders atualizados**: o vertex shader passa `texCoord` para o fragment shader; o fragment shader amostra a textura via `sampler2D` (com fallback para a cor sólida quando não há textura).
- **`Modelos3D/cube.obj` + `cube.mtl` + `cube_texture.bmp`**: cubo de exemplo com UVs em cada face e textura xadrez gerada.

## Como rodar

```
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
./Hello3D.exe
```
> O executável procura os modelos em `../Modelos3D/`, então rode-o de dentro de `build/`.

## Estrutura adicionada

```
include/stb_image.h         # carregamento de imagens
Modelos3D/cube.obj          # cubo com vt/vn e referência mtllib cube.mtl
Modelos3D/cube.mtl          # material com map_Kd cube_texture.bmp
Modelos3D/cube_texture.bmp  # textura xadrez 32x32
```

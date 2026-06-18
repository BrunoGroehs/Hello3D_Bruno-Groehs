# Diorama3D — Visualizador 3D

Visualizador 3D simples em **C++ / OpenGL** com pipeline gráfico completo: carregamento de OBJ/MTL, texturas, iluminação Phong de 3 pontos, câmera em primeira pessoa, animação por curvas de Bézier e configuração de cena por arquivo de texto.

Autor: **Bruno Groehs**

---

## Setup (Compilação e Execução)

### Dependências
- **CMake** ≥ 3.10
- Compilador C++17 (MSVC, MinGW ou GCC/Clang)
- **OpenGL** (já instalado em qualquer SO recente)
- As libs **GLFW** e **GLM** são baixadas automaticamente via `FetchContent` do CMake

### Build (Windows / MinGW ou MSVC)
```bash
cmake -B build
cmake --build build
```

### Executar
```bash
cd build
./Diorama3D
```
ou no Windows: `build\Diorama3D.exe`

A pasta `Modelos3D/` é copiada automaticamente para `build/` durante a configuração do CMake.

---

## Controles (Atalhos de Teclado)

### Câmera
| Tecla | Ação |
|---|---|
| `W` `A` `S` `D` | Mover câmera |
| Mouse | Olhar ao redor |
| Scroll | Zoom (FOV) |
| `ESC` | Sair |

### Seleção e Transformação de Objeto
| Tecla | Ação |
|---|---|
| `TAB` | Trocar o objeto selecionado (ciclo) |
| `← → ↑ ↓` | Translação no plano XY |
| `Page Up` / `Page Down` | Translação no eixo Z |
| `X` `Y` `Z` | Rotação contínua nos eixos |
| `[` `]` | Diminuir / aumentar escala uniforme |

### Iluminação (Phong — 3 pontos)
| Tecla | Ação |
|---|---|
| `1` | Liga/desliga **Key Light** |
| `2` | Liga/desliga **Fill Light** |
| `3` | Liga/desliga **Back Light** |
| `+` / `-` | Aumentar / diminuir intensidade |

### Materiais e Texturas
| Tecla | Ação |
|---|---|
| `M` | Alternar textura (map_Kd) ↔ cor sólida |

### Animação (Bézier)
| Tecla | Ação |
|---|---|
| `T` | Liga/desliga animação |
| `,` / `.` | Diminuir / aumentar velocidade |
| `L` | Recarregar pontos do arquivo `trajetoria.txt` |

---

## Arquitetura do Código

Todo o código-fonte está em **um único arquivo**, para fins didáticos e fácil leitura:

[src/Diorama3D.cpp](src/Diorama3D.cpp)

### Onde está cada coisa

| Componente | Local | Linhas (aprox.) |
|---|---|---|
| **Struct `SceneObject`** (geometria + material + transformação) | `Diorama3D.cpp` | ~20 |
| **Vertex Shader** (matrizes Model/View/Projection, normal world-space) | `Diorama3D.cpp` | ~58 |
| **Fragment Shader** (Phong + 3 luzes + atenuação + textura) | `Diorama3D.cpp` | ~75 |
| **Variáveis globais** (câmera, flags, vetores de cena, trajetória) | `Diorama3D.cpp` | ~135 |
| **`loadTrajectory` / `saveTrajectory`** (parser do arquivo de pontos) | `Diorama3D.cpp` | ~155 |
| **`bezierPoint`** (curva de Bézier cúbica) | `Diorama3D.cpp` | ~190 |
| **`loadScene`** (parser do arquivo de configuração da cena) | `Diorama3D.cpp` | ~205 |
| **`main`** (loop, criação da janela, uniforms de luz, render loop) | `Diorama3D.cpp` | ~260 |
| **Loop de render** (passa uniforms `model/view/projection`, desenha cada objeto) | `Diorama3D.cpp` | ~320 |
| **`key_callback`** (todos os atalhos) | `Diorama3D.cpp` | ~395 |
| **`processInput`** (movimento contínuo da câmera) | `Diorama3D.cpp` | ~470 |
| **`mouse_callback` / `scroll_callback`** | `Diorama3D.cpp` | ~485 |
| **`setupShader`** (compila e linka shaders) | `Diorama3D.cpp` | ~515 |
| **`loadSimpleOBJ`** (parser de OBJ com `v`, `vt`, `vn`, `f` → VAO) | `Diorama3D.cpp` | ~560 |
| **`loadMTL`** (lê `Ka`, `Kd`, `Ks`, `Ns`, `map_Kd`) | `Diorama3D.cpp` | ~660 |
| **`loadTexture`** (carrega imagem com stb_image) | `Diorama3D.cpp` | ~690 |

### Operações matemáticas fundamentais (para a arguição)

- **Matriz de Model**: composta por translação → rotação (auto-rotação se selecionado + rotação fixa do scene.txt) → escala — montada no loop de render para cada objeto.
- **Matriz de View**: `glm::lookAt(Position, Position + Front, Up)` em [include/Camera.h](include/Camera.h).
- **Matriz de Projection**: `glm::perspective(...)` montada a cada frame.
- **Cálculo de iluminação Phong**: feito no **Fragment Shader**. Para cada uma das 3 luzes calcula:
  - `ambient = 0.2 * ka * lightColor * intensity`
  - `diffuse = kd * max(dot(N, L), 0) * lightColor * attenuation * intensity`
  - `specular = ks * max(dot(V, R), 0)^Ns * lightColor * attenuation * intensity`
- **Atenuação**: `1 / (1 + 0.09·d + 0.032·d²)` (modelo de luz pontual).
- **Curva de Bézier cúbica**: `B(t) = (1-t)³P₀ + 3(1-t)²t·P₁ + 3(1-t)t²·P₂ + t³P₃` em janelas deslizantes de 4 pontos da lista — função `bezierPoint`.

---

## Configuração de Cena ([Modelos3D/scene.txt](Modelos3D/scene.txt))

Um objeto por linha. Linhas começando com `#` são ignoradas.

Formato:
```
caminho_obj  tx ty tz  rx ry rz  escala  usa_trajetoria
```

- **`caminho_obj`**: caminho relativo do `.obj`
- **`tx ty tz`**: translação inicial (offset)
- **`rx ry rz`**: rotação fixa em graus (X, Y, Z)
- **`escala`**: escala uniforme inicial
- **`usa_trajetoria`**: `1` para somar a posição da curva de Bézier, `0` para ficar parado

Exemplo (cena padrão):
```
Modelos3D/Suzanne.obj   0.0 0.0 0.0   0 0 0   1.0   1
Modelos3D/cube.obj      2.0 0.0 0.0   0 0 0   0.5   0
Modelos3D/cube.obj     -2.0 0.0 0.0   0 0 0   0.5   0
```

---

## Configuração de Trajetória ([Modelos3D/trajetoria.txt](Modelos3D/trajetoria.txt))

Lista de pontos `x y z` (mínimo 4 pontos para Bézier funcionar). A curva é montada em janelas de 4 pontos consecutivos e percorrida ciclicamente.

---

## Assets (Procedência)

| Asset | Origem |
|---|---|
| `Suzanne.obj` / `Suzanne.mtl` | Modelo padrão do **Blender 4.3.0** (export do "Monkey") |
| `Suzanne.png` | Textura de exemplo gerada localmente |
| `cube.obj` / `cube.mtl` | Geometria de cubo simples |
| `cube_texture.bmp` | Textura de exemplo simples |
| `stb_image.h` | Biblioteca pública: https://github.com/nothings/stb |
| `glad.c` | Gerado em https://glad.dav1d.de/ (OpenGL 3.3 Core) |
| `Camera.h` | Adaptado de https://learnopengl.com/Getting-started/Camera |

Software de processamento prévio: **Blender 4.3** (exportação do Suzanne em OBJ + MTL).

---

## Referências

- [LearnOpenGL — Joey de Vries](https://learnopengl.com/) — câmera, iluminação Phong, carregamento de modelos.
- [Documentação oficial OpenGL 3.3 Core](https://www.khronos.org/opengl/wiki/)
- [Documentação GLM](https://github.com/g-truc/glm) — biblioteca de matemática.
- [Documentação GLFW 3.4](https://www.glfw.org/docs/latest/) — janela e input.
- [Especificação Wavefront OBJ/MTL](https://en.wikipedia.org/wiki/Wavefront_.obj_file)
- Material da disciplina de **Computação Gráfica** — Unisinos.

---

## Status do Projeto

✅ Carregamento de OBJ + MTL  
✅ Mapeamento de textura (`map_Kd`)  
✅ Iluminação Phong (3 luzes pontuais com atenuação)  
✅ Câmera FPS (teclado + mouse)  
✅ Múltiplos objetos via arquivo de configuração  
✅ Seleção e transformação por objeto (translação, rotação, escala)  
✅ Animação por curva de Bézier cúbica  
✅ Toggle de textura, toggle individual de luzes, controle de intensidade  

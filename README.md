# Atividade CG - Criando o Ambiente de Programacao de Cenas 3D

Projeto Hello3D com OpenGL 3.3+ em C++.

## O que instalar antes de rodar

### 1. MSYS2
- Baixa em https://www.msys2.org e instala
- Abre o terminal MSYS2 e roda:
```
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake
```
- Adiciona `C:\msys64\ucrt64\bin` no PATH do Windows

### 2. Git
- Baixa em https://git-scm.com/downloads

### 3. Compilar e rodar
```
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
./Hello3D.exe
```

## Controles
- **X** - rotaciona no eixo X
- **Y** - rotaciona no eixo Y
- **Z** - rotaciona no eixo Z
- **ESC** - fecha

## Depois de rodar
- Tira um print da janela rodando
- Coloca no RESULT.md

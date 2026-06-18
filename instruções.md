# Objetivo da Avaliação

Durante o semestre, vocês desenvolveram progressivamente um visualizador 3D, integrando novos conceitos a cada etapa (desenho de primitivas, carregamento de malhas, mapeamento de texturas, iluminação, etc.). O objetivo desta avaliação final é apresentar a aplicação completa e unificada.

Não se trata de demonstrar as tarefas do semestre separadamente, mas sim de apresentar um visualizador funcional que integre todos os componentes do pipeline gráfico em uma única cena (diorama), atestando a autoria e o domínio do código-fonte (C++/Python e Shaders).

---

### Funcionalidades Obrigatórias

O visualizador deve possuir atalhos de teclado ou uma interface gráfica para alternar e demonstrar em tempo real:

1. **Seleção e Transformação**
   - Selecionar um objeto da cena
   - Aplicar operações de translação, rotação e escala uniforme

2. **Materiais e Texturas**
   - Alternar a exibição para comprovar a leitura correta dos coeficientes do material (ka, ks, kd do arquivo .mtl)
   - Demonstrar mapeamento de textura

3. **Iluminação (Phong)**
   - Modificar parâmetros de luz
   - Ligar/desligar individualmente as fontes de luz da cena
   - Incluir a lógica de 3 pontos
   - Evidenciar o modelo de iluminação implementado

4. **Câmera**
   - Navegar ativamente pela cena
   - Controle via teclado/mouse

5. **Animação**
   - Iniciar e pausar a trajetória de objetos
   - Definir com curvas paramétricas (Bézier)

### Durante a Apresentação

Você deverá:
- Responder a perguntas objetivas
- Apontar no código onde ocorrem as operações matemáticas fundamentais, como:
  - Parser do arquivo de configuração da cena
  - Passagem de uniforms
  - Cálculo de iluminação no Fragment Shader
  - Manipulação das matrizes de Model e View

---

### B. Repositório e Código-Fonte

**Link do repositório** (GitHub, GitLab, etc.) contendo:
- Todo o projeto
- Arquivo de configuração da cena (JSON, XML, txt, etc.)
- Assets podem estar incluídos para carregamento padrão, respeitando as licenças de uso e limites de tamanho da plataforma

**Arquivo README.md com**:
- **Setup**: Instruções exatas de compilação, execução e dependências utilizadas
- **Assets**: Procedência dos modelos 3D e texturas (com links de origem) e indicação de uso de softwares de processamento prévio (Blender, MeshLab, etc.)
- **Referências**: Bibliografia, documentação técnica (OpenGL, bibliotecas matemáticas) e tutoriais consultados durante o semestre
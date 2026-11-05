# Number Striker☄️

Projeto integrador: jogo educativo 2D em **C com Allegro 5**.

## Ideia
Uma cidade está sendo atacada por meteoros. Para protegê-la, o jogador resolve contas matemáticas antes que os meteoros atinjam os prédios. Respostas corretas destroem meteoros e aumentam a pontuação. Meteoros que atingem a cidade retiram vidas.

> **Estado atual:** protótipo jogável com menu, seleção de dificuldade, contas em meteoros, pontuação e vidas. Compilação e execução ainda precisam ser testadas no Visual Studio.

## Estrutura

```text
Projeto-Jogo2D-Senac/
├── src/
│   ├── main.c           # Ponto de entrada em C
│   ├── interface.c      # Interface e lógica do jogo
│   └── interface.h      # Declaração da função executar_jogo
├── assets/
│   └── README.md        # Organização dos recursos
├── docs/
│   └── PLANEJAMENTO.md  # Regras e roteiro de implementação
├── .gitignore
└── README.md
```

## Tecnologias
- Linguagem C
- Allegro 5 (bibliotecas core, font e primitives)
- Visual Studio no Windows
- Git e GitHub

## Como executar no Visual Studio
1. Instale e configure Allegro 5 em um projeto C compatível no Visual Studio.
2. Adicione `src/main.c` ao projeto.
3. Configure os diretórios de inclusão, bibliotecas e os arquivos de biblioteca Allegro correspondentes à versão e arquitetura usada.
4. Vincule os módulos `allegro`, `allegro_font` e `allegro_primitives`.
5. Adicione **ambos** os arquivos `src/main.c` e `src/interface.c` ao projeto e mantenha `src/interface.h` na mesma pasta.
6. Em Propriedades > C/C++ > Avançado, escolha Compilar como Código C (/TC).
7. Garanta que as DLLs necessárias estejam acessíveis durante a execução, se estiver usando vinculação dinâmica.
8. Compile e execute. Selecione a dificuldade com o mouse, clique em JOGAR, digite respostas numéricas e pressione Enter. Use **Esc** para sair.

Este repositório ainda não inclui arquivos de projeto do Visual Studio nem uma distribuição do Allegro.

## Próximos passos
Consulte [o planejamento](docs/PLANEJAMENTO.md) para acompanhar as tarefas.

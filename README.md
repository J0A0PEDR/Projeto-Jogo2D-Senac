# Number Striker☄️

Projeto integrador: jogo educativo 2D em **C com Allegro 5**.

## Ideia
Uma cidade está sendo atacada por meteoros. Para protegê-la, o jogador resolve contas matemáticas antes que os meteoros atinjam os prédios. Respostas corretas destroem meteoros e aumentam a pontuação. Meteoros que atingem a cidade retiram vidas.

> **Estado atual:** jogo com modos Fácil, Médio e Difícil, contas de soma, subtração e multiplicação (no Difícil), pontuação, laser, vidas e tela de fim de jogo. A compilação precisa ser validada no Visual Studio.

## Estrutura

```text
Projeto-Jogo2D-Senac/
├── src/
│   ├── main.c           # Ponto de entrada em C
│   ├── jogo.c           # Logica do jogo, interface e meteoros
│   └── jogo.h           # Declaracao da funcao executar_jogo
├── assets/
│   └── README.md        # Organização dos recursos
├── NumberStriker.sln
├── NumberStriker.vcxproj
├── packages.config
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

## Abrir no Visual Studio 2022 (modo rápido)

1. Clone ou baixe o repositório.
2. Abra **NumberStriker.sln** no Visual Studio 2022 com a carga de trabalho **Desenvolvimento para desktop com C++** (compilador MSVC usado para compilar C).
3. Clique em **Restaurar Pacotes NuGet** na solução, caso a restauração não aconteça automaticamente. É necessária conexão com a internet na primeira vez.
4. Escolha **x64 / Debug** e pressione **Ctrl+F5**.

O arquivo `NumberStriker.vcxproj` já compila `main.c` e `jogo.c` como **C11 (/TC)** e inclui o Allegro 5.2.11.3 via NuGet. Não é necessário criar outro projeto, incluir arquivos manualmente ou configurar caminhos de headers/libraries quando a restauração funciona.

**Observação:** os arquivos de projeto foram preparados, mas ainda não foram compilados em uma instalação Windows/Visual Studio. Caso o NuGet não restaure ou a integração das bibliotecas apresente erro, envie a saída de compilação para correção.

## Configuração manual (alternativa)
1. Instale e configure Allegro 5 em um projeto C compatível no Visual Studio.
2. Adicione `src/main.c` ao projeto.
3. Configure os diretórios de inclusão, bibliotecas e os arquivos de biblioteca Allegro correspondentes à versão e arquitetura usada.
4. Vincule os módulos `allegro`, `allegro_font` e `allegro_primitives`.
5. Adicione **ambos** os arquivos `src/main.c` e `src/jogo.c` ao projeto e mantenha `src/jogo.h` na mesma pasta.
6. Em Propriedades > C/C++ > Avançado, escolha Compilar como Código C (/TC).
7. Garanta que as DLLs necessárias estejam acessíveis durante a execução, se estiver usando vinculação dinâmica.
8. Compile e execute. Selecione a dificuldade com o mouse, clique em JOGAR, digite respostas numéricas e pressione Enter. Use **Esc** para sair.

O repositório inclui a solução e o projeto do Visual Studio. As bibliotecas do Allegro são restauradas pelo NuGet e não ficam armazenadas no Git.

## Próximos passos
Consulte [o planejamento](docs/PLANEJAMENTO.md) para acompanhar as tarefas.

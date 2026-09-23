# Projeto 1 — Processamento de imagens

Universidade Presbiteriana Mackenzie — Faculdade de Computação e Informática
Ciência da Computação — Computação Visual — Prof. André Kishimoto

Programa de linha de comando em C que carrega uma imagem, converte para escala de
cinza, exibe o histograma e permite equalizá-lo, alternar a resolução de exibição
e salvar o resultado.

> **A preencher pelo grupo:** nomes completos, RA de cada integrante e a divisão de
> contribuições (ver a seção [Contribuições](#contribuições)).

---

## Como usar

```
proj1 caminho_da_imagem.ext
```

Exemplo:

```
./build/proj1 foto.png
```

O programa abre duas janelas:

| Janela | Conteúdo |
|---|---|
| **Principal** | a imagem sendo processada, inicialmente em 1024x768, centralizada no monitor principal |
| **Secundária** (filha, fixa em 420x520, no canto (0,0)) | o histograma, as informações de análise e os dois botões de ação |

### Controles

| Ação | Efeito |
|---|---|
| Botão **Equalizar** / **Ver original** | equaliza o histograma e atualiza as duas janelas; clicando de novo, volta à imagem original em escala de cinza |
| Botão **Resolução original** / **1024x768** | alterna a resolução de exibição e redimensiona a janela principal |
| Tecla **S** | salva a imagem exibida como `output_image.png` na pasta atual |

Os botões são desenhados com primitivas da SDL e mudam de cor conforme o estado:
azul (neutro), azul claro (mouse sobre o botão) e azul escuro (botão pressionado).

---

## O que o programa faz

**1. Carregamento.** A imagem é carregada com `IMG_Load` (SDL_image). Se o caminho
não existir ou o arquivo não for um formato de imagem válido, o programa exibe a
mensagem de erro no terminal e encerra.

**2. Análise e conversão para escala de cinza.** O programa percorre os pixels e
verifica se R = G = B em todos eles; o resultado é informado no terminal. Se a
imagem for colorida, ela é convertida com

```
Y = 0.2125 * R + 0.7154 * G + 0.0721 * B
```

A imagem em escala de cinza é a base de todas as operações seguintes e é mantida
intacta em memória, o que permite reverter a equalização sem recarregar o arquivo.

**3. Histograma e análise.** O histograma (256 níveis) é calculado a partir da
imagem exibida. Dele saem duas medidas:

- **média de intensidade** — classifica a imagem como `escura` (< 85), `media`
  (85 a 170) ou `clara` (> 170);
- **desvio padrão** — classifica o contraste como `baixo` (< 35), `medio`
  (35 a 70) ou `alto` (> 70).

**4. Equalização.** Usa a função de distribuição acumulada do próprio histograma:

```
s_k = (L-1) * Σ(j=0..k) p(r_j)
```

A tabela de conversão é montada uma vez e aplicada pixel a pixel.

**5. Salvamento.** A tecla `S` grava `output_image.png` com `IMG_SavePNG`, na
resolução em que a imagem está sendo exibida. O programa verifica se o arquivo já
existia e informa no terminal se ele foi **criado** ou **sobrescrito**.

---

## Estrutura do projeto

```
.
├── src/
│   ├── main.c      — argumentos, inicialização, laço de eventos
│   ├── image.h/.c  — carregar, detectar cinza, converter, histograma, equalizar, salvar
│   └── ui.h/.c     — janelas, botões, desenho do histograma e dos textos
├── assets/
│   ├── DejaVuSans.ttf          — fonte usada nos textos
│   └── DejaVuSans-LICENSE.txt  — licença da fonte
├── makefile
└── README.md
```

A separação segue a responsabilidade de cada parte: `image` não conhece janelas e
`ui` não processa pixels. `main` liga os dois.

### Fonte

A fonte usada é a **DejaVu Sans**, de licença livre, incluída no repositório em
`assets/`. O caminho é montado em tempo de execução a partir de `SDL_GetBasePath()`,
ou seja, a pasta do executável — o programa não depende de fontes instaladas no
sistema nem do diretório de onde foi chamado, e funciona igual em Windows, Linux e
macOS. O `makefile` copia `assets/` para junto do executável.

---

## Compilação

### Requisitos

| Item | Versão usada no desenvolvimento |
|---|---|
| Sistema operacional | *(a preencher: ex. Windows 11 24H2 / Ubuntu 24.04 no WSL)* |
| Compilador | gcc *(a preencher: ex. 15.1.0 no Windows, 15.2.0 no WSL)* |
| SDL3 | 3.2.31 |
| SDL3_image | 3.2.7 |
| SDL3_ttf | 3.2.3 |
| Padrão da linguagem | C99 |

### Linux / WSL

Com as bibliotecas instaladas e visíveis ao `pkg-config`:

```bash
make
./build/proj1 caminho_da_imagem.png
```

Se as bibliotecas estiverem num prefixo próprio:

```bash
export PKG_CONFIG_PATH=/caminho/do/prefixo/lib/pkgconfig
make
```

### Windows (MinGW-w64)

Baixe os pacotes de desenvolvimento `-devel-mingw` de SDL3, SDL3_image e SDL3_ttf,
extraia para uma pasta única e aponte o `makefile` para ela:

```
mingw32-make SDL_PREFIX=C:/dev/libs/SDL3
```

As DLLs (`SDL3.dll`, `SDL3_image.dll`, `SDL3_ttf.dll`) precisam estar na pasta do
executável ou no `PATH`.

### Limpar

```
make clean
```

---

## Contribuições

> **A preencher pelo grupo.** Descreva o que cada integrante fez.

| Integrante | RA | Contribuição |
|---|---|---|
| | | |
| | | |

---

## Licença dos recursos

A fonte DejaVu Sans é distribuída sob a licença Bitstream Vera / DejaVu, que
permite redistribuição. O texto completo está em `assets/DejaVuSans-LICENSE.txt`.

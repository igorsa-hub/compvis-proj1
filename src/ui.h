/*
 * ui.h - Janelas, botoes e desenho da interface.
 */
#ifndef UI_H
#define UI_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#include "image.h"

/* Tamanho padrao da janela principal, exigido pelo enunciado. */
#define DEFAULT_WIDTH  1024
#define DEFAULT_HEIGHT 768

/* Tamanho fixo da janela secundaria, definido pelo grupo. */
#define SIDE_WIDTH  420
#define SIDE_HEIGHT 520

/* Estado visual do botao, refletindo a acao do usuario. */
typedef enum {
    BUTTON_NEUTRAL,
    BUTTON_HOVER,
    BUTTON_PRESSED
} ButtonState;

typedef struct {
    SDL_FRect rect;
    const char *label;
    ButtonState state;
} Button;

typedef struct {
    SDL_Window   *main_window;
    SDL_Renderer *main_renderer;
    SDL_Window   *side_window;
    SDL_Renderer *side_renderer;
    SDL_Texture  *image_texture;    /* textura da imagem exibida na janela principal */
    TTF_Font     *font;
    Button        btn_equalize;
    Button        btn_resolution;
} UI;

/* Cria as duas janelas, os renderizadores e carrega a fonte. */
bool ui_create(UI *ui);
void ui_destroy(UI *ui);

/* Recria a textura da janela principal a partir da imagem atual. */
bool ui_update_texture(UI *ui, const Image *img);

/* Redimensiona e reposiciona a janela principal para w x h. */
void ui_resize_main_window(UI *ui, int w, int h);

/* Desenho de cada janela. */
void ui_render_main(UI *ui);
void ui_render_side(UI *ui, const Image *img, int display_w, int display_h);

/* Atualiza o estado dos botoes conforme o mouse. Devolve o botao clicado
   (1 = equalizar, 2 = resolucao, 0 = nenhum) quando o clique e concluido. */
int ui_handle_mouse(UI *ui, const SDL_Event *event);

#endif

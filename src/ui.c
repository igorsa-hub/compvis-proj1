/*
 * ui.c - Janelas, botoes e desenho da interface.
 */
#include "ui.h"

#include <stdio.h>

#define FONT_FILE "assets/DejaVuSans.ttf"
#define FONT_SIZE 15.0f

/* Paleta dos botoes: um tom de azul por estado, como pede o enunciado. */
static const SDL_Color BTN_COLOR[3] = {
    { 40,  90, 200, 255},   /* BUTTON_NEUTRAL */
    { 80, 130, 235, 255},   /* BUTTON_HOVER   */
    { 22,  52, 130, 255}    /* BUTTON_PRESSED */
};

static const SDL_Color TEXT_COLOR = {235, 238, 242, 255};
static const SDL_Color INFO_COLOR = { 30,  34,  40, 255};

/* Area do histograma dentro da janela secundaria. */
#define HIST_X 20
#define HIST_Y 40
#define HIST_W 380
#define HIST_H 190

/* ------------------------------------------------------------------ */
/* Desenha um texto na posicao indicada.                               */
/* ------------------------------------------------------------------ */
static void draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                      float x, float y, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, 0, color);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_FRect dst = { x, y, (float)surface->w, (float)surface->h };
        SDL_RenderTexture(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

/* Desenha o texto centralizado dentro de um retangulo. */
static void draw_text_centered(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                               const SDL_FRect *box, SDL_Color color)
{
    int w = 0, h = 0;
    TTF_GetStringSize(font, text, 0, &w, &h);
    draw_text(renderer, font, text,
              box->x + (box->w - w) / 2.0f,
              box->y + (box->h - h) / 2.0f, color);
}

/* ------------------------------------------------------------------ */
/* Botoes desenhados com primitivas da SDL (retangulos).               */
/* ------------------------------------------------------------------ */
static void draw_button(SDL_Renderer *renderer, TTF_Font *font, const Button *btn)
{
    SDL_Color c = BTN_COLOR[btn->state];

    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(renderer, &btn->rect);

    SDL_SetRenderDrawColor(renderer, 20, 40, 100, 255);
    SDL_RenderRect(renderer, &btn->rect);

    draw_text_centered(renderer, font, btn->label, &btn->rect, TEXT_COLOR);
}

static bool point_in_button(const Button *btn, float x, float y)
{
    return x >= btn->rect.x && x <= btn->rect.x + btn->rect.w &&
           y >= btn->rect.y && y <= btn->rect.y + btn->rect.h;
}

/* ------------------------------------------------------------------ */
/* Criacao e destruicao                                                */
/* ------------------------------------------------------------------ */
bool ui_create(UI *ui)
{
    SDL_zerop(ui);

    /* Janela principal: 1024x768 e centralizada no monitor principal. */
    ui->main_window = SDL_CreateWindow("Processamento de imagens", DEFAULT_WIDTH, DEFAULT_HEIGHT, 0);
    if (!ui->main_window) {
        SDL_Log("Erro ao criar a janela principal: %s", SDL_GetError());
        return false;
    }
    ui->main_renderer = SDL_CreateRenderer(ui->main_window, NULL);
    if (!ui->main_renderer) {
        SDL_Log("Erro ao criar o renderizador principal: %s", SDL_GetError());
        return false;
    }

    /* Janela secundaria: tamanho fixo, posicionada em (0,0) e filha da principal. */
    ui->side_window = SDL_CreateWindow("Histograma", SIDE_WIDTH, SIDE_HEIGHT, 0);
    if (!ui->side_window) {
        SDL_Log("Erro ao criar a janela secundaria: %s", SDL_GetError());
        return false;
    }
    SDL_SetWindowParent(ui->side_window, ui->main_window);
    SDL_SetWindowPosition(ui->side_window, 0, 0);

    ui->side_renderer = SDL_CreateRenderer(ui->side_window, NULL);
    if (!ui->side_renderer) {
        SDL_Log("Erro ao criar o renderizador secundario: %s", SDL_GetError());
        return false;
    }

    /* A fonte vem junto com o programa: o caminho e montado a partir da
       pasta do executavel, entao funciona em qualquer sistema operacional
       e independe do diretorio de onde o programa foi chamado. */
    const char *base = SDL_GetBasePath();
    char font_path[1024];
    SDL_snprintf(font_path, sizeof(font_path), "%s%s", base ? base : "", FONT_FILE);

    ui->font = TTF_OpenFont(font_path, FONT_SIZE);
    if (!ui->font) {
        SDL_Log("Erro ao carregar a fonte \"%s\": %s", font_path, SDL_GetError());
        return false;
    }

    /* Botoes, abaixo do histograma e um abaixo do outro. */
    ui->btn_equalize   = (Button){ { 20.0f, 390.0f, 380.0f, 46.0f }, "Equalizar", BUTTON_NEUTRAL };
    ui->btn_resolution = (Button){ { 20.0f, 448.0f, 380.0f, 46.0f }, "Resolucao original", BUTTON_NEUTRAL };

    return true;
}

void ui_destroy(UI *ui)
{
    if (ui->image_texture)  SDL_DestroyTexture(ui->image_texture);
    if (ui->font)           TTF_CloseFont(ui->font);
    if (ui->side_renderer)  SDL_DestroyRenderer(ui->side_renderer);
    if (ui->side_window)    SDL_DestroyWindow(ui->side_window);
    if (ui->main_renderer)  SDL_DestroyRenderer(ui->main_renderer);
    if (ui->main_window)    SDL_DestroyWindow(ui->main_window);
    SDL_zerop(ui);
}

/* ------------------------------------------------------------------ */
/* Janela principal                                                    */
/* ------------------------------------------------------------------ */
bool ui_update_texture(UI *ui, const Image *img)
{
    if (ui->image_texture) {
        SDL_DestroyTexture(ui->image_texture);
        ui->image_texture = NULL;
    }
    ui->image_texture = SDL_CreateTextureFromSurface(ui->main_renderer, img->current);
    if (!ui->image_texture) {
        SDL_Log("Erro ao criar a textura da imagem: %s", SDL_GetError());
        return false;
    }
    return true;
}

void ui_resize_main_window(UI *ui, int w, int h)
{
    SDL_SetWindowSize(ui->main_window, w, h);

    /* Centraliza no monitor principal; se a janela nao couber na tela,
       posiciona o canto superior esquerdo em (0,0). */
    SDL_Rect bounds;
    SDL_DisplayID display = SDL_GetPrimaryDisplay();

    if (SDL_GetDisplayBounds(display, &bounds) && w <= bounds.w && h <= bounds.h) {
        SDL_SetWindowPosition(ui->main_window,
                              bounds.x + (bounds.w - w) / 2,
                              bounds.y + (bounds.h - h) / 2);
    } else {
        SDL_SetWindowPosition(ui->main_window, 0, 0);
    }
}

void ui_render_main(UI *ui)
{
    SDL_SetRenderDrawColor(ui->main_renderer, 24, 26, 30, 255);
    SDL_RenderClear(ui->main_renderer);

    if (ui->image_texture) {
        /* A imagem ocupa toda a janela: no modo 1024x768 ela e exibida
           nessa resolucao, e no modo original a janela tem o tamanho dela. */
        SDL_RenderTexture(ui->main_renderer, ui->image_texture, NULL, NULL);
    }
    SDL_RenderPresent(ui->main_renderer);
}

/* ------------------------------------------------------------------ */
/* Janela secundaria: histograma, informacoes e botoes                 */
/* ------------------------------------------------------------------ */
static void draw_histogram(SDL_Renderer *renderer, const Image *img)
{
    SDL_FRect area = { HIST_X, HIST_Y, HIST_W, HIST_H };

    SDL_SetRenderDrawColor(renderer, 248, 249, 250, 255);
    SDL_RenderFillRect(renderer, &area);

    /* Escala pela maior contagem, para o histograma ocupar toda a altura. */
    int max_count = 1;
    for (int k = 0; k < HIST_SIZE; ++k)
        if (img->histogram[k] > max_count) max_count = img->histogram[k];

    const float bar_w = (float)HIST_W / HIST_SIZE;

    SDL_SetRenderDrawColor(renderer, 45, 79, 214, 255);
    for (int k = 0; k < HIST_SIZE; ++k) {
        float bar_h = (float)img->histogram[k] / max_count * HIST_H;
        SDL_FRect bar = { HIST_X + k * bar_w, HIST_Y + HIST_H - bar_h, bar_w, bar_h };
        SDL_RenderFillRect(renderer, &bar);
    }

    SDL_SetRenderDrawColor(renderer, 150, 158, 166, 255);
    SDL_RenderRect(renderer, &area);
}

void ui_render_side(UI *ui, const Image *img, int display_w, int display_h)
{
    SDL_Renderer *r = ui->side_renderer;
    char line[160];

    SDL_SetRenderDrawColor(r, 232, 235, 238, 255);
    SDL_RenderClear(r);

    draw_text(r, ui->font, "Histograma da imagem", HIST_X, 12.0f, INFO_COLOR);
    draw_histogram(r, img);

    /* Marcacao dos extremos do eixo de intensidade. */
    draw_text(r, ui->font, "0", HIST_X, HIST_Y + HIST_H + 3.0f, INFO_COLOR);
    draw_text(r, ui->font, "255", HIST_X + HIST_W - 26.0f, HIST_Y + HIST_H + 3.0f, INFO_COLOR);

    /* Informacoes da analise. */
    float y = HIST_Y + HIST_H + 30.0f;

    SDL_snprintf(line, sizeof(line), "Entrada: %s",
                 img->was_color ? "colorida (convertida para cinza)" : "escala de cinza");
    draw_text(r, ui->font, line, HIST_X, y, INFO_COLOR);
    y += 21.0f;

    SDL_snprintf(line, sizeof(line), "Exibicao: %dx%d  |  original: %dx%d",
                 display_w, display_h, img->current->w, img->current->h);
    draw_text(r, ui->font, line, HIST_X, y, INFO_COLOR);
    y += 21.0f;

    SDL_snprintf(line, sizeof(line), "Media: %.2f  ->  imagem %s",
                 img->mean, image_brightness_label(img->mean));
    draw_text(r, ui->font, line, HIST_X, y, INFO_COLOR);
    y += 21.0f;

    SDL_snprintf(line, sizeof(line), "Desvio padrao: %.2f  ->  contraste %s",
                 img->stddev, image_contrast_label(img->stddev));
    draw_text(r, ui->font, line, HIST_X, y, INFO_COLOR);

    draw_button(r, ui->font, &ui->btn_equalize);
    draw_button(r, ui->font, &ui->btn_resolution);

    SDL_RenderPresent(r);
}

/* ------------------------------------------------------------------ */
/* Mouse sobre os botoes                                               */
/* ------------------------------------------------------------------ */
int ui_handle_mouse(UI *ui, const SDL_Event *event)
{
    const SDL_WindowID side_id = SDL_GetWindowID(ui->side_window);

    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        if (event->motion.windowID != side_id) break;
        /* So muda para hover se o botao nao estiver pressionado. */
        if (ui->btn_equalize.state != BUTTON_PRESSED)
            ui->btn_equalize.state = point_in_button(&ui->btn_equalize, event->motion.x, event->motion.y)
                                     ? BUTTON_HOVER : BUTTON_NEUTRAL;
        if (ui->btn_resolution.state != BUTTON_PRESSED)
            ui->btn_resolution.state = point_in_button(&ui->btn_resolution, event->motion.x, event->motion.y)
                                       ? BUTTON_HOVER : BUTTON_NEUTRAL;
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.windowID != side_id || event->button.button != SDL_BUTTON_LEFT) break;
        if (point_in_button(&ui->btn_equalize, event->button.x, event->button.y))
            ui->btn_equalize.state = BUTTON_PRESSED;
        if (point_in_button(&ui->btn_resolution, event->button.x, event->button.y))
            ui->btn_resolution.state = BUTTON_PRESSED;
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (event->button.windowID != side_id || event->button.button != SDL_BUTTON_LEFT) break;
        int clicked = 0;
        /* O clique so conta se soltar o botao ainda sobre ele. */
        if (ui->btn_equalize.state == BUTTON_PRESSED &&
            point_in_button(&ui->btn_equalize, event->button.x, event->button.y))
            clicked = 1;
        if (ui->btn_resolution.state == BUTTON_PRESSED &&
            point_in_button(&ui->btn_resolution, event->button.x, event->button.y))
            clicked = 2;

        ui->btn_equalize.state = point_in_button(&ui->btn_equalize, event->button.x, event->button.y)
                                 ? BUTTON_HOVER : BUTTON_NEUTRAL;
        ui->btn_resolution.state = point_in_button(&ui->btn_resolution, event->button.x, event->button.y)
                                   ? BUTTON_HOVER : BUTTON_NEUTRAL;
        return clicked;
    }
    default:
        break;
    }
    return 0;
}

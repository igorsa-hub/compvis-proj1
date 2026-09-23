/*
 * Projeto 1 - Processamento de imagens
 * Computacao Visual - Universidade Presbiteriana Mackenzie
 *
 * Uso: proj1 caminho_da_imagem.ext
 */
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#include "image.h"
#include "ui.h"

#define OUTPUT_FILE "output_image.png"

/* Estado do programa. */
typedef struct {
    Image img;
    UI    ui;
    bool  original_resolution;   /* false = 1024x768, true = resolucao da imagem */
    bool  running;
} App;

/* Resolucao em que a imagem esta sendo exibida na janela principal. */
static void current_display_size(const App *app, int *w, int *h)
{
    if (app->original_resolution) {
        *w = app->img.current->w;
        *h = app->img.current->h;
    } else {
        *w = DEFAULT_WIDTH;
        *h = DEFAULT_HEIGHT;
    }
}

/* Redesenha as duas janelas. */
static void redraw(App *app)
{
    int w, h;
    current_display_size(app, &w, &h);
    ui_render_main(&app->ui);
    ui_render_side(&app->ui, &app->img, w, h);
}

/* Requisito 5: alterna entre imagem equalizada e original em escala de cinza. */
static void on_equalize_clicked(App *app)
{
    image_toggle_equalization(&app->img);
    app->ui.btn_equalize.label = app->img.equalized ? "Ver original" : "Equalizar";

    ui_update_texture(&app->ui, &app->img);

    /* No modo "resolucao original" a janela acompanha o tamanho da imagem. */
    if (app->original_resolution)
        ui_resize_main_window(&app->ui, app->img.current->w, app->img.current->h);

    SDL_Log("Histograma %s.", app->img.equalized ? "equalizado" : "restaurado ao original");
}

/* Requisito 6: alterna entre a resolucao original da imagem e 1024x768. */
static void on_resolution_clicked(App *app)
{
    app->original_resolution = !app->original_resolution;
    app->ui.btn_resolution.label = app->original_resolution ? "1024x768" : "Resolucao original";

    int w, h;
    current_display_size(app, &w, &h);
    ui_resize_main_window(&app->ui, w, h);

    SDL_Log("Exibindo a imagem em %dx%d.", w, h);
}

/* Requisito 7: salva a imagem como exibida na janela principal. */
static void save_current_image(App *app)
{
    int w, h;
    current_display_size(app, &w, &h);

    SDL_Surface *to_save = app->img.current;
    SDL_Surface *scaled = NULL;

    /* No modo 1024x768 a imagem exibida esta redimensionada. */
    if (w != app->img.current->w || h != app->img.current->h) {
        scaled = image_scaled_copy(&app->img, w, h);
        if (!scaled) return;
        to_save = scaled;
    }

    bool overwritten = false;
    if (image_save_png(to_save, OUTPUT_FILE, &overwritten)) {
        SDL_Log("Arquivo %s %s (%dx%d).", OUTPUT_FILE,
                overwritten ? "sobrescrito" : "criado", w, h);
    }

    if (scaled) SDL_DestroySurface(scaled);
}

static void handle_event(App *app, const SDL_Event *event)
{
    switch (event->type) {
    case SDL_EVENT_QUIT:
        app->running = false;
        return;

    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        /* Fechar a janela principal encerra o programa. */
        if (event->window.windowID == SDL_GetWindowID(app->ui.main_window))
            app->running = false;
        return;

    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_S && !event->key.repeat)
            save_current_image(app);
        return;

    default:
        break;
    }

    int clicked = ui_handle_mouse(&app->ui, event);
    if (clicked == 1) on_equalize_clicked(app);
    else if (clicked == 2) on_resolution_clicked(app);
}

int main(int argc, char *argv[])
{
    App app = {0};

    /* Requisito 1: o caminho da imagem vem da linha de comando. */
    if (argc != 2) {
        SDL_Log("Uso: %s caminho_da_imagem.ext", argv[0]);
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Erro ao inicializar a SDL: %s", SDL_GetError());
        return 1;
    }
    if (!TTF_Init()) {
        SDL_Log("Erro ao inicializar a SDL_ttf: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /* Requisitos 1 e 2: carrega a imagem e converte para escala de cinza. */
    if (!image_load(&app.img, argv[1])) {
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* Requisito 3: as duas janelas. */
    if (!ui_create(&app.ui)) {
        image_destroy(&app.img);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    if (!ui_update_texture(&app.ui, &app.img)) {
        ui_destroy(&app.ui);
        image_destroy(&app.img);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* A janela principal comeca em 1024x768, centralizada. */
    app.original_resolution = false;
    ui_resize_main_window(&app.ui, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    SDL_Log("Imagem %dx%d carregada. Pressione S para salvar como %s.",
            app.img.current->w, app.img.current->h, OUTPUT_FILE);

    app.running = true;
    redraw(&app);

    while (app.running) {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            handle_event(&app, &event);

        redraw(&app);
        SDL_Delay(16);
    }

    ui_destroy(&app.ui);
    image_destroy(&app.img);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

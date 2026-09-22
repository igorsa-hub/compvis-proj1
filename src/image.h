/*
 * image.h - Carregamento, analise e processamento da imagem.
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define HIST_SIZE 256

/* Guarda a imagem e os dados de analise dela. */
typedef struct {
    SDL_Surface *gray;              /* imagem base em escala de cinza (preservada) */
    SDL_Surface *current;           /* imagem exibida: gray ou a versao equalizada */
    bool was_color;                 /* true se o arquivo de entrada era colorido */
    bool equalized;                 /* true se current aponta para a versao equalizada */
    int histogram[HIST_SIZE];       /* histograma de current */
    double mean;                    /* media de intensidade de current */
    double stddev;                  /* desvio padrao de intensidade de current */
} Image;

/* Carrega o arquivo, converte para escala de cinza e calcula a analise.
   Retorna false e imprime o erro no terminal se algo falhar. */
bool image_load(Image *img, const char *path);

/* Libera tudo que image_load alocou. */
void image_destroy(Image *img);

/* Alterna entre a imagem equalizada e a original em escala de cinza. */
void image_toggle_equalization(Image *img);

/* Salva a superficie em PNG. Escreve em *overwritten se o arquivo ja existia. */
bool image_save_png(SDL_Surface *surface, const char *path, bool *overwritten);

/* Devolve uma copia de current redimensionada para w x h (o chamador destroi). */
SDL_Surface *image_scaled_copy(const Image *img, int w, int h);

/* Classificacoes textuais exibidas na janela secundaria. */
const char *image_brightness_label(double mean);
const char *image_contrast_label(double stddev);

#endif

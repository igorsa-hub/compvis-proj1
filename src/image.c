/*
 * image.c - Carregamento, analise e processamento da imagem.
 */
#include "image.h"

#include <SDL3_image/SDL_image.h>
#include <math.h>

/* Todas as operacoes trabalham neste formato: 4 bytes por pixel (R, G, B, A). */
#define WORK_FORMAT SDL_PIXELFORMAT_RGBA32

/* ------------------------------------------------------------------ */
/* Verifica se a imagem ja esta em escala de cinza (R == G == B em     */
/* todos os pixels).                                                   */
/* ------------------------------------------------------------------ */
static bool is_grayscale(SDL_Surface *surface)
{
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(surface->format);
    const Uint32 *pixels = (const Uint32 *)surface->pixels;
    const int count = surface->w * surface->h;

    SDL_LockSurface(surface);
    for (int i = 0; i < count; ++i) {
        Uint8 r, g, b;
        SDL_GetRGB(pixels[i], fmt, NULL, &r, &g, &b);
        if (r != g || g != b) {
            SDL_UnlockSurface(surface);
            return false;
        }
    }
    SDL_UnlockSurface(surface);
    return true;
}

/* ------------------------------------------------------------------ */
/* Converte para escala de cinza usando Y = 0.2125R + 0.7154G + 0.0721B */
/* ------------------------------------------------------------------ */
static void convert_to_gray(SDL_Surface *surface)
{
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(surface->format);
    Uint32 *pixels = (Uint32 *)surface->pixels;
    const int count = surface->w * surface->h;

    SDL_LockSurface(surface);
    for (int i = 0; i < count; ++i) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[i], fmt, NULL, &r, &g, &b, &a);

        double y = 0.2125 * r + 0.7154 * g + 0.0721 * b;
        if (y > 255.0) y = 255.0;

        Uint8 level = (Uint8)(y + 0.5);
        pixels[i] = SDL_MapRGBA(fmt, NULL, level, level, level, a);
    }
    SDL_UnlockSurface(surface);
}

/* ------------------------------------------------------------------ */
/* Calcula histograma, media e desvio padrao da imagem atual.          */
/* ------------------------------------------------------------------ */
static void compute_stats(Image *img)
{
    SDL_Surface *surface = img->current;
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(surface->format);
    const Uint32 *pixels = (const Uint32 *)surface->pixels;
    const int count = surface->w * surface->h;

    SDL_memset(img->histogram, 0, sizeof(img->histogram));

    SDL_LockSurface(surface);
    for (int i = 0; i < count; ++i) {
        Uint8 r, g, b;
        SDL_GetRGB(pixels[i], fmt, NULL, &r, &g, &b);
        img->histogram[r]++;   /* imagem ja esta em cinza: R == G == B */
    }
    SDL_UnlockSurface(surface);

    /* Media: soma de (intensidade * quantidade de pixels) / total. */
    double sum = 0.0;
    for (int k = 0; k < HIST_SIZE; ++k)
        sum += (double)k * img->histogram[k];
    img->mean = sum / count;

    /* Desvio padrao a partir do mesmo histograma. */
    double variance = 0.0;
    for (int k = 0; k < HIST_SIZE; ++k) {
        double diff = k - img->mean;
        variance += diff * diff * img->histogram[k];
    }
    img->stddev = sqrt(variance / count);
}

/* ------------------------------------------------------------------ */
/* Equalizacao: s_k = (L-1) * soma acumulada de p(r_j)                 */
/* ------------------------------------------------------------------ */
static SDL_Surface *build_equalized(const Image *img)
{
    SDL_Surface *src = img->gray;
    SDL_Surface *dst = SDL_CreateSurface(src->w, src->h, WORK_FORMAT);
    if (!dst) {
        SDL_Log("Erro ao criar superficie para equalizacao: %s", SDL_GetError());
        return NULL;
    }

    const int count = src->w * src->h;

    /* Histograma da imagem em escala de cinza (base da equalizacao). */
    int hist[HIST_SIZE] = {0};
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(src->format);
    const Uint32 *src_pixels = (const Uint32 *)src->pixels;

    SDL_LockSurface(src);
    for (int i = 0; i < count; ++i) {
        Uint8 r, g, b;
        SDL_GetRGB(src_pixels[i], fmt, NULL, &r, &g, &b);
        hist[r]++;
    }

    /* Tabela de conversao: acumula a probabilidade e escala por L-1. */
    Uint8 lut[HIST_SIZE];
    double accumulated = 0.0;
    for (int k = 0; k < HIST_SIZE; ++k) {
        accumulated += (double)hist[k] / count;
        double s = (HIST_SIZE - 1) * accumulated;
        lut[k] = (Uint8)(s + 0.5);
    }

    /* Aplica a tabela pixel a pixel. */
    Uint32 *dst_pixels = (Uint32 *)dst->pixels;
    SDL_LockSurface(dst);
    for (int i = 0; i < count; ++i) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(src_pixels[i], fmt, NULL, &r, &g, &b, &a);
        Uint8 level = lut[r];
        dst_pixels[i] = SDL_MapRGBA(fmt, NULL, level, level, level, a);
    }
    SDL_UnlockSurface(dst);
    SDL_UnlockSurface(src);

    return dst;
}

/* ------------------------------------------------------------------ */
/* API publica                                                         */
/* ------------------------------------------------------------------ */
bool image_load(Image *img, const char *path)
{
    SDL_zerop(img);

    SDL_Surface *loaded = IMG_Load(path);
    if (!loaded) {
        SDL_Log("Erro: nao foi possivel carregar \"%s\". %s", path, SDL_GetError());
        SDL_Log("Verifique se o arquivo existe e se e um formato de imagem valido.");
        return false;
    }

    /* Converte para um formato unico para simplificar o acesso aos pixels. */
    SDL_Surface *work = SDL_ConvertSurface(loaded, WORK_FORMAT);
    SDL_DestroySurface(loaded);
    if (!work) {
        SDL_Log("Erro ao converter a imagem para RGBA32: %s", SDL_GetError());
        return false;
    }

    img->was_color = !is_grayscale(work);
    if (img->was_color) {
        SDL_Log("Imagem de entrada: colorida. Convertendo para escala de cinza...");
        convert_to_gray(work);
    } else {
        SDL_Log("Imagem de entrada: ja esta em escala de cinza.");
    }

    img->gray = work;
    img->current = work;
    img->equalized = false;
    compute_stats(img);
    return true;
}

void image_destroy(Image *img)
{
    if (img->current && img->current != img->gray)
        SDL_DestroySurface(img->current);
    if (img->gray)
        SDL_DestroySurface(img->gray);
    SDL_zerop(img);
}

void image_toggle_equalization(Image *img)
{
    if (img->equalized) {
        /* Volta para a imagem original em escala de cinza. */
        SDL_DestroySurface(img->current);
        img->current = img->gray;
        img->equalized = false;
    } else {
        SDL_Surface *equalized = build_equalized(img);
        if (!equalized) return;
        img->current = equalized;
        img->equalized = true;
    }
    compute_stats(img);
}

SDL_Surface *image_scaled_copy(const Image *img, int w, int h)
{
    SDL_Surface *copy = SDL_CreateSurface(w, h, WORK_FORMAT);
    if (!copy) {
        SDL_Log("Erro ao criar superficie redimensionada: %s", SDL_GetError());
        return NULL;
    }
    if (!SDL_BlitSurfaceScaled(img->current, NULL, copy, NULL, SDL_SCALEMODE_LINEAR)) {
        SDL_Log("Erro ao redimensionar a imagem: %s", SDL_GetError());
        SDL_DestroySurface(copy);
        return NULL;
    }
    return copy;
}

bool image_save_png(SDL_Surface *surface, const char *path, bool *overwritten)
{
    SDL_PathInfo info;
    *overwritten = SDL_GetPathInfo(path, &info);   /* true se o arquivo ja existe */

    if (!IMG_SavePNG(surface, path)) {
        SDL_Log("Erro ao salvar \"%s\": %s", path, SDL_GetError());
        return false;
    }
    return true;
}

/* Faixas de classificacao sobre os 256 niveis de intensidade. */
const char *image_brightness_label(double mean)
{
    if (mean < 85.0)  return "escura";
    if (mean < 170.0) return "media";
    return "clara";
}

const char *image_contrast_label(double stddev)
{
    if (stddev < 35.0) return "baixo";
    if (stddev < 70.0) return "medio";
    return "alto";
}

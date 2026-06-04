/**
 * pgm_io.h
 * ========
 * Module d'entrée/sortie pour images PGM (P5 binaire / P2 ASCII)
 * et PPM P6 couleur (RGB 8 bits/canal).
 *
 * Convention mémoire : tableaux row-major continus.
 *   PGM : data[r*width + c]           — 1 octet/pixel
 *   PPM : data[(r*width+c)*3 + canal] — 3 octets/pixel (R,G,B)
 */
#ifndef PGM_IO_H
#define PGM_IO_H

#include <stdint.h>

/* ── Structures ──────────────────────────────────────────────────── */

/** Image en niveaux de gris (PGM) */
typedef struct {
    int      width;
    int      height;
    int      maxval;    /**< valeur max (typiquement 255) */
    uint8_t *data;      /**< 1 octet/pixel, row-major     */
} PGMImage;

/** Image couleur RGB (PPM) */
typedef struct {
    int      width;
    int      height;
    uint8_t *data;      /**< 3 octets/pixel : R G B ...   */
} PPMImage;

/* ── Macros d'accès ──────────────────────────────────────────────── */
#define PGM_AT(img,r,c)    ((img)->data[(r)*(img)->width+(c)])
#define PPM_R(img,r,c)     ((img)->data[((r)*(img)->width+(c))*3+0])
#define PPM_G(img,r,c)     ((img)->data[((r)*(img)->width+(c))*3+1])
#define PPM_B(img,r,c)     ((img)->data[((r)*(img)->width+(c))*3+2])

/* ── PGM ─────────────────────────────────────────────────────────── */
PGMImage *pgm_load(const char *filename);
int       pgm_save(const PGMImage *img, const char *filename);
PGMImage *pgm_create(int w, int h, int maxval);
void      pgm_free(PGMImage *img);

/* ── PPM ─────────────────────────────────────────────────────────── */
PPMImage *ppm_load(const char *filename);
int       ppm_save(const PPMImage *img, const char *filename);
PPMImage *ppm_create(int w, int h);
void      ppm_free(PPMImage *img);

/** Convertit PGM → PPM (R=G=B=gris) pour superposition de couleurs */
PPMImage *ppm_from_pgm(const PGMImage *gray);

#endif /* PGM_IO_H */

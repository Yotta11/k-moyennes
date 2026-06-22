
#ifndef PGM_IO_H
#define PGM_IO_H

#include <stdint.h>



/** Image en niveaux de gris (PGM) */
typedef struct {
    int      width;
    int      height;
    int      maxval;    
    uint8_t *data;      
} PGMImage;


typedef struct {
    int      width;
    int      height;
    uint8_t *data;      /**< 3 octets/pixel : R G B ...   */
} PPMImage;

// macros d'acces'
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

#endif 

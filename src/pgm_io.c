 //PGM (P5/P2) et PPM (P6).
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pgm_io.h"

static void skip_ws(FILE *f)
{
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '#') { while ((c = fgetc(f)) != EOF && c != '\n'); }
        else if (!isspace((unsigned char)c)) { ungetc(c, f); break; }
    }
}

//pgm 8bits
PGMImage *pgm_create(int w, int h, int maxval)
{
    PGMImage *img = malloc(sizeof(PGMImage));
    if (!img) return NULL;
    img->width = w; img->height = h; img->maxval = maxval;
    img->data  = malloc((size_t)w * h);
    if (!img->data) { free(img); return NULL; }
    return img;
}

PGMImage *pgm_load(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) { perror(filename); return NULL; }

    char magic[4] = {0};
    if(fscanf(f, "%3s", magic) !=1){
    fprintf(stderr, "Erreur de lecture du format magique\n");
    
    return NULL;

    }
    int p5 = !strcmp(magic,"P5"), p2 = !strcmp(magic,"P2");
    if (!p5 && !p2) {
        fprintf(stderr,"pgm_load: format '%s' non supporté dans %s\n",magic,filename);
        fclose(f); return NULL;
    }
    int w, h, mv;
    skip_ws(f); 
    if(fscanf(f,"%d",&w)!=1){
    fprintf(stderr, "Erreur de lecture du format magique\n");
   
    return NULL;    
    }
    skip_ws(f); 
    if(fscanf(f,"%d",&h)!=1){
    fprintf(stderr, "Erreur de lecture du format magique\n");
   
    return NULL;   
     }
    skip_ws(f); 
    if(fscanf(f,"%d",&mv)!=1){
        fprintf(stderr, "Erreur de lecture du format magique\n");
   
    return NULL;        
    }
    fgetc(f); /* séparateur final */

    PGMImage *img = pgm_create(w, h, mv);
    if (!img) { fclose(f); return NULL; }

    if (p5) {
        if (fread(img->data, 1, (size_t)w*h, f) != (size_t)w*h) {
            fprintf(stderr,"pgm_load: données incomplètes %s\n",filename);
            pgm_free(img); fclose(f); return NULL;
        }
    } else {
        for (int i = 0; i < w*h; i++) {
            int v;
if (fscanf(f, "%d", &v) != 1) {
    fprintf(stderr, "Erreur de lecture du pixel\n");
    fclose(f);
    return NULL;
}

  img->data[i] = (uint8_t)v;
        }
    }
    fclose(f);
    return img;
}

int pgm_save(const PGMImage *img, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f) { perror(filename); return -1; }
    fprintf(f, "P5\n%d %d\n%d\n", img->width, img->height, img->maxval);
    fwrite(img->data, 1, (size_t)img->width * img->height, f);
    fclose(f); return 0;
}

void pgm_free(PGMImage *img)
{
    if (!img) return;
    free(img->data); free(img);
}

//ppm 24bits

PPMImage *ppm_create(int w, int h)
{
    PPMImage *img = malloc(sizeof(PPMImage));
    if (!img) return NULL;
    img->width = w; img->height = h;
    img->data  = malloc((size_t)w * h * 3);
    if (!img->data) { free(img); return NULL; }
    return img;
}

PPMImage *ppm_load(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) { perror(filename); return NULL; }

    char magic[4] = {0};
    if(fscanf(f, "%3s", magic) !=1){
        fprintf(stderr, "Erreur de lecture du format magique\n");
   
    return NULL;    
    }
    int p6 = !strcmp(magic,"P6"), p3 = !strcmp(magic,"P3");
    if (!p6 && !p3) {
        /* Tentative de chargement PGM converti en PPM */
        fprintf(stderr,"ppm_load: format '%s' non supporté dans %s\n",magic,filename);
        fclose(f); return NULL;
    }
    int w, h, mv;
    skip_ws(f);
      if(fscanf(f,"%d",&w)!=1){
        fprintf(stderr, "Erreur de lecture du format magique\n");
    return NULL;    
    }
    skip_ws(f); if(fscanf(f,"%d",&h)!=1){
        fprintf(stderr, "Erreur de lecture du format magique\n");
   
    return NULL;        
    }
    skip_ws(f); if(fscanf(f,"%d",&mv)!=1){
        fprintf(stderr, "Erreur de lecture du format magique\n");
   
    return NULL;        
    }
    fgetc(f);

    PPMImage *img = ppm_create(w, h);
    if (!img) { fclose(f); return NULL; }

    if (p6) {
       if(fread(img->data, 1, (size_t)w*h*3, f)!=1){
        
    }
    } else {
        for (int i = 0; i < w*h*3; i++) {
            int v; 
    if (fscanf(f, "%d", &v) != 1) {
    fprintf(stderr, "Erreur de lecture du pixel\n");
    fclose(f);
    return NULL;
    }
     img->data[i] = (uint8_t)v;
        }
    }
    fclose(f);
    return img;
}

int ppm_save(const PPMImage *img, const char *filename)
{
    FILE *f = fopen(filename, "wb");
    if (!f) { perror(filename); return -1; }
    fprintf(f, "P6\n%d %d\n255\n", img->width, img->height);
    fwrite(img->data, 1, (size_t)img->width * img->height * 3, f);
    fclose(f); return 0;
}

void ppm_free(PPMImage *img)
{
    if (!img) return;
    free(img->data); free(img);
}

PPMImage *ppm_from_pgm(const PGMImage *gray)
{
    PPMImage *rgb = ppm_create(gray->width, gray->height);
    if (!rgb) return NULL;
    int n = gray->width * gray->height;
    for (int i = 0; i < n; i++) {
        rgb->data[i*3+0] = gray->data[i];
        rgb->data[i*3+1] = gray->data[i];
        rgb->data[i*3+2] = gray->data[i];
    }
    return rgb;
}

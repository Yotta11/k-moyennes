/**

 * Implémentation de l'algorithme K-Moyennes (Lloyd) pour images PGM et PPM.
 *
 * Détail de l'implémentation :
 *
 *  ► Initialisation K-Means++ (init_pp=1) :
 *    - Choisir le 1er centroïde uniformément au hasard
 *    - Pour chaque centroïde suivant : choisir un pixel avec une probabilité
 *      proportionnelle à D²(x) = distance² au centroïde le plus proche déjà
 *      choisi → garantit une meilleure couverture initiale et accélère la
 *      convergence (Arthur & Vassilvitskii, 2007)
 *
 *  ► Initialisation uniforme (init_pp=0) :
 *    - K centroïdes régulièrement espacés dans [min_val, max_val]
 *
 *  ► Convergence :
 *    - Arrêt si max(|centroïde_nouveau − centroïde_ancien|) < epsilon
 *    - Ou si n_iter >= max_iter
 *
 *  ► Gestion des classes vides :
 *    - Si une classe perd tous ses pixels, son centroïde est réinitialisé
 *      au pixel le plus éloigné du centroïde global (stratégie de split)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "kmeans.h"

/* ── Générateur pseudo-aléatoire simple (LCG) ───────────────────── */
static unsigned int rng_state = 42;
static void   rng_seed(unsigned int s) { rng_state = s; }
static double rng_uniform(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return (rng_state >> 1) / (double)0x7FFFFFFF;
}
static int rng_int(int n) { return (int)(rng_uniform() * n) % n; }

/* ── Palette de couleurs pour visualisation ──────────────────────── */
static const uint8_t PALETTE[][3] = {
    {220,  50,  50}, {  50, 120, 220}, { 50, 200,  80},
    {255, 165,   0}, {180,  50, 220}, {  0, 200, 200},
    {255, 200,   0}, {100,  60,  20}, {255, 130, 180},
    { 80, 180,  80}, {  0,  80, 160}, {200, 100,   0},
    {150, 200, 255}, {255,  80,  80}, { 80, 255, 200},
    {200, 200, 100}
};
#define N_COLORS  ((int)(sizeof(PALETTE)/sizeof(PALETTE[0])))

/* ══════════════════════════════════════════════════════════════════ */
/*  NIVEAUX DE GRIS                                                   */
/* ══════════════════════════════════════════════════════════════════ */

/* ── Allocation / libération ─────────────────────────────────────── */
static KMeansResult *result_alloc(int k, int n)
{
    KMeansResult *r = calloc(1, sizeof(KMeansResult));
    if (!r) return NULL;
    r->k        = k;
    r->n_pixels = n;
    r->labels        = malloc((size_t)n * sizeof(int));
    r->centroids     = malloc((size_t)k * sizeof(double));
    r->class_size    = calloc((size_t)k, sizeof(long));
    r->class_inertia = calloc((size_t)k, sizeof(double));
    if (!r->labels || !r->centroids || !r->class_size || !r->class_inertia) {
        kmeans_free(r); return NULL;
    }
    return r;
}

void kmeans_free(KMeansResult *r)
{
    if (!r) return;
    free(r->labels); free(r->centroids);
    free(r->class_size); free(r->class_inertia);
    free(r);
}

/* ── Initialisation K-Means++ ────────────────────────────────────── */
static void init_kpp(const uint8_t *data, int n, double *centroids, int k)
{
    /* 1er centroïde : pixel aléatoire */
    centroids[0] = data[rng_int(n)];

    double *dist2 = malloc((size_t)n * sizeof(double));
    if (!dist2) return;

    for (int c = 1; c < k; c++) {
        /* Calcul de D²(x) pour chaque pixel */
        double sum = 0.0;
        for (int i = 0; i < n; i++) {
            double min_d2 = DBL_MAX;
            for (int j = 0; j < c; j++) {
                double d = data[i] - centroids[j];
                double d2 = d * d;
                if (d2 < min_d2) min_d2 = d2;
            }
            dist2[i] = min_d2;
            sum += min_d2;
        }
        /* Tirage proportionnel à D² (roulette wheel) */
        double target = rng_uniform() * sum;
        double acc = 0.0;
        int chosen = n - 1;
        for (int i = 0; i < n; i++) {
            acc += dist2[i];
            if (acc >= target) { chosen = i; break; }
        }
        centroids[c] = data[chosen];
    }
    free(dist2);
}

/* ── Initialisation uniforme ─────────────────────────────────────── */
static void init_uniform(const uint8_t *data, int n, double *centroids, int k)
{
    uint8_t vmin = 255, vmax = 0;
    for (int i = 0; i < n; i++) {
        if (data[i] < vmin) vmin = data[i];
        if (data[i] > vmax) vmax = data[i];
    }
    for (int c = 0; c < k; c++)
        centroids[c] = vmin + (double)(vmax - vmin) * c / (k - 1);
}

/* ── Algorithme principal (niveaux de gris) ──────────────────────── */
KMeansResult *kmeans_pgm(const PGMImage *img, const KMeansParams *p)
{
    int n = img->width * img->height;
    int k = p->k;
    rng_seed(p->seed);

    KMeansResult *r = result_alloc(k, n);
    if (!r) return NULL;

    /* ─ 1. Initialisation des centroïdes ─ */
    if (p->init_pp)
        init_kpp(img->data, n, r->centroids, k);
    else
        init_uniform(img->data, n, r->centroids, k);

    double *new_centroids = malloc((size_t)k * sizeof(double));
    long   *counts        = malloc((size_t)k * sizeof(long));
    if (!new_centroids || !counts) {
        free(new_centroids); free(counts);
        kmeans_free(r); return NULL;
    }

    r->converged = 0;

    for (int iter = 0; iter < p->max_iter; iter++) {

        /* ─ 2. Affectation : pixel → centroïde le plus proche ─ */
        for (int i = 0; i < n; i++) {
            double v = img->data[i];
            double min_d2 = DBL_MAX;
            int    best   = 0;
            for (int c = 0; c < k; c++) {
                double d = v - r->centroids[c];
                double d2 = d * d;
                if (d2 < min_d2) { min_d2 = d2; best = c; }
            }
            r->labels[i] = best;
        }

        /* ─ 3. Mise à jour des centroïdes ─ */
        memset(new_centroids, 0, (size_t)k * sizeof(double));
        memset(counts,        0, (size_t)k * sizeof(long));
        for (int i = 0; i < n; i++) {
            int c = r->labels[i];
            new_centroids[c] += img->data[i];
            counts[c]++;
        }

        /* Gestion des classes vides : réaffectation au pixel le plus loin */
        for (int c = 0; c < k; c++) {
            if (counts[c] == 0) {
                /* Trouver le pixel le plus éloigné de son centroïde actuel */
                double max_d2 = -1.0;
                int    worst  = 0;
                for (int i = 0; i < n; i++) {
                    double d = img->data[i] - r->centroids[r->labels[i]];
                    double d2 = d * d;
                    if (d2 > max_d2) { max_d2 = d2; worst = i; }
                }
                new_centroids[c] = img->data[worst];
                counts[c]        = 1;
            } else {
                new_centroids[c] /= counts[c];
            }
        }

        /* ─ 4. Test de convergence ─ */
        double max_move = 0.0;
        for (int c = 0; c < k; c++) {
            double mv = fabs(new_centroids[c] - r->centroids[c]);
            if (mv > max_move) max_move = mv;
        }
        memcpy(r->centroids, new_centroids, (size_t)k * sizeof(double));
        r->n_iter = iter + 1;

        if (max_move < p->epsilon) { r->converged = 1; break; }
    }

    /* ─ 5. Calcul des statistiques finales ─ */
    /* Réaffectation finale avec les centroïdes convergés */
    for (int i = 0; i < n; i++) {
        double v = img->data[i];
        double min_d2 = DBL_MAX;
        int    best   = 0;
        for (int c = 0; c < k; c++) {
            double d = v - r->centroids[c];
            double d2 = d * d;
            if (d2 < min_d2) { min_d2 = d2; best = c; }
        }
        r->labels[i] = best;
    }

    memset(r->class_size,    0, (size_t)k * sizeof(long));
    memset(r->class_inertia, 0, (size_t)k * sizeof(double));
    r->total_inertia = 0.0;
    for (int i = 0; i < n; i++) {
        int    c  = r->labels[i];
        double d  = img->data[i] - r->centroids[c];
        double d2 = d * d;
        r->class_size[c]++;
        r->class_inertia[c] += d2;
        r->total_inertia     += d2;
    }

    free(new_centroids);
    free(counts);
    return r;
}

/* ── Rendu niveaux de gris ───────────────────────────────────────── */
PGMImage *kmeans_render_gray(const PGMImage *src, const KMeansResult *r)
{
    PGMImage *out = pgm_create(src->width, src->height, 255);
    if (!out) return NULL;
    int n = src->width * src->height;
    for (int i = 0; i < n; i++)
        out->data[i] = (uint8_t)(r->centroids[r->labels[i]] + 0.5);
    return out;
}

/* ── Rendu coloré (palette) ──────────────────────────────────────── */
PPMImage *kmeans_render_color(const PGMImage *src, const KMeansResult *r)
{
    PPMImage *out = ppm_create(src->width, src->height);
    if (!out) return NULL;
    int n = src->width * src->height;
    for (int i = 0; i < n; i++) {
        int c = r->labels[i] % N_COLORS;
        out->data[i*3+0] = PALETTE[c][0];
        out->data[i*3+1] = PALETTE[c][1];
        out->data[i*3+2] = PALETTE[c][2];
    }
    return out;
}

/* ── Affichage résultat gris ─────────────────────────────────────── */
void kmeans_print_result(const KMeansResult *r, const char *name)
{
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║  K-Moyennes (gris) : %-32s║\n", name);
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  K=%d  |  Itérations : %3d  |  Convergé : %s         ║\n",
           r->k, r->n_iter, r->converged ? "OUI" : "NON");
    printf("║  Inertie totale (WCSS) : %-27.2f║\n", r->total_inertia);
    printf("╠══════╦═══════════╦═══════════╦═════════════╦═════════╣\n");
    printf("║Classe║ Centroïde ║  Pixels   ║      %%      ║ Inertie ║\n");
    printf("╠══════╬═══════════╬═══════════╬═════════════╬═════════╣\n");
    for (int c = 0; c < r->k; c++) {
        double pct = 100.0 * r->class_size[c] / r->n_pixels;
        printf("║  %2d  ║  %7.2f  ║  %7ld  ║   %6.2f %%   ║%9.1f║\n",
               c, r->centroids[c], r->class_size[c], pct, r->class_inertia[c]);
    }
    printf("╚══════╩═══════════╩═══════════╩═════════════╩═════════╝\n");
}

//couleur rgb

static KMeansResultRGB *result_alloc_rgb(int k, int n)
{
    KMeansResultRGB *r = calloc(1, sizeof(KMeansResultRGB));
    if (!r) return NULL;
    r->k = k; r->n_pixels = n;
    r->labels        = malloc((size_t)n * sizeof(int));
    r->centroids     = malloc((size_t)k * sizeof(double[3]));
    r->class_size    = calloc((size_t)k, sizeof(long));
    r->class_inertia = calloc((size_t)k, sizeof(double));
    if (!r->labels || !r->centroids || !r->class_size || !r->class_inertia) {
        kmeans_free_rgb(r); return NULL;
    }
    return r;
}

void kmeans_free_rgb(KMeansResultRGB *r)
{
    if (!r) return;
    free(r->labels); free(r->centroids);
    free(r->class_size); free(r->class_inertia);
    free(r);
}

/* Distance euclidienne² dans RGB */
static inline double dist3sq(const double *a, const uint8_t *b)
{
    double dr = a[0]-b[0], dg = a[1]-b[1], db = a[2]-b[2];
    return dr*dr + dg*dg + db*db;
}

/* Initialisation K-Means++ couleur */
static void init_kpp_rgb(const uint8_t *data, int n, double (*centroids)[3], int k)
{
    /* 1er centroïde : pixel aléatoire */
    int idx = rng_int(n);
    centroids[0][0] = data[idx*3+0];
    centroids[0][1] = data[idx*3+1];
    centroids[0][2] = data[idx*3+2];

    double *dist2 = malloc((size_t)n * sizeof(double));
    if (!dist2) return;

    for (int c = 1; c < k; c++) {
        double sum = 0.0;
        for (int i = 0; i < n; i++) {
            double min_d2 = DBL_MAX;
            for (int j = 0; j < c; j++) {
                double d2 = dist3sq(centroids[j], data + i*3);
                if (d2 < min_d2) min_d2 = d2;
            }
            dist2[i] = min_d2; sum += min_d2;
        }
        double target = rng_uniform() * sum;
        double acc = 0.0; int chosen = n-1;
        for (int i = 0; i < n; i++) {
            acc += dist2[i];
            if (acc >= target) { chosen = i; break; }
        }
        centroids[c][0] = data[chosen*3+0];
        centroids[c][1] = data[chosen*3+1];
        centroids[c][2] = data[chosen*3+2];
    }
    free(dist2);
}


static void init_uniform_rgb(int n, double (*centroids)[3], int k)
{
    (void)n;
    for (int c = 0; c < k; c++) {
        double t = (double)c / (k-1);
        /* Dégradé noir → blanc dans l'espace RGB */
        centroids[c][0] = centroids[c][1] = centroids[c][2] = t * 255.0;
    }
}

KMeansResultRGB *kmeans_ppm(const PPMImage *img, const KMeansParams *p)
{
    int n = img->width * img->height;
    int k = p->k;
    rng_seed(p->seed);

    KMeansResultRGB *r = result_alloc_rgb(k, n);
    if (!r) return NULL;

    /* ─ 1. Initialisation ─ */
    if (p->init_pp)
        init_kpp_rgb(img->data, n, r->centroids, k);
    else
        init_uniform_rgb(n, r->centroids, k);

    double (*new_c)[3] = malloc((size_t)k * sizeof(double[3]));
    long   *counts     = malloc((size_t)k * sizeof(long));
    if (!new_c || !counts) {
        free(new_c); free(counts); kmeans_free_rgb(r); return NULL;
    }

    r->converged = 0;

    for (int iter = 0; iter < p->max_iter; iter++) {

        /* ─ 2. Affectation ─ */
        for (int i = 0; i < n; i++) {
            double min_d2 = DBL_MAX; int best = 0;
            for (int c = 0; c < k; c++) {
                double d2 = dist3sq(r->centroids[c], img->data + i*3);
                if (d2 < min_d2) { min_d2 = d2; best = c; }
            }
            r->labels[i] = best;
        }

        /* ─ 3. Mise à jour ─ */
        memset(new_c,  0, (size_t)k * sizeof(double[3]));
        memset(counts, 0, (size_t)k * sizeof(long));
        for (int i = 0; i < n; i++) {
            int c = r->labels[i];
            new_c[c][0] += img->data[i*3+0];
            new_c[c][1] += img->data[i*3+1];
            new_c[c][2] += img->data[i*3+2];
            counts[c]++;
        }
        for (int c = 0; c < k; c++) {
            if (counts[c] == 0) {
                /* Classe vide : réinitialisation */
                int worst = rng_int(n);
                new_c[c][0] = img->data[worst*3+0];
                new_c[c][1] = img->data[worst*3+1];
                new_c[c][2] = img->data[worst*3+2];
            } else {
                new_c[c][0] /= counts[c];
                new_c[c][1] /= counts[c];
                new_c[c][2] /= counts[c];
            }
        }

        /* ─ 4. Convergence ─ */
        double max_move = 0.0;
        for (int c = 0; c < k; c++) {
            double d = fabs(new_c[c][0]-r->centroids[c][0])
                     + fabs(new_c[c][1]-r->centroids[c][1])
                     + fabs(new_c[c][2]-r->centroids[c][2]);
            if (d > max_move) max_move = d;
            memcpy(r->centroids[c], new_c[c], 3*sizeof(double));
        }
        r->n_iter = iter + 1;
        if (max_move < p->epsilon) { r->converged = 1; break; }
    }

    /* ─ 5. Statistiques finales ─ */
    /* Réaffectation finale */
    for (int i = 0; i < n; i++) {
        double min_d2 = DBL_MAX; int best = 0;
        for (int c = 0; c < k; c++) {
            double d2 = dist3sq(r->centroids[c], img->data + i*3);
            if (d2 < min_d2) { min_d2 = d2; best = c; }
        }
        r->labels[i] = best;
    }
    memset(r->class_size, 0, (size_t)k*sizeof(long));
    memset(r->class_inertia, 0, (size_t)k*sizeof(double));
    r->total_inertia = 0.0;
    for (int i = 0; i < n; i++) {
        int c = r->labels[i];
        double d2 = dist3sq(r->centroids[c], img->data + i*3);
        r->class_size[c]++;
        r->class_inertia[c] += d2;
        r->total_inertia     += d2;
    }

    free(new_c); free(counts);
    return r;
}

PPMImage *kmeans_render_rgb(const PPMImage *src, const KMeansResultRGB *r)
{
    PPMImage *out = ppm_create(src->width, src->height);
    if (!out) return NULL;
    int n = src->width * src->height;
    for (int i = 0; i < n; i++) {
        int c = r->labels[i];
        out->data[i*3+0] = (uint8_t)(r->centroids[c][0] + 0.5);
        out->data[i*3+1] = (uint8_t)(r->centroids[c][1] + 0.5);
        out->data[i*3+2] = (uint8_t)(r->centroids[c][2] + 0.5);
    }
    return out;
}

void kmeans_print_result_rgb(const KMeansResultRGB *r, const char *name)
{
    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║  K-Moyennes (RGB) : %-45s║\n", name);
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║  K=%d  |  Itérations : %3d  |  Convergé : %s                   ║\n",
           r->k, r->n_iter, r->converged ? "OUI" : "NON");
    printf("║  Inertie totale (WCSS) : %-39.2f║\n", r->total_inertia);
    printf("╠══════╦═══════════════════════╦════════════╦══════════╦══════════╣\n");
    printf("║Classe║  Centroïde (R, G, B)  ║  Pixels    ║   %%     ║ Inertie  ║\n");
    printf("╠══════╬═══════════════════════╬════════════╬══════════╬══════════╣\n");
    for (int c = 0; c < r->k; c++) {
        double pct = 100.0 * r->class_size[c] / r->n_pixels;
        printf("║  %2d  ║ (%5.1f,%5.1f,%5.1f) ║  %8ld  ║  %6.2f%%  ║%10.0f║\n",
               c, r->centroids[c][0], r->centroids[c][1], r->centroids[c][2],
               r->class_size[c], pct, r->class_inertia[c]);
    }
    printf("╚══════╩═══════════════════════╩════════════╩══════════╩══════════╝\n");
}


#ifndef KMEANS_H
#define KMEANS_H

#include "pgm_io.h"

/* ── Paramètres de l'algorithme ──────────────────────────────────── */
typedef struct {
    int    k;           /**< nombre de classes (≥ 2)                  */
    int    max_iter;    /**< nombre maximum d'itérations (ex : 100)   */
    double epsilon;     /**< seuil de convergence (ex : 0.5)          */
    int    init_pp;     /**< 1 = K-Means++, 0 = initialisation uniforme */
    unsigned int seed;  /**< graine aléatoire pour reproductibilité   */
} KMeansParams;

/** Paramètres par défaut */
#define KMEANS_DEFAULTS { .k=4, .max_iter=100, .epsilon=0.5, .init_pp=1, .seed=42 }

/* ── Résultat K-Moyennes (niveaux de gris) ───────────────────────── */
typedef struct {
    int      k;                 /**< nombre de classes               */
    int      n_pixels;          /**< nombre total de pixels          */
    int     *labels;            /**< étiquette [0..k-1] par pixel    */
    double  *centroids;         /**< centroïde de chaque classe [k]  */
    long    *class_size;        /**< nombre de pixels par classe [k] */
    double  *class_inertia;     /**< WCSS partielle par classe  [k]  */
    double   total_inertia;     /**< WCSS totale (critère global)    */
    int      n_iter;            /**< itérations effectuées           */
    int      converged;         /**< 1 si convergence atteinte       */
} KMeansResult;

/* ── Résultat K-Moyennes couleur RGB ─────────────────────────────── */
typedef struct {
    int      k;
    int      n_pixels;
    int     *labels;
    double (*centroids)[3];     /**< centroïde [k][3] : (R,G,B)     */
    long    *class_size;
    double  *class_inertia;
    double   total_inertia;
    int      n_iter;
    int      converged;
} KMeansResultRGB;

/* ── API niveaux de gris ─────────────────────────────────────────── */
/**
 * Lance l'algorithme K-Moyennes sur une image PGM.
 * @return résultat alloué dynamiquement, ou NULL en cas d'erreur
 */
KMeansResult *kmeans_pgm(const PGMImage *img, const KMeansParams *p);

/**
 * Génère l'image segmentée : chaque pixel prend la valeur du centroïde
 * de sa classe (image en niveaux de gris uniformes par région).
 */
PGMImage *kmeans_render_gray(const PGMImage *src, const KMeansResult *r);

/**
 * Génère une image de segmentation colorée : chaque classe reçoit
 * une couleur distincte pour visualisation.
 */
PPMImage *kmeans_render_color(const PGMImage *src, const KMeansResult *r);

/** Libère un KMeansResult. */
void kmeans_free(KMeansResult *r);

/* ── API couleur RGB ─────────────────────────────────────────────── */

/**
 * Lance l'algorithme K-Moyennes sur une image PPM couleur.
 */
KMeansResultRGB *kmeans_ppm(const PPMImage *img, const KMeansParams *p);

/**
 * Génère l'image segmentée couleur : chaque pixel prend la couleur
 * moyenne (R,G,B) de sa classe.
 */
PPMImage *kmeans_render_rgb(const PPMImage *src, const KMeansResultRGB *r);

/** Libère un KMeansResultRGB. */
void kmeans_free_rgb(KMeansResultRGB *r);

/* ── Affichage des résultats ─────────────────────────────────────── */

/** Affiche un tableau récapitulatif sur stdout. */
void kmeans_print_result(const KMeansResult *r, const char *imgname);
void kmeans_print_result_rgb(const KMeansResultRGB *r, const char *imgname);

#endif /* KMEANS_H */

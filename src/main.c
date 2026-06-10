/**
 
 
 * Usage : ./kmeans [image.pgm|image.ppm] [K]
 *   Sans argument → traitement de toutes les images de démonstration.
 *
 * Sorties dans ./output/ :
 *   <nom>_k<K>_gray.pgm    — segmentation (niveaux de gris uniformes)
 *   <nom>_k<K>_color.ppm   — segmentation (palette de couleurs vives)
 *   <nom>_k<K>_rgb.ppm     — segmentation couleur (images PPM uniquement)
 *
 * Pour chaque image et chaque valeur de K ∈ {2, 3, 4, 6, 8} :
 *   - Initialisation K-Means++
 *   - Convergence jusqu'à epsilon=0.5 ou 100 itérations max
 *   - Affichage des statistiques (centroïdes, tailles, inertie WCSS)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgm_io.h"
#include "kmeans.h"

/* ── Utilitaire : nom de base sans extension ─────────────────────── */
static void basename_noext(const char *path, char *out, int maxlen)
{
    const char *s = strrchr(path, '/');
    s = s ? s + 1 : path;
    strncpy(out, s, (size_t)(maxlen-1));
    out[maxlen-1] = '\0';
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';
}

static int ends_with(const char *s, const char *suf) {
    int ls = (int)strlen(s), lf = (int)strlen(suf);
    return ls >= lf && strcmp(s + ls - lf, suf) == 0;
}

/* ── Traitement d'une image PGM ──────────────────────────────────── */
static void process_pgm(const char *path, int k)
{
    char base[256];
    basename_noext(path, base, sizeof(base));

    PGMImage *img = pgm_load(path);
    if (!img) { fprintf(stderr," Erreur chargement %s\n", path); return; }

    KMeansParams p = KMEANS_DEFAULTS;
    p.k = k;

    KMeansResult *r = kmeans_pgm(img, &p);
    if (!r) { pgm_free(img); return; }

    kmeans_print_result(r, base);

    /* Rendu niveaux de gris */
    char path_gray[512], path_col[512];
    snprintf(path_gray, sizeof(path_gray), "output/%s_k%d_gray.pgm", base, k);
    snprintf(path_col,  sizeof(path_col),  "output/%s_k%d_color.ppm", base, k);

    PGMImage *seg_gray = kmeans_render_gray(img, r);
    if (seg_gray) {
        pgm_save(seg_gray, path_gray);
        printf(" Sauvegardé : %s\n", path_gray);
        pgm_free(seg_gray);
    }

    /* Rendu coloré */
    PPMImage *seg_col = kmeans_render_color(img, r);
    if (seg_col) {
        ppm_save(seg_col, path_col);
        printf(" Sauvegardé : %s\n", path_col);
        ppm_free(seg_col);
    }

    kmeans_free(r);
    pgm_free(img);
}

/* ── Traitement d'une image PPM couleur ──────────────────────────── */
static void process_ppm(const char *path, int k)
{
    char base[256];
    basename_noext(path, base, sizeof(base));

    PPMImage *img = ppm_load(path);
    if (!img) { fprintf(stderr," Erreur chargement %s\n", path); return; }

    KMeansParams p = KMEANS_DEFAULTS;
    p.k = k;

    KMeansResultRGB *r = kmeans_ppm(img, &p);
    if (!r) { ppm_free(img); return; }

    kmeans_print_result_rgb(r, base);

    char path_rgb[512];
    snprintf(path_rgb, sizeof(path_rgb), "output/%s_k%d_rgb.ppm", base, k);

    PPMImage *seg = kmeans_render_rgb(img, r);
    if (seg) {
        ppm_save(seg, path_rgb);
        printf(" Sauvegardé : %s\n", path_rgb);
        ppm_free(seg);
    }

    kmeans_free_rgb(r);
    ppm_free(img);
}


int main(int argc, char *argv[])
{
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║      Segmentation K-Moyennes d'Images (C99)          ║\n");
    printf("║      PGM (niveaux de gris) + PPM (couleur RGB)       ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");

    
    if (argc >= 2) {
        int k = (argc >= 3) ? atoi(argv[2]) : 4;
        if (k < 2) k = 2;
        if (ends_with(argv[1], ".ppm"))
            process_ppm(argv[1], k);
        else
            process_pgm(argv[1], k);
        return EXIT_SUCCESS;
    }

    
    const char *pgm_images[] = {
        "../assets/cameraman.pgm",
        "../assets/bridge.pgm",
        "../assets/clown.pgm",
        "../assets/couple.pgm",
        "../assets/zelda.pgm",
        NULL
    };
    const int K_VALUES[] = {2, 3, 4, 6};
    const int N_K = 4;

    for (int i = 0; pgm_images[i]; i++) {
        printf("\n══════════════════════════════════════════════════════\n");
        printf(" Image PGM : %s\n", pgm_images[i]);
        printf("══════════════════════════════════════════════════════\n");
        for (int ki = 0; ki < N_K; ki++)
            process_pgm(pgm_images[i], K_VALUES[ki]);
    }

    /* Image couleur sailboat avec K ∈ {2,3,4,6} */
    printf("\n══════════════════════════════════════════════════════\n");
    printf(" Image PPM couleur : ../assets/sailboat.ppm\n");
    printf("══════════════════════════════════════════════════════\n");
    for (int ki = 0; ki < N_K; ki++)
        process_ppm("../assets/sailboat.ppm", K_VALUES[ki]);

    printf("\n Traitement terminé. Résultats dans ./output/\n\n");
    return EXIT_SUCCESS;
}

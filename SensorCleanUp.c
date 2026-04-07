/*
 * SensorCleanUp.c
 *
 *  Created on: Mar 31, 2026
 *      Author: alecbroe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_READINGS     200
#define MAX_OBJECTS       20
#define MAX_DIST_THRESH   2.4f   /* distances >= this threshold = "no object" */
#define OUTLIER_MAX_JUMP  0.6f   /* max allowed jump (m) vs both neighbors   */
#define SMOOTH_HALF_WIN   1      /* moving-average ±1 neighbor each side      */

typedef struct { int angle; float distance; } Reading;
typedef struct { int number; float angle; float distance; int width_deg; } Object;

/* ---- file I/O ---- */
int load(const char *fn, Reading r[], int max) {
    FILE *fp = fopen(fn, "r");
    if (!fp) { perror(fn); return -1; }
    char line[128]; int n = 0;
    fgets(line, sizeof(line), fp); /* skip header line */
    while (fgets(line, sizeof(line), fp) && n < max) {
        if (strncmp(line, "END", 3) == 0) break;
        int a; float d;
        if (sscanf(line, "%d %f", &a, &d) == 2)
            { r[n].angle = a; r[n].distance = d; n++; }
    }
    fclose(fp); return n;
}

void save(const char *fn, Reading r[], int n) {
    FILE *fp = fopen(fn, "w");
    if (!fp) { perror(fn); return; }
    fprintf(fp, "Angle(Degrees)\tDistance(m)\n");
    for (int i = 0; i < n; i++)
        fprintf(fp, "%d\t\t\t\t%.2f\n", r[i].angle, r[i].distance);
    fprintf(fp, "END\n");
    fclose(fp);
    printf("Cleaned data saved to '%s'\n", fn);
}

/* ---- Step 1: Outlier rejection ----
 * A reading is an outlier if it differs from BOTH its immediate neighbors
 * by more than OUTLIER_MAX_JUMP meters. Replace it with the neighbor average.
 * This handles single-point spikes common in PING/IR sensors.
 */
void reject_outliers(Reading raw[], Reading out[], int n) {
    for (int i = 0; i < n; i++) out[i] = raw[i];
    for (int i = 1; i < n - 1; i++) {
        float prev = raw[i-1].distance;
        float next = raw[i+1].distance;
        float cur  = raw[i].distance;
        if (fabsf(cur - prev) > OUTLIER_MAX_JUMP &&
            fabsf(cur - next) > OUTLIER_MAX_JUMP)
        {
            out[i].distance = (prev + next) / 2.0f;
        }
    }
}

/* ---- Step 2: Moving-average smoothing (small window) ----
 * Smooths residual noise without blurring object edges too aggressively.
 */
void moving_average(Reading in[], Reading out[], int n, int half_win) {
    for (int i = 0; i < n; i++) {
        int lo = (i - half_win < 0)   ? 0     : i - half_win;
        int hi = (i + half_win >= n)  ? n - 1 : i + half_win;
        float sum = 0.0f; int cnt = 0;
        for (int j = lo; j <= hi; j++) { sum += in[j].distance; cnt++; }
        out[i] = in[i];
        out[i].distance = sum / (float)cnt;
    }
}

/* ---- Object detection ---- */
int find_objects(Reading r[], int n, Object objs[], int max_objs) {
    int cnt = 0, in_obj = 0, start = -1;
    for (int i = 0; i <= n; i++) {
        int active = (i < n) && (r[i].distance < MAX_DIST_THRESH);
        if (active && !in_obj)  { in_obj = 1; start = i; }
        else if (!active && in_obj) {
            in_obj = 0;
            int end = i - 1;
            if (cnt >= max_objs) break;
            float center = (r[start].angle + r[end].angle) / 2.0f;
            float best_dist = r[start].distance;
            float best_diff = 1e9f;
            for (int j = start; j <= end; j++) {
                float diff = fabsf(r[j].angle - center);
                if (diff < best_diff) { best_diff = diff; best_dist = r[j].distance; }
            }
            objs[cnt].number    = cnt + 1;
            objs[cnt].angle     = center;
            objs[cnt].distance  = best_dist;
            objs[cnt].width_deg = r[end].angle - r[start].angle;
            cnt++;
        }
    }
    return cnt;
}

void print_objects(Object objs[], int n) {
    printf("\n%-10s %-12s %-14s %-10s\n",
           "Object#", "Angle(deg)", "Distance(m)", "Width(deg)");
    printf("%-10s %-12s %-14s %-10s\n",
           "-------", "----------", "-----------", "----------");
    for (int i = 0; i < n; i++)
        printf("%-10d %-12.1f %-14.2f %-10d\n",
               objs[i].number, objs[i].angle,
               objs[i].distance, objs[i].width_deg);
    printf("\nTotal objects: %d\n", n);
}

int main(int argc, char *argv[]) {
    const char *in_file  = (argc > 1) ? argv[1] : "mock-cybot-sensor-scan.txt";
    const char *out_file = (argc > 2) ? argv[2] : "cleaned-sensor-scan.txt";

    Reading raw[MAX_READINGS], step1[MAX_READINGS], cleaned[MAX_READINGS];
    Object  objects[MAX_OBJECTS];

    int n = load(in_file, raw, MAX_READINGS);
    if (n <= 0) { fprintf(stderr, "No data loaded.\n"); return 1; }
    printf("Loaded %d readings from '%s'\n\n", n, in_file);

    /* Print raw data summary */
    printf("--- Raw Data Object Summary ---");
    Object raw_objs[MAX_OBJECTS];
    int raw_cnt = find_objects(raw, n, raw_objs, MAX_OBJECTS);
    print_objects(raw_objs, raw_cnt);

    /* Apply cleanup pipeline */
    reject_outliers(raw, step1, n);
    moving_average(step1, cleaned, n, SMOOTH_HALF_WIN);

    save(out_file, cleaned, n);

    /* Print cleaned data summary */
    printf("\n--- Cleaned Data Object Summary ---");
    int obj_cnt = find_objects(cleaned, n, objects, MAX_OBJECTS);
    print_objects(objects, obj_cnt);

    return 0;
}

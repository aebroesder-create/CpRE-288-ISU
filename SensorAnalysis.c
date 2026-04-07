/*
 * SensorAnalysis.c
 *
 *  Created on: Mar 31, 2026
 *      Author: alecbroe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_READINGS   200
#define MAX_OBJECTS     20
#define MAX_DIST_THRESH 2.4   /* distances >= this are treated as "no object" */

typedef struct {
    int   angle;
    float distance;
} SensorReading;

typedef struct {
    int   number;
    float angle;       /* center angle in degrees */
    float distance;    /* distance at center angle */
    int   width_deg;   /* angular width in degrees */
} Object;

/* Load readings from file, skipping header and "END" lines */
int load_readings(const char *filename, SensorReading readings[], int max) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { perror("Cannot open file"); return -1; }

    char line[128];
    int count = 0;

    /* Skip header line */
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp) && count < max) {
        if (strncmp(line, "END", 3) == 0) break;
        int angle; float dist;
        if (sscanf(line, "%d %f", &angle, &dist) == 2) {
            readings[count].angle    = angle;
            readings[count].distance = dist;
            count++;
        }
    }
    fclose(fp);
    return count;
}

/* Find objects in the readings array */
int find_objects(SensorReading readings[], int n_readings,
                 Object objects[], int max_objects)
{
    int obj_count = 0;
    int in_object = 0;
    int obj_start_idx = -1;

    for (int i = 0; i <= n_readings; i++) {
        int is_object_reading = (i < n_readings) &&
                                (readings[i].distance < MAX_DIST_THRESH);

        if (is_object_reading && !in_object) {
            /* Start of a new object */
            in_object     = 1;
            obj_start_idx = i;

        } else if (!is_object_reading && in_object) {
            /* End of current object */
            in_object = 0;
            int obj_end_idx = i - 1;

            if (obj_count >= max_objects) break;

            int start_angle = readings[obj_start_idx].angle;
            int end_angle   = readings[obj_end_idx].angle;
            int width       = end_angle - start_angle;
            float center    = (start_angle + end_angle) / 2.0f;

            /* Find reading closest to center angle */
            float min_diff = 1e9f;
            float center_dist = readings[obj_start_idx].distance;
            for (int j = obj_start_idx; j <= obj_end_idx; j++) {
                float diff = fabsf(readings[j].angle - center);
                if (diff < min_diff) {
                    min_diff     = diff;
                    center_dist  = readings[j].distance;
                }
            }

            objects[obj_count].number    = obj_count + 1;
            objects[obj_count].angle     = center;
            objects[obj_count].distance  = center_dist;
            objects[obj_count].width_deg = width;
            obj_count++;
        }
    }
    return obj_count;
}

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "mock-cybot-sensor-scan.txt";

    SensorReading readings[MAX_READINGS];
    Object        objects[MAX_OBJECTS];

    int n = load_readings(filename, readings, MAX_READINGS);
    if (n <= 0) { fprintf(stderr, "No data loaded.\n"); return 1; }

    printf("Loaded %d readings from '%s'\n\n", n, filename);

    int obj_count = find_objects(readings, n, objects, MAX_OBJECTS);

    printf("%-10s %-12s %-14s %-10s\n",
           "Object#", "Angle(deg)", "Distance(m)", "Width(deg)");
    printf("%-10s %-12s %-14s %-10s\n",
           "-------", "----------", "-----------", "----------");

    for (int i = 0; i < obj_count; i++) {
        printf("%-10d %-12.1f %-14.2f %-10d\n",
               objects[i].number,
               objects[i].angle,
               objects[i].distance,
               objects[i].width_deg);
    }

    printf("\nTotal objects detected: %d\n", obj_count);
    return 0;
}

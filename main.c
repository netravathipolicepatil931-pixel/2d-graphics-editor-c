#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
 
#define ROWS 30
#define COLS 80
#define MAX_OBJECTS 50
 
/* ── Canvas ─────────────────────────────────────────────── */
char canvas[ROWS][COLS];
 
/* ── Object types ───────────────────────────────────────── */
typedef enum { CIRCLE = 1, RECTANGLE, LINE, TRIANGLE } ShapeType;
 
typedef struct {
    int x, y;          /* centre / top-left / start point  */
    int x2, y2;        /* end / bottom-right / second point */
    int x3, y3;        /* third vertex (triangle only)      */
    int radius;        /* circle only                       */
    ShapeType type;
    int active;        /* 1 = in use, 0 = deleted           */
} Object;
 
Object objects[MAX_OBJECTS];
int object_count = 0;
 
/* ── Canvas helpers ─────────────────────────────────────── */
void init_canvas(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            canvas[r][c] = '_';
}
 
void display_canvas(void) {
    printf("\n");
    /* column ruler */
    printf("   ");
    for (int c = 0; c < COLS; c += 10) printf("%-10d", c);
    printf("\n   ");
    for (int c = 0; c < COLS; c++) printf("%d", c % 10);
    printf("\n");
 
    for (int r = 0; r < ROWS; r++) {
        printf("%2d|", r);
        for (int c = 0; c < COLS; c++) putchar(canvas[r][c]);
        printf("|\n");
    }
    printf("\n");
}
 
static inline void plot(int r, int c) {
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = '*';
}
 
static inline void erase(int r, int c) {
    if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
        canvas[r][c] = '_';
}
 
/* ── Drawing primitives ─────────────────────────────────── */
 
/* Bresenham line */
void draw_line_coords(int r1, int c1, int r2, int c2, void (*fn)(int,int)) {
    int dr = abs(r2 - r1), dc = abs(c2 - c1);
    int sr = (r1 < r2) ? 1 : -1;
    int sc = (c1 < c2) ? 1 : -1;
    int err = dr - dc, e2;
 
    while (1) {
        fn(r1, c1);
        if (r1 == r2 && c1 == c2) break;
        e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r1 += sr; }
        if (e2 <  dr) { err += dr; c1 += sc; }
    }
}
 
/* Midpoint circle */
void draw_circle_coords(int cr, int cc, int rad, void (*fn)(int,int)) {
    int x = 0, y = rad, d = 1 - rad;
    while (x <= y) {
        fn(cr+y, cc+x); fn(cr-y, cc+x);
        fn(cr+y, cc-x); fn(cr-y, cc-x);
        fn(cr+x, cc+y); fn(cr-x, cc+y);
        fn(cr+x, cc-y); fn(cr-x, cc-y);
        if (d < 0) d += 2*x + 3;
        else { d += 2*(x-y) + 5; y--; }
        x++;
    }
}
 
void draw_rect_coords(int r1, int c1, int r2, int c2, void (*fn)(int,int)) {
    for (int c = c1; c <= c2; c++) { fn(r1,c); fn(r2,c); }
    for (int r = r1; r <= r2; r++) { fn(r,c1); fn(r,c2); }
}
 
void draw_triangle_coords(int r1,int c1, int r2,int c2, int r3,int c3,
                          void (*fn)(int,int)) {
    draw_line_coords(r1,c1, r2,c2, fn);
    draw_line_coords(r2,c2, r3,c3, fn);
    draw_line_coords(r3,c3, r1,c1, fn);
}
 
/* ── Render a single object ─────────────────────────────── */
void render_object(const Object *o, void (*fn)(int,int)) {
    switch (o->type) {
        case CIRCLE:
            draw_circle_coords(o->y, o->x, o->radius, fn); break;
        case RECTANGLE:
            draw_rect_coords(o->y, o->x, o->y2, o->x2, fn); break;
        case LINE:
            draw_line_coords(o->y, o->x, o->y2, o->x2, fn); break;
        case TRIANGLE:
            draw_triangle_coords(o->y,o->x, o->y2,o->x2, o->y3,o->x3, fn); break;
    }
}
 
/* Redraw entire canvas from scratch */
void redraw_all(void) {
    init_canvas();
    for (int i = 0; i < object_count; i++)
        if (objects[i].active)
            render_object(&objects[i], plot);
}
 
/* ── Object list display ────────────────────────────────── */
void list_objects(void) {
    printf("\n%-4s %-12s %s\n", "ID", "Type", "Parameters");
    printf("--------------------------------------------------\n");
    int found = 0;
    for (int i = 0; i < object_count; i++) {
        if (!objects[i].active) continue;
        found = 1;
        const char *name[] = {"","Circle","Rectangle","Line","Triangle"};
        printf("[%2d] %-12s ", i+1, name[objects[i].type]);
        switch (objects[i].type) {
            case CIRCLE:
                printf("centre(%d,%d) radius=%d", objects[i].x, objects[i].y, objects[i].radius);
                break;
            case RECTANGLE:
                printf("(%d,%d) -> (%d,%d)", objects[i].x, objects[i].y, objects[i].x2, objects[i].y2);
                break;
            case LINE:
                printf("(%d,%d) -> (%d,%d)", objects[i].x, objects[i].y, objects[i].x2, objects[i].y2);
                break;
            case TRIANGLE:
                printf("(%d,%d) (%d,%d) (%d,%d)",
                    objects[i].x, objects[i].y,
                    objects[i].x2, objects[i].y2,
                    objects[i].x3, objects[i].y3);
                break;
        }
        printf("\n");
    }
    if (!found) printf("  (no objects)\n");
    printf("\n");
}
 
/* ── Input helpers ──────────────────────────────────────── */
int get_int(const char *prompt) {
    int v; printf("%s", prompt); scanf("%d", &v); return v;
}
 
int select_object(void) {
    list_objects();
    int id = get_int("Enter object ID (0 to cancel): ");
    if (id <= 0 || id > object_count || !objects[id-1].active) {
        printf("Invalid ID.\n"); return -1;
    }
    return id - 1;
}
 
/* ── Add operations ─────────────────────────────────────── */
void add_circle(void) {
    if (object_count >= MAX_OBJECTS) { printf("Max objects reached.\n"); return; }
    Object o = {0}; o.type = CIRCLE; o.active = 1;
    o.x = get_int("  Centre col (x): ");
    o.y = get_int("  Centre row (y): ");
    o.radius = get_int("  Radius       : ");
    objects[object_count++] = o;
    render_object(&o, plot);
    printf("Circle added (ID %d).\n", object_count);
}
 
void add_rectangle(void) {
    if (object_count >= MAX_OBJECTS) { printf("Max objects reached.\n"); return; }
    Object o = {0}; o.type = RECTANGLE; o.active = 1;
    o.x  = get_int("  Top-left  col (x1): ");
    o.y  = get_int("  Top-left  row (y1): ");
    o.x2 = get_int("  Bot-right col (x2): ");
    o.y2 = get_int("  Bot-right row (y2): ");
    objects[object_count++] = o;
    render_object(&o, plot);
    printf("Rectangle added (ID %d).\n", object_count);
}
 
void add_line(void) {
    if (object_count >= MAX_OBJECTS) { printf("Max objects reached.\n"); return; }
    Object o = {0}; o.type = LINE; o.active = 1;
    o.x  = get_int("  Start col (x1): ");
    o.y  = get_int("  Start row (y1): ");
    o.x2 = get_int("  End   col (x2): ");
    o.y2 = get_int("  End   row (y2): ");
    objects[object_count++] = o;
    render_object(&o, plot);
    printf("Line added (ID %d).\n", object_count);
}
 
void add_triangle(void) {
    if (object_count >= MAX_OBJECTS) { printf("Max objects reached.\n"); return; }
    Object o = {0}; o.type = TRIANGLE; o.active = 1;
    printf("  Vertex 1:\n");
    o.x  = get_int("    col (x1): "); o.y  = get_int("    row (y1): ");
    printf("  Vertex 2:\n");
    o.x2 = get_int("    col (x2): "); o.y2 = get_int("    row (y2): ");
    printf("  Vertex 3:\n");
    o.x3 = get_int("    col (x3): "); o.y3 = get_int("    row (y3): ");
    objects[object_count++] = o;
    render_object(&o, plot);
    printf("Triangle added (ID %d).\n", object_count);
}
 
/* ── Delete ─────────────────────────────────────────────── */
void delete_object(void) {
    int idx = select_object();
    if (idx < 0) return;
    objects[idx].active = 0;
    redraw_all();
    printf("Object %d deleted.\n", idx+1);
}
 
/* ── Modify ─────────────────────────────────────────────── */
void modify_object(void) {
    int idx = select_object();
    if (idx < 0) return;
 
    Object *o = &objects[idx];
    printf("Re-enter parameters for object %d:\n", idx+1);
 
    switch (o->type) {
        case CIRCLE:
            o->x      = get_int("  New centre col (x)  : ");
            o->y      = get_int("  New centre row (y)  : ");
            o->radius = get_int("  New radius          : ");
            break;
        case RECTANGLE:
            o->x  = get_int("  New top-left  col (x1): ");
            o->y  = get_int("  New top-left  row (y1): ");
            o->x2 = get_int("  New bot-right col (x2): ");
            o->y2 = get_int("  New bot-right row (y2): ");
            break;
        case LINE:
            o->x  = get_int("  New start col (x1): ");
            o->y  = get_int("  New start row (y1): ");
            o->x2 = get_int("  New end   col (x2): ");
            o->y2 = get_int("  New end   row (y2): ");
            break;
        case TRIANGLE:
            printf("  Vertex 1:\n");
            o->x  = get_int("    col (x1): "); o->y  = get_int("    row (y1): ");
            printf("  Vertex 2:\n");
            o->x2 = get_int("    col (x2): "); o->y2 = get_int("    row (y2): ");
            printf("  Vertex 3:\n");
            o->x3 = get_int("    col (x3): "); o->y3 = get_int("    row (y3): ");
            break;
    }
    redraw_all();
    printf("Object %d modified.\n", idx+1);
}
 
/* ── Clear canvas ───────────────────────────────────────── */
void clear_all(void) {
    object_count = 0;
    init_canvas();
    printf("Canvas cleared.\n");
}
 
/* ── Menus ──────────────────────────────────────────────── */
void add_menu(void) {
    printf("\n--- Add Object ---\n");
    printf("  1. Circle\n");
    printf("  2. Rectangle\n");
    printf("  3. Line\n");
    printf("  4. Triangle\n");
    printf("  0. Back\n");
    int ch = get_int("Choice: ");
    switch (ch) {
        case 1: add_circle();    break;
        case 2: add_rectangle(); break;
        case 3: add_line();      break;
        case 4: add_triangle();  break;
        case 0: break;
        default: printf("Invalid choice.\n");
    }
}
 
void main_menu(void) {
    int ch;
    do {
        printf("\n========================================\n");
        printf("       2D GRAPHICS EDITOR  (C)\n");
        printf("========================================\n");
        printf("  1. Display Canvas\n");
        printf("  2. Add Object\n");
        printf("  3. Delete Object\n");
        printf("  4. Modify Object\n");
        printf("  5. List Objects\n");
        printf("  6. Clear Canvas\n");
        printf("  0. Exit\n");
        printf("----------------------------------------\n");
        ch = get_int("Choice: ");
        switch (ch) {
            case 1: display_canvas();  break;
            case 2: add_menu();        break;
            case 3: delete_object();   break;
            case 4: modify_object();   break;
            case 5: list_objects();    break;
            case 6: clear_all();       break;
            case 0: printf("Goodbye!\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (ch != 0);
}
 
/* ── Entry point ────────────────────────────────────────── */
int main(void) {
    init_canvas();
    main_menu();
     return 0;
}
 
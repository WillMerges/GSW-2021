// https://codereview.stackexchange.com/questions/263470/ncurses-listview-control

#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <stdint.h>
#include <stdlib.h>
#include <ncurses.h>

typedef enum lv_color {
    black = COLOR_BLACK,
    blue = COLOR_BLUE,
    cyan = COLOR_CYAN,
    green = COLOR_GREEN,
    magenta = COLOR_MAGENTA,
    red = COLOR_RED,
    white = COLOR_WHITE,
    yellow = COLOR_YELLOW,
} lv_color;

typedef enum column_alignment {

    // Aligns the column text and the cells in that column to the left
    left,

    // Aligns the column text and the cells in that column centered
    center,

    // Aligns the column text and the cells in that column to the right
    right

} column_alignment;

typedef struct lv_cell {

    // The text in the cell
    char *text;

    short bg_color;

    short fg_color;

} lv_cell;

typedef struct lv_column {

    // The column text
    char *text;

    // The width of the column (depends on width of the row cells)
    uint16_t width;

    // Specifies the alignment of the text in entire column
    column_alignment alignment;

} lv_column;

typedef struct lv_row {

    // Holds all cells in the row
    lv_cell *cells;

    // Total amount of cells in the row
    uint8_t cell_count;

} lv_row;

typedef struct listview {

    // Holds all columns
    lv_column *columns;

    // Holds amount of columns
    uint16_t column_count;

    // Holds all rows
    lv_row *rows;

    // Holds amount of rows
    uint16_t rows_count;

    // Maximum window width the listview can occupy
    uint16_t listview_max_width;

    // Maximum window height the listview can occupy
    uint16_t listview_max_height;

    // Amount of spaces padded to the left and right of each cell
    uint8_t cell_padding;

} listview;


listview *listview_new();

void listview_free(listview *lv);

void listview_add_column(listview *lv, char* text);

int listview_set_column_alignment(listview *lv, int column_index, column_alignment alignment);

int listview_insert_row(listview* lv, int row_index, ...);

int listview_delete_column(listview *lv, int column_index);

void listview_add_row(listview *lv, ...);

int listview_delete_row(listview *lv, int row_index);

int listview_update_cell_text(listview *lv, int row_index, int cell_index, char* text);

int listview_update_cell_background_color(listview* lv, int row_index, int cell_index, lv_color bg_color);

int listview_update_cell_foreground_color(listview* lv, int row_index, int cell_index, lv_color fg_color);

void listview_render(WINDOW *win, listview *lv);

void update_max_col_width(listview *lv, int row_index);

#endif

// https://codereview.stackexchange.com/questions/263470/ncurses-listview-control

#include "listview.h"
#include <ncurses.h>
#include <curses.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

listview *listview_new() {
    listview *lv = (listview*)malloc(sizeof(listview));
    if (!lv) { return NULL; }

    lv->rows = NULL;
    lv->rows_count = 0;
    lv->columns = NULL;
    lv->column_count = 0;
    lv->listview_max_width = 0;
    lv->listview_max_height = 0;
    lv->cell_padding = 2;

    return lv;
}

void listview_add_column(listview *lv, char *text) {
    lv->columns = (lv_column*)realloc(lv->columns, (lv->column_count + 1) * sizeof(lv_column));
    if (!lv->columns) { return; }

    lv->columns[lv->column_count].text = text;
    lv->columns[lv->column_count].alignment = left;
    lv->columns[lv->column_count].width = (2 * lv->cell_padding) + strlen(text);

    lv->column_count++;
}

int listview_set_column_alignment(listview *lv, int column_index, column_alignment alignment) {
    if (column_index >= lv->column_count)
        return -1;

    lv->columns[column_index].alignment = alignment;
    return 0;
}

void listview_render(WINDOW *win, listview *lv) {
    // Define colors
    wclear(win);
    init_pair(10, COLOR_BLACK, COLOR_WHITE);
    init_pair(20, COLOR_WHITE, COLOR_BLACK);
    getmaxyx(win, lv->listview_max_height, lv->listview_max_width);
    wattr_on(win, COLOR_PAIR(10), NULL);

    // Render columns first
    int cursor_pos = 0;
    for (int i = 0; i < lv->column_count; i++) {
        char *col_text = lv->columns[i].text;
        int col_width = lv->columns[i].width;
        int col_fill = col_width - (2 * lv->cell_padding) - strlen(col_text);
        int extra_pad = lv->listview_max_width - cursor_pos - (lv->cell_padding + strlen(col_text) + lv->cell_padding);

        if (i == lv->column_count - 1) {
            mvwprintw(win, 0, cursor_pos, "%*s%s%*s%*s", lv->cell_padding, "", col_text, lv->cell_padding, "",
                      extra_pad, "");
            cursor_pos = 0;
            break;
        } else {
            if (lv->columns[i].alignment == left) {
                mvwprintw(win, 0, cursor_pos, "%*s%s%*s%*s", lv->cell_padding, "", col_text, lv->cell_padding, "",
                          col_fill,
                          "");
            } else if (lv->columns[i].alignment == right) {
                mvwprintw(win, 0, cursor_pos, "%*s%*s%s%*s", lv->cell_padding, "", col_fill, "", col_text,
                          lv->cell_padding, "");
            } else if (lv->columns[i].alignment == center) {
                int col_fill_left = col_fill / 2;
                int coll_fill_right = col_fill - col_fill_left;
                mvwprintw(win, 0, cursor_pos, "%*s%*s%s%*s%*s", lv->cell_padding, "", col_fill_left, "", col_text,
                          coll_fill_right, "", lv->cell_padding, "");
            }
        }
        cursor_pos += lv->cell_padding + strlen(col_text) + lv->cell_padding + col_fill;
    }
    wattr_on(win, COLOR_PAIR(20), NULL);

    // Render rows
    for (int i = 0; i < lv->rows_count; i++) {
        for (int j = 0; j < lv->rows[i].cell_count; j++) {
            char *cell_text = lv->rows[i].cells[j].text;
            int col_width = lv->columns[j].width;
            int col_fill = col_width - (2 * lv->cell_padding) - strlen(cell_text);
            int extra_pad =
                    lv->listview_max_width - cursor_pos - (lv->cell_padding + strlen(cell_text) + lv->cell_padding);

            init_pair(30, lv->rows[i].cells[j].fg_color, lv->rows[i].cells[j].bg_color);
            wattr_on(win, COLOR_PAIR(30), NULL);
            if (j == lv->rows[i].cell_count - 1) {
                mvwprintw(win, i + 1, cursor_pos, "%*s%s%*s%*s", lv->cell_padding, "", cell_text, lv->cell_padding,
                          "", extra_pad, "");
                cursor_pos = 0;
                break;
            } else {
                if (lv->columns[j].alignment == left) {
                    mvwprintw(win, i + 1, cursor_pos, "%*s%s%*s%*s", lv->cell_padding, "", cell_text, lv->cell_padding,
                              "", col_fill, "");
                } else if (lv->columns[j].alignment == right) {
                    mvwprintw(win, i + 1, cursor_pos, "%*s%*s%s%*s", lv->cell_padding, "", col_fill, "", cell_text,
                              lv->cell_padding, "");
                } else if (lv->columns[j].alignment == center) {
                    int col_fill_left = col_fill / 2;
                    int coll_fill_right = col_fill - col_fill_left;
                    mvwprintw(win, i + 1, cursor_pos, "%*s%*s%s%*s%*s", lv->cell_padding, "", col_fill_left, "",
                              cell_text, coll_fill_right, "", lv->cell_padding, "");
                }
            }
            // free_pair(color_id);
            wattr_off(win, COLOR_PAIR(30), NULL);
            cursor_pos += lv->cell_padding + strlen(cell_text) + lv->cell_padding + col_fill;
        }
    }
}

int listview_delete_column(listview *lv, int column_index)
{
    if(column_index >= lv->column_count)
        return -1;

    // Remove column
    lv_column *new_cols = (lv_column*)malloc(sizeof(lv_column) * (lv->column_count - 1));
    memmove(new_cols, lv->columns, column_index * sizeof(lv_column));
    memmove(&new_cols[column_index], &lv->columns[column_index + 1], (lv->column_count - column_index - 1) * sizeof(lv_column));
    free(lv->columns);
    lv->columns = new_cols;

    // Remove effected cells in every row
    for (int i = 0; i < lv->rows_count; i++)
    {
        lv_cell *new_cells = (lv_cell*)malloc(sizeof(lv_cell) * (lv->column_count - 1));
        memmove(new_cells, lv->rows[i].cells, column_index * sizeof(lv_cell));
        memmove(&new_cells[column_index], &lv->rows[i].cells[column_index + 1], (lv->column_count - column_index - 1) * sizeof(lv_cell));

        free(lv->rows[i].cells);
        lv->rows[i].cells = new_cells;
        lv->rows[i].cell_count--;
    }

    lv->column_count--;

    return 0;
}

int listview_update_cell_background_color(listview *lv, int row_index, int cell_index, lv_color bg_color)
{
    if (bg_color < 0 || bg_color > 7)
        return -1;
    lv->rows[row_index].cells[cell_index].bg_color = bg_color;
    return 0;
}

int listview_update_cell_foreground_color(listview *lv, int row_index, int cell_index, lv_color fg_color)
{
    if (fg_color < 0 || fg_color > 7)
        return -1;
    lv->rows[row_index].cells[cell_index].fg_color = fg_color;
    return 0;
}

void listview_add_row(listview *lv, ...) {
    va_list argp;
    va_start(argp, lv);

    lv->rows = (lv_row*)realloc(lv->rows, sizeof(lv_row) * (lv->rows_count + 1));
    lv->rows[lv->rows_count].cells = (lv_cell*)calloc(lv->column_count, sizeof(lv_cell));
    lv->rows[lv->rows_count].cell_count = lv->column_count;

    for (int i = 0; i < lv->column_count; i++) {
        char *text = va_arg(argp,
        char*);
        lv->rows[lv->rows_count].cells[i].text = text;
        lv->rows[lv->rows_count].cells[i].bg_color = black;
        lv->rows[lv->rows_count].cells[i].fg_color = white;
    }

    lv->rows_count++;
    va_end(argp);

    update_max_col_width(lv, lv->rows_count);
}

int listview_delete_row(listview *lv, int row_index) {
    if (row_index > lv->rows_count)
        return - 1;

    lv_row *new_rows = (lv_row*)malloc(sizeof(lv_row) * (lv->rows_count - 1));
    if (new_rows == NULL)
        return -2;

    memmove(new_rows, lv->rows, row_index * sizeof(lv_row));
    memmove(&new_rows[row_index], &lv->rows[row_index + 1], (lv->rows_count - row_index - 1) * sizeof(lv_row));

    free(lv->rows);
    lv->rows = new_rows;
    lv->rows_count--;

    update_max_col_width(lv, lv->rows_count);
    return 0;
}

int listview_insert_row(listview *lv, int row_index, ...) {
    if (row_index > lv->rows_count)
        return -1;

    va_list argp;
    va_start(argp, row_index);

    lv->rows = (lv_row*)realloc(lv->rows, sizeof(lv_row) * (lv->rows_count + 1));
    if(!lv->rows)
        return -2;

    if (row_index < lv->rows_count) {
        int entries_to_move = (lv->rows_count - row_index);
        memmove(&lv->rows[row_index + 1], &lv->rows[row_index], entries_to_move * sizeof(lv_row));
        lv->rows[row_index].cells = (lv_cell *) malloc(sizeof(lv_cell) * lv->column_count);
        for (int i = 0; i < lv->column_count; i++) {
            lv->rows[row_index].cell_count = lv->column_count;
            lv->rows[row_index].cells[i].text = va_arg(argp, char*);
            lv->rows[row_index].cells[i].fg_color = white;
            lv->rows[row_index].cells[i].bg_color = black;
        }
    } else if (row_index == lv->rows_count) {
        lv->rows[row_index].cells = (lv_cell *) malloc(sizeof(lv_cell) * lv->column_count);
        for (int i = 0; i < lv->column_count; i++) {
            lv->rows[row_index].cell_count = lv->column_count;
            lv->rows[row_index].cells[i].text = va_arg(argp, char*);
            lv->rows[row_index].cells[i].fg_color = white;
            lv->rows[row_index].cells[i].bg_color = black;
        }
    }

    lv->rows_count++;
    update_max_col_width(lv, lv->rows_count);

    return 0;
}

int listview_update_cell_text(listview *lv, int row_index, int cell_index, char *text) {
    if (row_index >= lv->rows_count || cell_index >= lv->column_count)
        return -1;

    if(text == NULL)
        return -2;

    lv->rows[row_index].cells[cell_index].text = text;
    update_max_col_width(lv, lv->rows_count);
    return 0;
}

void update_max_col_width(listview *lv, int row_index) {

    // Make sure index is valid
    if (row_index > lv->rows_count || row_index < 0)
        return;

    // Reset all max col width values
    for (int i = 0; i < lv->column_count; i++) {
        lv->columns[i].width = strlen(lv->columns[i].text) + (2 * lv->cell_padding);
    }
    // Recompute the max col width values for rows
    for (int i = 0; i < lv->rows_count; i++) {
        for (int j = 0; j < lv->rows[i].cell_count; j++) {
            if (strlen(lv->rows[i].cells[j].text) + (2 * lv->cell_padding) > lv->columns[j].width)
                lv->columns[j].width = strlen(lv->rows[i].cells[j].text) + (2 * lv->cell_padding);
        }
    }
}

void listview_free(listview *lv) {

    // Free rows
    free(lv->rows);

    // Free headers
    free(lv->columns);

    // Free lv itself
    free(lv);
}

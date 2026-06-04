#include <ctype.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define GRID_SIZE 9

// Color pair indices
#define COLOR_USER    1  // Blue: user-entered
#define COLOR_SOLVED  2  // White: solver-filled
#define COLOR_INVALID 3  // Yellow: invalid/duplicate

// Function declarations
void draw_grid(void);
void handle_input(int ch);
bool is_valid(int row, int col, int num, int grid_to_check[GRID_SIZE][GRID_SIZE]);
bool is_valid_full_grid(int grid_to_check[GRID_SIZE][GRID_SIZE]);
bool solve(int grid_to_solve[GRID_SIZE][GRID_SIZE]);
void solve_sudoku(void);
void clear_grid(void);
int count_filled(void);
int cursor_y(void);
void clear_solution_cells(void);

int grid[GRID_SIZE][GRID_SIZE] = {0};
int user_grid[GRID_SIZE][GRID_SIZE] = {0};
int cursor_row = 0, cursor_col = 0;
bool solved = false;

// Maps cursor_row to the screen row accounting for box dividers
int cursor_y(void) {
    int y = cursor_row + 4; // +3 for title, +1 base offset
    if (cursor_row > 2) y++;
    if (cursor_row > 5) y++;
    return y;
}

void draw_grid(void) {
    clear();
    printw("+-----------------------+\n|     SUDOKU SOLVER     |\n+-----------------------+\n");
    for (int i = 0; i < GRID_SIZE; i++) {
        if (i % 3 == 0) printw("+-------+-------+-------+\n");
        for (int j = 0; j < GRID_SIZE; j++) {
            if (j % 3 == 0) printw("| ");
            if (grid[i][j] != 0) {
                int pair = (user_grid[i][j] == 2) ? COLOR_INVALID
                         : (user_grid[i][j] == 1) ? COLOR_USER
                         : COLOR_SOLVED;
                attron(COLOR_PAIR(pair));
                printw("%d ", grid[i][j]);
                attroff(COLOR_PAIR(pair));
            } else {
                printw("  ");
            }
        }
        printw("|\n");
    }
    printw("+-------+-------+-------+\n");
    printw("\n[S]olve / [C]lear / [Q]uit\n");
    move(cursor_y(), cursor_col * 2 + (cursor_col / 3) * 2 + 2);
}

bool is_valid(int row, int col, int num, int grid_to_check[GRID_SIZE][GRID_SIZE]) {
    for (int x = 0; x < GRID_SIZE; x++)
        if (grid_to_check[row][x] == num || grid_to_check[x][col] == num) return false;

    int box_row_start = row - row % 3;
    int box_col_start = col - col % 3;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (grid_to_check[box_row_start + i][box_col_start + j] == num) return false;

    return true;
}

// Resets stale invalid marks, then re-scans for duplicates.
// Returns true if no duplicates found.
bool is_valid_full_grid(int grid_to_check[GRID_SIZE][GRID_SIZE]) {
    // Clear stale invalid marks (leave user-entered as 1, solver-filled as 0)
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            if (user_grid[i][j] == 2) user_grid[i][j] = 1;

    bool valid = true;
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (grid_to_check[row][col] == 0) continue;
            int val = grid_to_check[row][col];

            for (int i = 0; i < GRID_SIZE; i++) {
                if (i != col && grid_to_check[row][i] == val) {
                    user_grid[row][i] = 2;
                    user_grid[row][col] = 2;
                    valid = false;
                }
                if (i != row && grid_to_check[i][col] == val) {
                    user_grid[i][col] = 2;
                    user_grid[row][col] = 2;
                    valid = false;
                }
            }
            int brs = row - row % 3, bcs = col - col % 3;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    int r = brs + i, c = bcs + j;
                    if ((r != row || c != col) && grid_to_check[r][c] == val) {
                        user_grid[r][c] = 2;
                        user_grid[row][col] = 2;
                        valid = false;
                    }
                }
            }
        }
    }
    return valid;
}

bool solve(int grid_to_solve[GRID_SIZE][GRID_SIZE]) {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (grid_to_solve[row][col] == 0) {
                for (int num = 1; num <= GRID_SIZE; num++) {
                    if (is_valid(row, col, num, grid_to_solve)) {
                        grid_to_solve[row][col] = num;
                        if (solve(grid_to_solve)) return true;
                        grid_to_solve[row][col] = 0;
                    }
                }
                return false;
            }
        }
    }
    return true;
}

// Clears solver-filled cells (user_grid == 0) after editing a solved grid
void clear_solution_cells(void) {
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            if (user_grid[i][j] == 0) grid[i][j] = 0;
    solved = false;
}

// Advances cursor to next cell, wrapping at row end but not past last cell
void advance_cursor(void) {
    if (cursor_col == GRID_SIZE - 1) {
        cursor_col = 0;
        if (cursor_row < GRID_SIZE - 1) cursor_row++;
    } else {
        cursor_col++;
    }
}

void handle_input(int ch) {
    bool needs_redraw = true;

    if (isdigit(ch) && ch != '0') {
        int new_num = ch - '0';
        bool is_duplicate = false;

        if (!solved) {
            for (int i = 0; i < GRID_SIZE; i++) {
                if (i != cursor_col && grid[cursor_row][i] == new_num && user_grid[cursor_row][i] == 1) { is_duplicate = true; break; }
                if (i != cursor_row && grid[i][cursor_col] == new_num && user_grid[i][cursor_col] == 1) { is_duplicate = true; break; }
            }
            if (!is_duplicate) {
                int brs = cursor_row - cursor_row % 3, bcs = cursor_col - cursor_col % 3;
                for (int i = 0; i < 3 && !is_duplicate; i++)
                    for (int j = 0; j < 3 && !is_duplicate; j++) {
                        int r = brs + i, c = bcs + j;
                        if ((r != cursor_row || c != cursor_col) && grid[r][c] == new_num && user_grid[r][c] == 1)
                            is_duplicate = true;
                    }
            }
        }

        if (is_duplicate) {
            beep();
        } else {
            if (solved) clear_solution_cells();
            grid[cursor_row][cursor_col] = new_num;
            user_grid[cursor_row][cursor_col] = 1;
            advance_cursor();
        }
    } else if (ch == '0' || ch == '.' || ch == '-' || ch == '*' || ch == ' ') {
        if (solved) clear_solution_cells();
        grid[cursor_row][cursor_col] = 0;
        user_grid[cursor_row][cursor_col] = 0;
        advance_cursor();
    } else if (ch == KEY_RIGHT || ch == '\t' || ch == 10) {
        advance_cursor();
    } else if (ch == KEY_LEFT || ch == KEY_BTAB) {
        if (cursor_col == 0) {
            cursor_col = GRID_SIZE - 1;
            if (cursor_row > 0) cursor_row--;
        } else {
            cursor_col--;
        }
    } else if (ch == KEY_UP) {
        cursor_row = (cursor_row - 1 + GRID_SIZE) % GRID_SIZE;
    } else if (ch == KEY_DOWN) {
        cursor_row = (cursor_row + 1) % GRID_SIZE;
    } else if (ch == KEY_BACKSPACE || ch == 127) {
        // Move back, but don't wrap past (0,0)
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = GRID_SIZE - 1;
        }
        grid[cursor_row][cursor_col] = 0;
        user_grid[cursor_row][cursor_col] = 0;
    } else if (ch == 's' || ch == 'S') {
        needs_redraw = false;
        if (count_filled() == 0) {
            draw_grid();
            attron(COLOR_PAIR(COLOR_INVALID));
            mvprintw(GRID_SIZE * 2 + 2, 0, "Nothing to solve!");
            attroff(COLOR_PAIR(COLOR_INVALID));
            refresh();
            getch();
            draw_grid();
        } else {
            draw_grid();
            mvprintw(GRID_SIZE * 2 + 2, 0, "Confirm Solve? (y/n): ");
            refresh();
            int confirm = getch();
            if (confirm == 'y' || confirm == 'Y' || confirm == 10) {
                solve_sudoku();
            } else {
                draw_grid();
            }
        }
    } else if (ch == 'c' || ch == 'C') {
        needs_redraw = false;
        if (count_filled() == 0) {
            draw_grid();
            attron(COLOR_PAIR(COLOR_INVALID));
            mvprintw(GRID_SIZE * 2 + 2, 0, "Nothing to clear!");
            attroff(COLOR_PAIR(COLOR_INVALID));
            refresh();
            getch();
            draw_grid();
        } else {
            draw_grid();
            mvprintw(GRID_SIZE * 2 + 2, 0, "Clear? (y/n): ");
            refresh();
            int confirm = getch();
            if (confirm == 'y' || confirm == 'Y' || confirm == 10) {
                clear_grid();
                solved = false;
            }
            draw_grid();
        }
    } else if (ch == 'q' || ch == 'Q') {
        needs_redraw = false;
        draw_grid();
        mvprintw(GRID_SIZE * 2 + 2, 0, "Quit? (y/n): ");
        refresh();
        int confirm = getch();
        if (confirm == 'y' || confirm == 'Y' || confirm == 10) {
            endwin();
            exit(0);
        }
        draw_grid();
    }

    if (needs_redraw) {
        draw_grid();
        refresh();
    }
}

void solve_sudoku(void) {
    int grid_copy[GRID_SIZE][GRID_SIZE];
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            grid_copy[i][j] = grid[i][j];

    if (!is_valid_full_grid(grid_copy)) {
        draw_grid();
        mvprintw(GRID_SIZE * 2 + 2, 0, "Invalid grid!");
        refresh();
        return;
    }

    clock_t start_time = clock();
    bool solution_found = solve(grid_copy);
    clock_t end_time = clock();

    if (solution_found) {
        for (int i = 0; i < GRID_SIZE; i++)
            for (int j = 0; j < GRID_SIZE; j++)
                grid[i][j] = grid_copy[i][j];
        solved = true;

        draw_grid();
        double elapsed_ns = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1e9;
        mvprintw(GRID_SIZE * 2 + 2, 0, "Solved! Time: %.0fns", elapsed_ns);
        if (count_filled() < 17) {
            attron(COLOR_PAIR(COLOR_INVALID));
            mvprintw(GRID_SIZE * 2 + 3, 0, "Note: This solution may not be unique.");
            attroff(COLOR_PAIR(COLOR_INVALID));
        }
    } else {
        draw_grid();
        mvprintw(GRID_SIZE * 2 + 2, 0, "No solution exists.");
        attron(COLOR_PAIR(COLOR_INVALID));
        mvprintw(GRID_SIZE * 2 + 3, 0, "Warning: Unsolvable grid!");
        attroff(COLOR_PAIR(COLOR_INVALID));
        solved = false;
    }
    refresh();
}

void clear_grid(void) {
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = 0;
            user_grid[i][j] = 0;
        }
    cursor_row = 0;
    cursor_col = 0;
}

int count_filled(void) {
    int count = 0;
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            if (grid[i][j] != 0) count++;
    return count;
}

int main(void) {
    initscr();
    start_color();
    init_pair(COLOR_USER,    COLOR_BLUE,   COLOR_BLACK);
    init_pair(COLOR_SOLVED,  COLOR_WHITE,  COLOR_BLACK);
    init_pair(COLOR_INVALID, COLOR_YELLOW, COLOR_BLACK);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    draw_grid();
    refresh();

    int ch;
    while (1) {
        ch = getch();
        handle_input(ch);
    }

    endwin();
    return 0;
}

#include <ctype.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define GRID_SIZE 9

// Function declarations
void draw_grid(void);
void handle_input(int ch);
bool is_valid(int row, int col, int num, int grid_to_check[GRID_SIZE][GRID_SIZE]);
bool is_valid_full_grid(int grid_to_check[GRID_SIZE][GRID_SIZE]);
bool solve(int grid_to_solve[GRID_SIZE][GRID_SIZE]);
void solve_sudoku(void);
void clear_grid(void);
int count_filled(void);

int grid[GRID_SIZE][GRID_SIZE] = {0};
int user_grid[GRID_SIZE][GRID_SIZE] = {0};
int cursor_row = 0, cursor_col = 0, cursor_line = 1;
bool solved = false;

void draw_grid(void) {
    clear();

    // Add decorative title with ASCII characters
    printw("+-----------------------+\n|     SUDOKU SOLVER     |\n+-----------------------+\n");

    for (int i = 0; i < GRID_SIZE; i++) {
        if (i % 3 == 0) printw("+-------+-------+-------+\n");
        for (int j = 0; j < GRID_SIZE; j++) {
            if (j % 3 == 0) printw("| ");
            if (grid[i][j] != 0) {
                if (user_grid[i][j] == 1) attron(COLOR_PAIR(1)); // Blue for user-entered
                else if (user_grid[i][j] == 2) attron(COLOR_PAIR(3)); // Red for invalid
                else attron(COLOR_PAIR(2)); // White for solution
                printw("%d ", grid[i][j]);
                attroff(COLOR_PAIR(user_grid[i][j] == 1 ? 1 : (user_grid[i][j] == 2 ? 3 : 2)));
            } else printw("  ");
        }
        printw("|\n");
    }
    printw("+-------+-------+-------+\n");
    printw("\n[S]olve / [C]lear / [Q]uit\n");

    // Use cursor_line for vertical placement
    int y = cursor_row + 4; // Basic mapping + 3 for the title
    if (cursor_row > 2) y++; // Adjust for first gridline
    if (cursor_row > 5) y++; // Adjust for second gridline
    move(y, cursor_col * 2 + (cursor_col / 3) * 2 + 2);
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

bool is_valid_full_grid(int grid_to_check[GRID_SIZE][GRID_SIZE]) {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (grid_to_check[row][col] != 0) {
                // Check for duplicates in the same row, column, and 3x3 box
                for (int i = 0; i < GRID_SIZE; i++) {
                    if (i != col && grid_to_check[row][i] == grid_to_check[row][col]) {
                        user_grid[row][i] = 2; // Mark duplicate in row
                        user_grid[row][col] = 2; // Mark the original cell
                    }
                    if (i != row && grid_to_check[i][col] == grid_to_check[row][col]) {
                        user_grid[i][col] = 2; // Mark duplicate in column
                        user_grid[row][col] = 2; // Mark the original cell
                    }
                }
                int box_row_start = row - row % 3;
                int box_col_start = col - col % 3;
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        int r = box_row_start + i;
                        int c = box_col_start + j;
                        if ((r != row || c != col) && grid_to_check[r][c] == grid_to_check[row][col]) {
                            user_grid[r][c] = 2; // Mark duplicate in 3x3 box
                            user_grid[row][col] = 2; // Mark the original cell
                        }
                    }
                }
            }
        }
    }
    // After marking duplicates, check if any cell is marked as invalid
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (user_grid[row][col] == 2) {
                return false; // Invalid grid
            }
        }
    }
    return true; // Valid grid
}

bool solve(int grid_to_solve[GRID_SIZE][GRID_SIZE]) {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (grid_to_solve[row][col] == 0) {
                for (int num = 1; num <= GRID_SIZE; num++) {
                    if (is_valid(row, col, num, grid_to_solve)) {
                        grid_to_solve[row][col] = num;

                        if (solve(grid_to_solve)) {
                            return true;
                        } else {
                            grid_to_solve[row][col] = 0; // Backtrack
                        }
                    }
                }
                return false; // No valid number found for this cell
            }
        }
    }
    return true; // All cells filled
}
void handle_input(int ch) {
    if (isdigit(ch) && ch != '0') {
        int new_num = ch - '0';
        bool is_duplicate = false;

        // Check if the new number is a duplicate of a user-entered number
        if (!solved) {
            for (int i = 0; i < GRID_SIZE; i++) {
                if (i != cursor_col && grid[cursor_row][i] == new_num && user_grid[cursor_row][i] == 1) {
                    is_duplicate = true;
                    break;
                }
                if (i != cursor_row && grid[i][cursor_col] == new_num && user_grid[i][cursor_col] == 1) {
                    is_duplicate = true;
                    break;
                }
            }
            int box_row_start = cursor_row - cursor_row % 3;
            int box_col_start = cursor_col - cursor_col % 3;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    int r = box_row_start + i;
                    int c = box_col_start + j;
                    if ((r != cursor_row || c != cursor_col) && grid[r][c] == new_num && user_grid[r][c] == 1) {
                        is_duplicate = true;
                        break;
                    }
                }
            }
        }

        if (is_duplicate) {
            beep(); // Beep if invalid entry
        } else {
            grid[cursor_row][cursor_col] = new_num;
            user_grid[cursor_row][cursor_col] = 1;
            // Move cursor to the next cell
            if (cursor_col == GRID_SIZE - 1) {
                cursor_col = 0;
                if (cursor_row < GRID_SIZE - 1) {
                    cursor_row++;
                }
            } else {
                cursor_col++;
            }
        }

        // Clear the solution if the grid is modified after solving
        if (solved) {
            for (int i = 0; i < GRID_SIZE; i++) {
                for (int j = 0; j < GRID_SIZE; j++) {
                    if (!user_grid[i][j]) {
                        grid[i][j] = 0;
                    }
                }
            }
            solved = false;
        }
    } else if (ch == '0' || ch == '.' || ch == '-' || ch == '*' || ch == ' ') {
        grid[cursor_row][cursor_col] = 0;
        user_grid[cursor_row][cursor_col] = 0;
        // Move cursor to the next cell
        if (cursor_col == GRID_SIZE - 1) {
            cursor_col = 0;
            if (cursor_row < GRID_SIZE - 1) {
                cursor_row++;
            }
        } else {
            cursor_col++;
        }

        // Clear the solution if the grid is modified after solving
        if (solved) {
            for (int i = 0; i < GRID_SIZE; i++) {
                for (int j = 0; j < GRID_SIZE; j++) {
                    if (!user_grid[i][j]) {
                        grid[i][j] = 0;
                    }
                }
            }
            solved = false;
        }
    } else if (ch == KEY_RIGHT || ch == '\t' || ch == 10) { // 10 is the ASCII code for enter
        if (cursor_col == GRID_SIZE - 1) {
            cursor_col = 0;
            if (cursor_row < GRID_SIZE - 1) {
                cursor_row++;
            }
        } else {
            cursor_col = (cursor_col + 1) % GRID_SIZE;
        }
    } else if (ch == KEY_LEFT || ch == KEY_BTAB) {
        if (cursor_col == 0) {
            cursor_col = GRID_SIZE - 1;
            if (cursor_row > 0) {
                cursor_row--;
            }
        } else {
            cursor_col = (cursor_col - 1 + GRID_SIZE) % GRID_SIZE;
        }
    } else if (ch == KEY_UP) cursor_row = (cursor_row - 1 + GRID_SIZE) % GRID_SIZE;
    else if (ch == KEY_DOWN) cursor_row = (cursor_row + 1) % GRID_SIZE;
    else if (ch == KEY_BACKSPACE || ch == 127) {
        cursor_col = (cursor_col - 1 + GRID_SIZE) % GRID_SIZE;
        grid[cursor_row][cursor_col] = 0;
        user_grid[cursor_row][cursor_col] = 0;
    }

    // Update cursor_line based on cursor_row
    cursor_line = cursor_row + 1; // Basic mapping
    if (cursor_row > 2) cursor_line++; // Adjust for first gridline
    if (cursor_row > 5) cursor_line++; // Adjust for second gridline

    draw_grid(); // Redraw after any input
    refresh();

    if (ch == 's' || ch == 'S') {
        if (count_filled() == 0) {
            attron(COLOR_PAIR(3)); // Set color to yellow
            mvprintw(GRID_SIZE * 2 + 2, 0, "Nothing to solve!");
            attroff(COLOR_PAIR(3));
            refresh();
            getch(); // Wait for a key press before clearing the message
            mvprintw(GRID_SIZE * 2 + 2, 0, "                                    "); // Clear the message
            // Return cursor to its previous position
            move(cursor_line, cursor_col * 2 + (cursor_col / 3) * 2 + 2);
            refresh();
        } else {
            mvprintw(GRID_SIZE * 2 + 2, 0, "Confirm Solve? (y/n): ");
            refresh();
            int confirm = getch();
            if (confirm == 'y' || confirm == 'Y' || confirm == 10) { // 10 is ASCII for Enter
                solve_sudoku();
            } else if (confirm == 'n' || confirm == 'N' || confirm == 27) { // 27 is ASCII for Esc
                mvprintw(GRID_SIZE * 2 + 2, 0, "                                        "); // Clear the prompt
                mvprintw(GRID_SIZE * 2 + 3, 0, "                                                "); // Clear the potential warning
                refresh();
                draw_grid();
            }
        }
    } else if (ch == 'c' || ch == 'C') {
        if (count_filled() == 0) {
            attron(COLOR_PAIR(3)); // Set color to yellow
            mvprintw(GRID_SIZE * 2 + 2, 0, "Nothing to clear!");
            attroff(COLOR_PAIR(3));
            refresh();
            getch(); // Wait for a key press before clearing the message
            mvprintw(GRID_SIZE * 2 + 2, 0, "                                 "); // Clear the message
            // Return cursor to its previous position
            move(cursor_line, cursor_col * 2 + (cursor_col / 3) * 2 + 2);
            refresh();
        } else {
            mvprintw(GRID_SIZE * 2 + 2, 0, "Clear? (y/n): ");
            refresh();
            int confirm = getch();
            if (confirm == 'y' || confirm == 'Y' || confirm == 10) { // 10 is ASCII for Enter
                clear_grid();
                solved = false;
                draw_grid();
                refresh();
            } else if (confirm == 'n' || confirm == 'N' || confirm == 27) { // 27 is ASCII for Esc
                mvprintw(GRID_SIZE * 2 + 2, 0, "                                 "); // Clear the prompt
                refresh();
                draw_grid();
            }
        }
    } else if (ch == 'q' || ch == 'Q') {
        refresh(); // Refresh the screen before displaying the prompt
        mvprintw(GRID_SIZE * 2 + 2, 0, "Quit? (y/n): ");
        refresh();
        int confirm = getch();
        if (confirm == 'y' || confirm == 'Y' || confirm == 10) { // 10 is ASCII for Enter
            endwin();
            exit(0);
        } else if (confirm == 'n' || confirm == 'N' || confirm == 27) { // 27 is ASCII for Esc
            mvprintw(GRID_SIZE * 2 + 2, 0, "                                 "); // Clear the prompt
            refresh();
            draw_grid();
        }
    }
}

void solve_sudoku(void) {
    int grid_copy[GRID_SIZE][GRID_SIZE];
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            grid_copy[i][j] = grid[i][j];

    clock_t start_time = clock(); // Record start time

    bool solution_found = solve(grid_copy); // Store the result of solve()

    if (!is_valid_full_grid(grid_copy)) { // Check for illegal entries
        mvprintw(GRID_SIZE * 2 + 2, 0, "Invalid grid!");
        solved = false;

        draw_grid();
        refresh();
    } else if (solution_found) {
        // Store the cursor position before solving
        int old_row = cursor_row;
        int old_col = cursor_col;

        for (int i = 0; i < GRID_SIZE; i++)
            for (int j = 0; j < GRID_SIZE; j++)
                grid[i][j] = grid_copy[i][j];

        solved = true;
        cursor_row = 0;
        cursor_col = 0;

        // Update cursor_line based on cursor_row
        cursor_line = cursor_row + 1; // Basic mapping
        if (cursor_row > 2) cursor_line++; // Adjust for first gridline
        if (cursor_row > 5) cursor_line++; // Adjust for second gridline

        draw_grid();
        refresh();

        clock_t end_time = clock(); // Record end time
        double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1e9; // Calculate elapsed time in ns

        // Display "Solved!" and warning (if applicable) AFTER refresh
        mvprintw(GRID_SIZE * 2 + 2, 0, "Solved! Time: %.0fns", elapsed_time);
        if (count_filled() < 17) {
            attron(COLOR_PAIR(3));
            mvprintw(GRID_SIZE * 2 + 3, 0, "Note: This solution may not be unique.");
            attroff(COLOR_PAIR(3));
        }
        refresh(); // Refresh again to show the messages

        // Restore the cursor position after solving
        cursor_row = old_row;
        cursor_col = old_col;

        // Update cursor_line based on cursor_row
        cursor_line = cursor_row + 1; // Basic mapping
        if (cursor_row > 2) cursor_line++; // Adjust for first gridline
        if (cursor_row > 5) cursor_line++; // Adjust for second gridline

        // Redraw the grid with the restored cursor position
        draw_grid();
        refresh();
    } else {
        mvprintw(GRID_SIZE * 2 + 2, 0, "No solution exists.");
        solved = false;

        draw_grid();
        refresh();
    }

    // If no solution was displayed, print a warning
    if (!solution_found) {
        attron(COLOR_PAIR(3));
        mvprintw(GRID_SIZE * 2 + 3, 0, "Warning: Unsolvable grid!");
        attroff(COLOR_PAIR(3));
        refresh();
    }
}

void clear_grid(void) {
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = 0;
            user_grid[i][j] = 0;
        }
    cursor_row = 0;
    cursor_col = 0;
    cursor_line = 1; // Reset cursor_line as well
    move(cursor_line, 2); // Move to the top-left corner
}

int count_filled(void) {
    int count = 0;
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            if (grid[i][j] != 0) // Count all non-zero entries
                count++;
    return count;
}

int main(void) {
    initscr();
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    draw_grid();
    refresh();

    int ch;
    while (1) { // Always loop
        ch = getch();
        handle_input(ch); // Call handle_input for every key press
    }

    endwin();
    return 0;
}

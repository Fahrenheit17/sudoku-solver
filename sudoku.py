import curses
import time

try:
    import windows_curses  # noqa: F401
except ImportError:
    pass

GRID_SIZE = 9

COLOR_USER    = 1  # Blue: user-entered
COLOR_SOLVED  = 2  # White: solver-filled
COLOR_INVALID = 3  # Yellow: invalid/duplicate

grid      = [[0] * GRID_SIZE for _ in range(GRID_SIZE)]
user_grid = [[0] * GRID_SIZE for _ in range(GRID_SIZE)]
cursor_row = 0
cursor_col = 0
solved = False


def cursor_y():
    y = cursor_row + 4
    if cursor_row > 2: y += 1
    if cursor_row > 5: y += 1
    return y


def draw_grid(stdscr):
    stdscr.clear()
    stdscr.addstr("+-----------------------+\n|     SUDOKU SOLVER     |\n+-----------------------+\n")
    for i in range(GRID_SIZE):
        if i % 3 == 0:
            stdscr.addstr("+-------+-------+-------+\n")
        for j in range(GRID_SIZE):
            if j % 3 == 0:
                stdscr.addstr("| ")
            if grid[i][j] != 0:
                pair = (COLOR_INVALID if user_grid[i][j] == 2
                        else COLOR_USER if user_grid[i][j] == 1
                        else COLOR_SOLVED)
                stdscr.addstr(str(grid[i][j]) + " ", curses.color_pair(pair))
            else:
                stdscr.addstr("  ")
        stdscr.addstr("|\n")
    stdscr.addstr("+-------+-------+-------+\n")
    stdscr.addstr("\n[S]olve / [C]lear / [Q]uit\n")
    stdscr.move(cursor_y(), cursor_col * 2 + (cursor_col // 3) * 2 + 2)


def is_valid(row, col, num, g):
    if num in g[row]:
        return False
    if num in [g[r][col] for r in range(GRID_SIZE)]:
        return False
    brs, bcs = row - row % 3, col - col % 3
    for i in range(3):
        for j in range(3):
            if g[brs + i][bcs + j] == num:
                return False
    return True


def is_valid_full_grid():
    # Reset stale invalid marks
    for i in range(GRID_SIZE):
        for j in range(GRID_SIZE):
            if user_grid[i][j] == 2:
                user_grid[i][j] = 1

    valid = True
    for row in range(GRID_SIZE):
        for col in range(GRID_SIZE):
            val = grid[row][col]
            if val == 0:
                continue
            for i in range(GRID_SIZE):
                if i != col and grid[row][i] == val:
                    user_grid[row][i] = 2
                    user_grid[row][col] = 2
                    valid = False
                if i != row and grid[i][col] == val:
                    user_grid[i][col] = 2
                    user_grid[row][col] = 2
                    valid = False
            brs, bcs = row - row % 3, col - col % 3
            for i in range(3):
                for j in range(3):
                    r, c = brs + i, bcs + j
                    if (r != row or c != col) and grid[r][c] == val:
                        user_grid[r][c] = 2
                        user_grid[row][col] = 2
                        valid = False
    return valid


def solve(g):
    for row in range(GRID_SIZE):
        for col in range(GRID_SIZE):
            if g[row][col] == 0:
                for num in range(1, GRID_SIZE + 1):
                    if is_valid(row, col, num, g):
                        g[row][col] = num
                        if solve(g):
                            return True
                        g[row][col] = 0
                return False
    return True


def clear_solution_cells():
    global solved
    for i in range(GRID_SIZE):
        for j in range(GRID_SIZE):
            if user_grid[i][j] == 0:
                grid[i][j] = 0
    solved = False


def advance_cursor():
    global cursor_row, cursor_col
    if cursor_col == GRID_SIZE - 1:
        cursor_col = 0
        if cursor_row < GRID_SIZE - 1:
            cursor_row += 1
    else:
        cursor_col += 1


def count_filled():
    return sum(1 for i in range(GRID_SIZE) for j in range(GRID_SIZE) if grid[i][j] != 0)


def clear_grid():
    global cursor_row, cursor_col
    for i in range(GRID_SIZE):
        for j in range(GRID_SIZE):
            grid[i][j] = 0
            user_grid[i][j] = 0
    cursor_row = 0
    cursor_col = 0


def solve_sudoku(stdscr):
    global solved

    if not is_valid_full_grid():
        draw_grid(stdscr)
        stdscr.addstr(GRID_SIZE * 2 + 2, 0, "Invalid grid!")
        stdscr.refresh()
        return

    grid_copy = [row[:] for row in grid]
    start = time.perf_counter_ns()
    solution_found = solve(grid_copy)
    elapsed_ns = time.perf_counter_ns() - start

    if solution_found:
        for i in range(GRID_SIZE):
            for j in range(GRID_SIZE):
                grid[i][j] = grid_copy[i][j]
        solved = True
        draw_grid(stdscr)
        stdscr.addstr(GRID_SIZE * 2 + 2, 0, f"Solved! Time: {elapsed_ns}ns")
        if count_filled() < 17:
            stdscr.addstr(GRID_SIZE * 2 + 3, 0,
                          "Note: This solution may not be unique.",
                          curses.color_pair(COLOR_INVALID))
    else:
        draw_grid(stdscr)
        stdscr.addstr(GRID_SIZE * 2 + 2, 0, "No solution exists.")
        stdscr.addstr(GRID_SIZE * 2 + 3, 0, "Warning: Unsolvable grid!",
                      curses.color_pair(COLOR_INVALID))
        solved = False
    stdscr.refresh()


def handle_input(stdscr, ch):
    global cursor_row, cursor_col, solved
    needs_redraw = True

    if ord('1') <= ch <= ord('9'):
        new_num = ch - ord('0')
        is_duplicate = False
        if not solved:
            for i in range(GRID_SIZE):
                if i != cursor_col and grid[cursor_row][i] == new_num and user_grid[cursor_row][i] == 1:
                    is_duplicate = True; break
                if i != cursor_row and grid[i][cursor_col] == new_num and user_grid[i][cursor_col] == 1:
                    is_duplicate = True; break
            if not is_duplicate:
                brs = cursor_row - cursor_row % 3
                bcs = cursor_col - cursor_col % 3
                for i in range(3):
                    for j in range(3):
                        r, c = brs + i, bcs + j
                        if (r != cursor_row or c != cursor_col) and grid[r][c] == new_num and user_grid[r][c] == 1:
                            is_duplicate = True; break
        if is_duplicate:
            curses.beep()
        else:
            if solved:
                clear_solution_cells()
            grid[cursor_row][cursor_col] = new_num
            user_grid[cursor_row][cursor_col] = 1
            advance_cursor()

    elif ch in (ord('0'), ord('.'), ord('-'), ord('*'), ord(' ')):
        if solved:
            clear_solution_cells()
        grid[cursor_row][cursor_col] = 0
        user_grid[cursor_row][cursor_col] = 0
        advance_cursor()

    elif ch in (curses.KEY_RIGHT, ord('\t'), 10):
        advance_cursor()

    elif ch in (curses.KEY_LEFT, curses.KEY_BTAB):
        if cursor_col == 0:
            cursor_col = GRID_SIZE - 1
            if cursor_row > 0:
                cursor_row -= 1
        else:
            cursor_col -= 1

    elif ch == curses.KEY_UP:
        cursor_row = (cursor_row - 1 + GRID_SIZE) % GRID_SIZE

    elif ch == curses.KEY_DOWN:
        cursor_row = (cursor_row + 1) % GRID_SIZE

    elif ch in (curses.KEY_BACKSPACE, 127):
        if cursor_col > 0:
            cursor_col -= 1
        elif cursor_row > 0:
            cursor_row -= 1
            cursor_col = GRID_SIZE - 1
        grid[cursor_row][cursor_col] = 0
        user_grid[cursor_row][cursor_col] = 0

    elif ch in (ord('s'), ord('S')):
        needs_redraw = False
        if count_filled() == 0:
            draw_grid(stdscr)
            stdscr.addstr(GRID_SIZE * 2 + 2, 0, "Nothing to solve!",
                          curses.color_pair(COLOR_INVALID))
            stdscr.refresh()
            stdscr.getch()
            draw_grid(stdscr)
        else:
            draw_grid(stdscr)
            stdscr.addstr(GRID_SIZE * 2 + 2, 0, "Confirm Solve? (y/n): ")
            stdscr.refresh()
            confirm = stdscr.getch()
            if confirm in (ord('y'), ord('Y'), 10):
                solve_sudoku(stdscr)
            else:
                draw_grid(stdscr)

    elif ch in (ord('c'), ord('C')):
        needs_redraw = False
        if count_filled() == 0:
            draw_grid(stdscr)
            stdscr.addstr(GRID_SIZE * 2 + 2, 0, "Nothing to clear!",
                          curses.color_pair(COLOR_INVALID))
            stdscr.refresh()
            stdscr.getch()
            draw_grid(stdscr)
        else:
            draw_grid(stdscr)
            stdscr.addstr(GRID_SIZE * 2 + 2, 0, "Clear? (y/n): ")
            stdscr.refresh()
            confirm = stdscr.getch()
            if confirm in (ord('y'), ord('Y'), 10):
                clear_grid()
                solved = False
            draw_grid(stdscr)

    elif ch in (ord('q'), ord('Q')):
        needs_redraw = False
        draw_grid(stdscr)
        stdscr.addstr(GRID_SIZE * 2 + 2, 0, "Quit? (y/n): ")
        stdscr.refresh()
        confirm = stdscr.getch()
        if confirm in (ord('y'), ord('Y'), 10):
            return False
        draw_grid(stdscr)

    if needs_redraw:
        draw_grid(stdscr)
        stdscr.refresh()

    return True


def main(stdscr):
    curses.start_color()
    curses.init_pair(COLOR_USER,    curses.COLOR_BLUE,  curses.COLOR_BLACK)
    curses.init_pair(COLOR_SOLVED,  curses.COLOR_WHITE, curses.COLOR_BLACK)
    curses.init_pair(COLOR_INVALID, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.cbreak()
    stdscr.keypad(True)

    draw_grid(stdscr)
    stdscr.refresh()

    while True:
        ch = stdscr.getch()
        if not handle_input(stdscr, ch):
            break


if __name__ == "__main__":
    curses.wrapper(main)

# Open Match Changelog

- Create `CHANGELOG.md` to track project changes
- Create basic raylib 9x9 grid renderer in `src/main.c`
- Add makefile for macOS/Linux with `pkg-config` support
- Add `.gitignore` for C and raylib build artifacts
- Add Homebrew include/lib fallback to `Makefile` for raylib on macOS
 - Update default window to 1920x1080 and enable resizable window in `src/main.c`
 - Center game board within window and keep centered on resize
 - Add pressed tile visual feedback: tile raises with drop shadow while held
 - Improve press effect visibility and robustness of press detection
 - Remove build warnings by making grid dimensions compile-time constants
 - Fix tile hover detection to use exact rectangle collision matching drawing code
 - Correct centering on initial launch by using consistent window dimensions
 - Align mouse coordinates with render DPI scaling on initial launch for correct detection
 - Centered scale-up effect for pressed tiles with improved shadow
 - macOS menu now shows "Open Match" by naming binary with space in makefile
 - Add drag-to-swap interaction: drag raised tile to adjacent tile to swap; invalid drops revert

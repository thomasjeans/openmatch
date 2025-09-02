Open Match Changelog

- Create `CHANGELOG.md` to track project changes
- Create basic raylib 9x9 grid renderer in `src/main.c`
- Add makefile for macOS/Linux with `pkg-config` support
- Add `.gitignore` for C and raylib build artifacts
- Add Homebrew include/lib fallback to `Makefile` for raylib on macOS
 - Update default window to 1920x1080 and enable resizable window in `src/main.c`
 - Center game board within window and keep centered on resize

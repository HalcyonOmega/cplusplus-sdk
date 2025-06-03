# Source Control

---

## Overview

This project uses Git for source control and GitHub for hosting and collaboration. Git is the industry standard for version control, and GitHub makes it easy to manage code, track issues, and collaborate (even if it's just you for now).

## Best Practices

- **Commit Often, Commit Clearly:**
  - Make small, focused commits with clear, descriptive messages.
  - Use the present tense ("Add feature" not "Added feature").
  - Example: `Fix bug in JSON-RPC parsing logic`

- **Branching:**
  - Use feature branches for new features or fixes (e.g., `feature/async-support`, `bugfix/timeout-handling`).
  - Keep `main` (or `master`) stable and always buildable.
  - Merge feature branches via pull requests (even if you're solo—it's good practice and keeps history clean).

- **Pull Requests (PRs):**
  - Use PRs to review changes before merging to `main`.
  - Link PRs to issues when possible.
  - Use PR templates if you want to standardize descriptions/checklists.

- **.gitignore:**
  - Use a `.gitignore` file to exclude build artifacts, temporary files, and IDE/project settings.
  - Example entries:
    ```
    build/
    *.o
    *.obj
    *.exe
    *.log
    .vscode/
    .DS_Store
    ```

- **Tagging Releases:**
  - Use annotated tags for releases (e.g., `v1.0.0`).
  - Example: `git tag -a v1.0.0 -m "First stable release"`
  - Push tags to GitHub: `git push --tags`

- **Write Good Commit Messages:**
  - Start with a short summary (50 chars or less), followed by a blank line and a more detailed explanation if needed.
  - Example:
    ```
    Add async operations support

    Implements std::future-based async API for MCP requests.
    Updates documentation and adds tests.
    ```

- **Keep History Clean:**
  - Rebase and squash commits before merging if it makes the history easier to follow.
  - Avoid committing generated files or large binaries.

## Collaboration

- Even if you're working solo, following these practices makes it easier to onboard collaborators (or impress maintainers if you want this to become the official SDK!).
- Use GitHub Issues and Projects to track bugs, features, and progress.

---

**TL;DR:**
- Use Git and GitHub
- Commit often, with clear messages
- Use branches and PRs
- Ignore build artifacts
- Tag releases
- Keep history clean

# Source Control

-   **System:** Git
-   **Branching Strategy:** (e.g., Gitflow, GitHub Flow)
-   **Commit Message Conventions:** 
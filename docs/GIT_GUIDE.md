# Git Guide

A short, hands-on intro to the git workflow we use on SAMWISE. By the end you
will have opened a pull request (PR), watched it **fail** CI, fixed it, and
pushed again to turn it green. That loop — push, read the failure, fix, push —
is most of what day-to-day work on this repo looks like.

Budget about 30 minutes.

## 0. Setup (once)

```bash
# Tell git who you are (use the email attached to your GitHub account)
git config --global user.name "Your Name"
git config --global user.email "you@stanford.edu"

# Install clang-format (used by our C linter)
brew install clang-format        # macOS
sudo apt install clang-format    # Ubuntu / WSL
```

Make sure you can push to GitHub. The easiest path is the GitHub CLI:

```bash
brew install gh    # or see https://cli.github.com
gh auth login
```

Then clone the repo (skip if you already have it):

```bash
git clone https://github.com/stanford-ssi/samwise-flight-software.git
cd samwise-flight-software
git submodule update --init --recursive
```

## 1. The mental model

| Term | What it is |
| --- | --- |
| **Working tree** | The files on your disk right now. |
| **Staging area** | The changes you've picked for the next commit (`git add`). |
| **Commit** | A saved snapshot with a message. Commits are local until you push. |
| **Branch** | A movable name pointing at a line of commits. `main` is the shared one. |
| **Remote (`origin`)** | The copy on GitHub. `push` sends commits there; `pull` brings them back. |
| **Pull request** | A request to merge your branch into `main`. CI runs on it and teammates review it. |

The rule: **never commit directly to `main`.** Every change goes on its own
branch and lands through a PR.

## 2. Everyday commands

```bash
git status                 # What changed? What's staged? Run this constantly.
git diff                   # Unstaged changes
git diff --staged          # Staged changes (what will be committed)
git log --oneline -10      # Recent history

git switch main            # Go to main
git pull                   # Update it from GitHub
git switch -c my-branch    # Create a new branch from here and switch to it

git add path/to/file       # Stage a file
git commit -m "message"    # Commit what's staged
git push -u origin my-branch   # First push of a new branch
git push                   # Later pushes on the same branch
```

Branch names: short and descriptive, e.g. `adcs/fix-sun-vector` or
`yourname/git-practice`.

Commit messages: one line in the imperative saying what the commit does —
"Fix watchdog timeout", not "fixed stuff".

## 3. Exercise: a PR that fails, then passes

Our CI runs a C/C++ lint check on every PR. It uses the repo's
[`.clang-format`](../.clang-format), which requires **Allman braces** (each `{`
on its own line) and **4-space indents**. You're going to break that on
purpose.

### 3.1 Make a branch

```bash
git switch main
git pull
git switch -c <yourname>/git-practice
```

### 3.2 Add a file

Create `docs/git-practice/<yourname>.c` (use your SUNet ID or first name) and
paste this in **exactly as written** — the formatting is wrong on purpose:

```c
#include <stdio.h>

// Git practice file for <yourname>
int main(void) {
  int sats_launched = 1;
  if (sats_launched > 0) { printf("Hello from <yourname>!\n"); }
  return 0;
}
```

This file isn't part of the build, so it can't break anything — it only exists
for the linter to look at.

### 3.3 Commit and push

```bash
git status                       # your new file shows as untracked
git add docs/git-practice/<yourname>.c
git status                       # now it shows as staged
git commit -m "Add git practice file for <yourname>"
git push -u origin <yourname>/git-practice
```

### 3.4 Open the PR

```bash
gh pr create --base main --title "Git practice: <yourname>" \
  --body "Onboarding git exercise. Please do not merge."
```

(Or click the "Compare & pull request" link that `git push` printed.)

### 3.5 Watch it fail

On the PR page, scroll to the checks at the bottom. Within a minute or two
**C/C++ Lint Check** turns red ❌. Click **Details** to read the log, and look
for the bot's comment on the PR — it shows exactly which lines don't match our
style.

Reading a CI failure and figuring out what it wants is the actual skill here.
Don't skip this step.

### 3.6 Fix it locally

```bash
clang-format -i docs/git-practice/<yourname>.c
git diff                          # see what clang-format changed
```

The braces should now each be on their own line with 4-space indents. Then:

```bash
git add docs/git-practice/<yourname>.c
git commit -m "Format practice file with clang-format"
git push
```

No `-u` and no new PR needed — pushing to the same branch **updates the
existing PR** automatically.

### 3.7 Watch it pass

Back on the PR, CI re-runs on your new commit. The lint check should turn
green ✅. Look at the **Commits** tab: both commits are there, the failing one
and the fix.

Post the PR link in the software channel. A lead will close it **without
merging** so `main` stays clean. Afterwards, tidy up locally:

```bash
git switch main
git branch -D <yourname>/git-practice
```

## 4. On real PRs

- Run `./format_all.sh` (or set up `pre-commit install`, see
  [`.pre-commit-config.yaml`](../.pre-commit-config.yaml)) before pushing C
  code so the linter never surprises you.
- Keep PRs small and focused on one thing.
- If `main` moved while you worked: `git switch main && git pull`, then
  `git switch my-branch && git merge main`, fix any conflicts, and push.
- Address review comments with **new commits** on the same branch, exactly like
  in 3.6.

## 5. Getting unstuck

| Situation | Command |
| --- | --- |
| Undo changes to a file you haven't staged | `git restore <file>` |
| Unstage a file (keep the changes) | `git restore --staged <file>` |
| Fix the message of your last, unpushed commit | `git commit --amend` |
| "Your branch is behind" on push | `git pull`, then `git push` |
| Committed to `main` by accident (not pushed) | `git switch -c new-branch`, then `git switch main && git reset --hard origin/main` |
| No idea what's going on | `git status` and ask in the channel |

Avoid `git push --force` on shared branches unless you know exactly why you
need it.

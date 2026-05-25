# Issue Import

This directory contains generated issue-import artifacts for the current research repository. Regenerate the matching phase artifacts after changing a phase backlog.

Regenerate Phase 1:

```powershell
py -3 Tools\backlog_to_issues.py
```

Regenerate Phase 2:

```powershell
py -3 Tools\backlog_to_issues.py --phase 2 --backlog docs\phase2-backlog.md --base-label platform
```

The CSV is intended as a staging artifact for GitHub issue creation. Review labels, milestone names, and descriptions before creating issues.

Preview Phase 1 GitHub CLI commands without creating anything:

```powershell
py -3 Tools\github_issue_sync.py --check-existing
```

Preview Phase 2 GitHub CLI commands without creating anything:

```powershell
py -3 Tools\github_issue_sync.py --csv docs\issue-import\phase2-issues.csv --check-existing
```

Create reviewed Phase 1 issues only when ready:

```powershell
py -3 Tools\github_issue_sync.py --check-existing --create
```

Create reviewed Phase 2 issues only when ready:

```powershell
py -3 Tools\github_issue_sync.py --csv docs\issue-import\phase2-issues.csv --check-existing --create
```

# Issue Import

This directory contains generated issue-import artifacts for the current research repository. Regenerate them after changing `docs/phase1-backlog.md`.

```powershell
py -3 Tools\backlog_to_issues.py
```

The CSV is intended as a staging artifact for GitHub issue creation. Review labels, milestone names, and descriptions before creating issues.

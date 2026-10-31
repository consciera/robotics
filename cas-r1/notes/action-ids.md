# R1 action IDs

Gesture / action IDs for the Unitree R1 EDU. Each entry names its source and
session. **Reported** = someone or something told us. **Observed** = we triggered
it and saw it.

| ID | Action | Status | Source |
|---|---|---|---|
| 34 | Raise both arms | **Reported by the app assistant, not verified** | Session 04 (2026-10-31) |
| 36 | "Dramatic ta-da", hands forward | **Reported by the app assistant, not verified** | Session 04 (2026-10-31) |

## How these were found

During session 04 the Unitree app's voice assistant volunteered these IDs in
conversation. Nobody asked for them. They haven't been tried. Each one moves to
**Observed** only after it's triggered and the movement is seen.

Published deliberately: these are gesture indices on a machine you must already
physically control. No authentication is bypassed, no remote surface is opened,
and no safety interlock is involved.

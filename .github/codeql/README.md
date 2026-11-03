# CodeQL analysis

The game, IOP module, desktop tools, and compiler plugin have separate build-traced analyses.
NTSC and PAL builds are scanned independently. Wiswa generates the Actions-only workflow.

The standard format-string query produces an explanation graph larger than the CodeQL result
limit. `prepare-queries.py` adapts the installed upstream query to export shortest explanation
paths between its reported sources and sinks. Detection predicates and the final result selection
are unchanged. The generated suite is checked against the current standard suite to ensure every
query remains included.

The query regression check compares complete source and sink matches against the unmodified
query, verifies unsafe and safe calls, and requires explanation paths for every finding. Changes
to the upstream query structure stop preparation until the adapter has been reviewed.

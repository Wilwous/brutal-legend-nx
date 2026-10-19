# Security and sensitive data

Do not attach game files, keys, saves, full memory dumps, or unredacted logs to a public report. Do not submit credentials through an issue.

For a vulnerability, use GitHub's private vulnerability reporting if enabled. Otherwise, open a minimal issue requesting a private reporting channel without exploit details, secrets or private data. There is no dedicated security mailbox at this stage.

An accidental sensitive-data commit is an incident even if later deleted. Stop publication, notify the owner privately through an established channel, rotate exposed credentials when applicable, and review Git history and remote copies. A `.gitignore` rule does not remove tracked content or historical objects.

Game intake uses local tools and never uploads content. Treat disc directory entries as untrusted: reject traversal paths, symlinks, overlapping/out-of-range extents, output collisions and truncation. Extraction must target a new private directory outside Git and must not write to the source image. Hash the original before and after extraction.

No game installer, network service, binary loader, or Switch application is implemented in this revision.

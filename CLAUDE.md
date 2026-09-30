# Notes for Claude

## Commits

- Commits name Frédéric Bogaerts <info@netpack.pt> as author and committer,
  never Claude, with a `Co-Authored-By: Claude ...` trailer so it stays
  visible who wrote the change. Set the identity before the first commit of a
  session: `git config user.name "Frédéric Bogaerts"` and
  `git config user.email "info@netpack.pt"`.
- Work goes on `main`, not on a `claude/...` branch.

## Releases

- Tags are annotated, with a short release note in the style of the v4.0
  tag, and name Frédéric Bogaerts <info@netpack.pt> as the tagger — never
  Claude. With the identity above set, `git tag -a vX.YZ ...` does that.

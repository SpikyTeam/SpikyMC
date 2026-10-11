# Contributing to SpikyMC

Thank you for your interest in contributing!

## Signing Your Work

All commits must be signed off with a [Developer Certificate of Origin](https://developercertificate.org/) (DCO) sign-off.
If you don't know how that works, you can retroactively sign-off all your commits with:

```bash
git rebase --signoff develop
git push --force
```

Note that signing and signing-off are two different things!

## Restrictions on Generative AI Usage (AI Policy)

If you used AI assistance for your contribution, please make sure you are aware of our restrictions:

- AI-generated code must be thoroughly reviewed and tested by a human before submission.
- Do not submit AI-generated code verbatim without understanding it.
- AI assistance is allowed for documentation, refactoring suggestions, and learning purposes.
- The maintainers reserve the right to reject AI-generated contributions that do not meet quality standards.

## Pull Requests

1. Fork the repository and create a feature branch from `develop`.
2. Follow the existing code style (see `.clang-format`).
3. Make sure your commits are signed off.
4. Open a pull request with a clear description of the changes.
5. Ensure CI passes and address review feedback.

## Code Style

- C++23, formatted with [clang-format](.clang-format).
- Follow the existing patterns in the codebase.

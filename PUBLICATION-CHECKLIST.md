# Moss publication checklist / 2026-10-03

Intended public repository: https://github.com/t4kUM1o/moss-launcher

This is a local preparation checklist, not proof of a GitHub upload or a complete
credential audit. No commit, remote change, push or binary release was made while
preparing this document. GitHub visibility/content could not be verified using
the web reader at this step.

## Before the first source upload

- Publish only the Moss source tree, not the parent Codex workspace, `outputs`,
  `work`, previous Python launcher, personal settings or any `UserData`.
- Keep the independent-project README, upstream attribution, licenses, build
  instructions and modifications. Do not claim API approval or completed features.
- Review both the exact proposed files and any Git history to be uploaded for
  private credentials, personal data and unrelated files. Ignore patterns alone
  do not protect already tracked files or previous commits.
- The current local `origin` is the Prism Launcher upstream. Do not push to it or
  change it without confirming the intended publication method. The new Moss URL
  is a separate destination; do not force-push or replace existing remote content.
- Choose deliberately between preserving upstream history and publishing a clean
  source snapshot. A source snapshot must retain attribution and the baseline
  commit identity. It must include libnbtplusplus source or a valid pinned
  submodule reference so the published source can be built.
- Audit `.github` before upload. Inherited workflows, publishing scripts,
  funding links and support templates are not configured for Moss. Do not enable
  upstream automated builds, scheduled tasks or publishing as a side effect of
  the first push; disable/adapt them in the proposed publication tree first.
- Logs under `tests/testdata/TestLogs` are existing upstream test fixtures, not
  evidence of this user's gameplay. Do not remove required fixtures merely
  because they end in `.log`; review provenance and modifications instead.
- Run the fork checks and verify the proposed source contents. A README-only
  change does not require launching the application or privileged link tests.
- Obtain the owner's approval of the exact public scope before pushing. Do not
  attach binaries or publish a GitHub Release without separate authorization.

## API application and binaries

- The application form's website field needs a publicly accessible URL containing
  meaningful information about Moss; an empty repository does not provide that.
  A repository URL does not guarantee Minecraft API approval.
- Keep the form's application name/client ID consistent with the registered app.
  Contact address, tenant ID and the user's acceptance of terms must be supplied
  and confirmed by the owner, not invented by the launcher.
- Treat the Microsoft client ID as a public identifier, not a private secret.
  Do not publish client secrets, provider API keys, access/refresh tokens,
  one-time codes or login callback URLs.
- Before a public binary release: obtain needed API approval, verify real-account
  authentication and game ownership checks, address credential storage/log
  privacy, and provide matching sources/dependency notices with the runtime.

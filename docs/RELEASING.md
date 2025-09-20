# Releasing (local workflow)

Minimal steps to publish a change to npm from your local machine. Run these from the repository root.

1) Run tests and install deps

```powershell
npm ci
npm test
```

2) Bump version, commit, and push tag (recommended)

```powershell
# Bump patch version and create annotated git tag
npm version patch
# Push commit and tag to origin
git push origin main --follow-tags
```

3) Or create/push a tag only

```powershell
# Create annotated tag and push (replace vX.Y.Z)
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin vX.Y.Z
```

4) Verify release and npm

```powershell
# Check Actions UI for the `prebuild-and-publish` run and Releases for created release
npm view @ssxv/node-printer version
```

That's it — keep this file short and update only if the CI trigger or publish process changes.
# Releasing and tagging

This doc contains the step-by-step commands to release a new version of `@ssxv/node-printer`. Use these commands locally or in automation when preparing a new release.

## Pre-requisites
- Ensure your working tree is clean (`git status --porcelain` should be empty)
- Ensure tests pass locally: `npm ci && npm test`
- You must have permission to push tags and releases to the repository
- Ensure `NPM_TOKEN` is set in GitHub repository secrets if you want CI to publish to npm

## Quick manual release (recommended for small projects)
1. Bump version and create a git tag (this updates `package.json` and commits it):

```powershell
# Bump patch version and create annotated git tag
npm version patch
# Push commit and tag
git push origin main --follow-tags
```

2. Wait for CI to run the release workflow (your repository is configured to trigger on `push` tags `v*`).

3. Verify on GitHub:
- Actions tab: watch the `prebuild-and-publish` workflow
- Releases tab: a release should be created with attached prebuild assets

4. Verify package on npm:

```powershell
npm view @ssxv/node-printer version
```

## Manual tag-only release (if you don't want `package.json` changed locally)
1. Create an annotated tag:

```powershell
# Create annotated tag (replace v0.1.1 with your desired semver tag)
git tag -a v0.1.1 -m "Release v0.1.1"
# Push tag
git push origin v0.1.1
```

2. CI will run the `prebuild-and-publish` workflow and publish.

## Release with dry-run (test the workflow without publishing to npm)
If you want to validate the prebuild artifact generation and release creation without actually publishing to npm, temporarily change the `publish` step in `.github/workflows/prebuild-and-publish.yml` to:

```yaml
- name: Publish to npm (DRY-RUN)
  run: echo "DRY RUN: would run npm publish here"
```

Then push a test tag (for example `v0.0.0-test`) and observe the Actions run and the Release assets.

## Reverting a release/tag
If something went wrong and you need to undo:

```powershell
# Delete local tag
git tag -d v0.1.1
# Delete remote tag
git push origin :refs/tags/v0.1.1
# If npm publish occurred and you need to unpublish (use with caution):
npm unpublish @ssxv/node-printer@0.1.1 --force
# (Unpublishing is restricted for packages older than 72 hours on npm and has policies)
```

## CI and secrets
- `NPM_TOKEN` must be stored in repository secrets to allow CI to publish to npm

- Release creation note: this repository's release workflow is configured to use a Personal
  Access Token (PAT) stored as a repository secret named `RELEASE_TOKEN`. Some repository
  or organization policies prevent the default `GITHUB_TOKEN` from creating releases or
  uploading release assets (resulting in HTTP 403 during the release step). To ensure the
  release step can create the GitHub Release and attach the prebuilt artifacts, create a
  PAT and add it to repository secrets as `RELEASE_TOKEN` (instructions below).

- How to create a PAT and add it as `RELEASE_TOKEN`:
  1. Open GitHub → Settings → Developer settings → "Personal access tokens" → "Tokens (classic)" → "Generate new token".
  2. Give the token a descriptive name (e.g. `node-printer-release-bot`) and an expiration you are comfortable with.
  3. Grant the token the `repo` scope (this allows creating releases and uploading assets). For public-only repos you may be able to use `public_repo` instead.
  4. Generate the token and copy the value (you will not be able to view it again).
  5. In your repository: Settings → Security → Secrets and variables → Actions → New repository secret.
     - Name: `RELEASE_TOKEN`
     - Value: paste the PAT value you copied
  6. Save the secret.

- Alternative: if your repository settings allow it, you can instead grant the workflow's `GITHUB_TOKEN` the required permissions by adding a `permissions:` block at the top of the workflow (or enabling the permission in repository settings). Example workflow-level snippet:

```yaml
permissions:
  contents: write
  packages: write
```

  However, many organizations restrict elevating `GITHUB_TOKEN` permissions; if that is the case you must use a PAT and `RELEASE_TOKEN` as described above.

- If `RELEASE_TOKEN` is not present (or a PAT lacks the required scopes), the release creation step may fail with HTTP 403 and the workflow will be unable to attach prebuild assets.

## Notes and best practices
- Use `v` prefix tags like `v1.2.3` to match the release workflow's `v*` trigger
- Prefer the `npm version` flow to ensure `package.json` and git are in sync
- Use Release notes in GitHub Releases to document what's changed for users
- Consider signing release artifacts if you want extra assurance for consumers

---
Generated by developer tooling. Keep this file updated if you change the release workflow.

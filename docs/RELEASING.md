# Releasing

Step-by-step commands to release a new version of `@ssxv/node-printer`.

## Prerequisites
- Clean working tree (`git status --porcelain` should be empty)
- Tests pass: `npm ci && npm test`
- Push permissions to repository
- `NPM_TOKEN` set in GitHub repository secrets

## Release Methods

### Method 1: Version bump (recommended)
```powershell
# Bump version, commit, and create tag
npm version patch
git push origin main --follow-tags
```

### Method 2: Tag only
```powershell
# Create and push tag (replace vX.Y.Z)
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin vX.Y.Z
```

## Verification
After pushing, CI will trigger the `prebuild-and-publish` workflow. Verify:
- GitHub Actions: `prebuild-and-publish` workflow completes
- GitHub Releases: new release with prebuild assets
- npm: `npm view @ssxv/node-printer version`

## Troubleshooting

### Reverting a release
```powershell
# Delete local and remote tag
git tag -d vX.Y.Z
git push origin :refs/tags/vX.Y.Z
# Unpublish from npm (use with caution, restricted after 72 hours)
npm unpublish @ssxv/node-printer@X.Y.Z --force
```

### CI Setup
If release creation fails with HTTP 403, add a Personal Access Token:
1. GitHub → Settings → Developer settings → Personal access tokens → Generate new token
2. Grant `repo` scope, copy the token value
3. Repository → Settings → Secrets → New repository secret
   - Name: `RELEASE_TOKEN`
   - Value: paste token

## Notes
- Use `v` prefix tags (e.g., `v1.2.3`) to trigger the release workflow
- Prefer `npm version` to keep `package.json` and git in sync

# Release Process

---

## Overview

A clear release process helps ensure that every version of the SDK is stable, well-documented, and easy for users to adopt. This project uses **epoch semantic versioning** for all releases.

## Versioning Scheme

- **Epoch Semantic Versioning:**
  - Version numbers are in the form: `EPOCH.MAJOR.MINOR.PATCH` (e.g., `1.0.0.0`).
  - The epoch is incremented for breaking changes that require a reset of the versioning scheme (rare).
  - MAJOR, MINOR, and PATCH follow standard semantic versioning rules:
    - **MAJOR:** Breaking changes
    - **MINOR:** New features, backward compatible
    - **PATCH:** Bug fixes, backward compatible

## Release Steps

1. **Prepare the Release:**
   - Make sure all tests pass and CI is green on all supported platforms.
   - Update documentation, changelog, and version numbers in CMake and config files.
   - Review and merge all relevant PRs.

2. **Tag the Release:**
   - Create an annotated Git tag with the version number (e.g., `1.2.0.0`).
   - Example: `git tag -a 1.2.0.0 -m "Release 1.2.0.0"`
   - Push tags to GitHub: `git push --tags`

3. **Create a GitHub Release:**
   - Draft a new release on GitHub, using the tag as the version.
   - Write clear release notes:
     - Highlight new features, bug fixes, and breaking changes.
     - Link to relevant issues and PRs.
   - Attach pre-built binaries/packages if available.

4. **Pre-Releases (Optional):**
   - For beta or RC releases, use pre-release tags (e.g., `1.2.0.0-beta1`).
   - Mark the release as a pre-release on GitHub.

5. **CI/CD Automation:**
   - Automate builds, tests, and packaging for each release using CI (e.g., GitHub Actions).
   - Optionally, automate publishing to package managers (vcpkg, Conan) or uploading binaries.

6. **Post-Release:**
   - Announce the release (README, project site, etc.).
   - Monitor for issues or regressions and patch as needed.

## Changelog

- Maintain a `CHANGELOG.md` with a summary of changes for each release.
- Follow [Keep a Changelog](https://keepachangelog.com/) style for clarity.

---

**TL;DR:**
- Use epoch semantic versioning (EPOCH.MAJOR.MINOR.PATCH)
- Tag and document every release
- Automate builds/tests with CI
- Write clear release notes and changelogs
- Patch quickly if issues are found

-   **Release Checklist:** 
-   **CI/CD Pipeline for Releases:** 
-   **Tagging and Version Bumping Strategy:** 

</rewritten_file> 
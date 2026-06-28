# Documentation Directory

This directory contains the project documentation that is published to GitHub Pages.

## Structure

- **index.html** - Main landing page for GitHub Pages (published to gh-pages branch)
- **wiki/** - Documentation markdown files (published to GitHub Pages)
  - Home.md - Documentation home page
  - RWLContainer.md - RWLContainer API documentation
  - WaitableQueue.md - WaitableQueue API documentation
  - Configuration.md - Configuration guide
  - Examples.md - Usage examples
  - FAQ.md - Frequently asked questions
  - Getting-Started.md - Getting started guide
  - _Sidebar.md - Navigation sidebar
  - _Footer.md - Footer content
- **API.md** - Detailed API reference
- **Tests.md** - Testing documentation

## Publishing

Documentation is automatically published to GitHub Pages during the CI/CD pipeline:

1. **GitHub Pages** (gh-pages branch)
   - Triggered on main/master branch commits
   - Publishes index.html and markdown files
   - Accessible at: https://siddiqsoft.github.io/RWLContainer/

## Local Development

To preview the documentation locally:

1. Open `index.html` in a web browser
2. For markdown files, use a markdown viewer or GitHub's web interface

## Editing

When editing documentation:

1. Update files in this directory
2. Commit changes to the main branch
3. The CI/CD pipeline will automatically publish changes to GitHub Pages

## Guidelines

- Keep documentation up-to-date with code changes
- Use clear, concise language
- Include code examples where appropriate
- Link to related documentation
- Update version numbers when releasing new versions

---
id: troubleshooting
title: Build and launch troubleshooting
description: Build and launch troubleshooting
section: Getting started
order: 6
revision: 1
---

## Build and launch {#launch}

Use the source/catalogue version installed in this project. Build the native editor with app/build.ps1 and configure Python/Core with app/setup_mcp.ps1. Confirm Visual Studio C++, the Windows SDK, Git access to pinned dependencies and CMake. Generated historical candidates are not executable replacements.

Launch the resulting CyGameEditor.exe or Lancer.cmd. The launcher quotes its path and sets the working directory; immediate failures appear in its console. Native logs are under data/logs. An access-denied failure needs the exact executable, file rights and Windows/application-control logs checked. Do not rename an old binary or disable protection as a substitute for diagnosis. Preserve projects/assets/preferences.

## A missing tool or manual {#missing}

A view does not establish a codec or engine compiler. Check the adapter descriptor and executable provider registry. For material/import/build failures, inspect Core job state, reports and runtime setup. Missing Documentation/Documentation.html means the complete generated documentation folder was not built or copied. Regenerate the manual using the documented source build rather than opening the obsolete six-section web guide.

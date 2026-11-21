# Security Policy

## Supported versions

| Version | Supported |
|---|---|
| 1.0.x | Yes |

## Reporting a vulnerability

Please do not open a public issue for a security problem.

Use GitHub's private vulnerability reporting for this repository
(**Security** → **Report a vulnerability**), or contact the maintainer through
the GitHub profile listed in the repository.

Include:

- affected version and build,
- host/DAW and operating system,
- a description of the issue and its impact,
- reproduction steps or a minimal test case,
- any suggested fix.

You will receive an acknowledgement as soon as possible. Please allow time for
a fix and a coordinated release before publishing details.

## Scope

Crestwright is a local audio effect. It does not make network requests
(`JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0`) and does not collect telemetry.
Relevant areas include the plug-in state parser, the installer, and the build
scripts.

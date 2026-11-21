@echo off
rem SPDX-License-Identifier: AGPL-3.0-or-later
rem Copyright (c) 2026 Linning
rem Run this after closing Cubase. Older builds were renamed instead of deleted
rem because Cubase keeps the loaded DLL locked.
for /d %%D in ("C:\Program Files\Common Files\VST3\Crestwright.vst3.old*") do rmdir /s /q "%%D"
for /d %%D in ("C:\Program Files\Common Files\VST3\Crestwright (old).vst3") do rmdir /s /q "%%D"

echo Cleanup done. If anything remains, close Cubase first.
pause

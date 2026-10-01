$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force -Path '../build/host-tests' | Out-Null
    foreach ($test in @('test_button','test_dsp','test_voicing','test_panel_mapping','test_tuning','test_speed_cv')) {
        & cl /nologo /EHsc /O2 /W4 /std:c++14 "$test.cpp" "/Fe:../build/host-tests/$test.exe" "/Fo:../build/host-tests/$test.obj"
        if ($LASTEXITCODE -ne 0) { throw "$test compilation failed: $LASTEXITCODE" }
        & "../build/host-tests/$test.exe"
        if ($LASTEXITCODE -ne 0) { throw "$test failed: $LASTEXITCODE" }
    }
} finally { Pop-Location }





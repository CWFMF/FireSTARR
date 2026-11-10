$ErrorActionPreference = "Stop"

$DIR = pwd
$REPO = "firestarr"
$BRANCH = "unstable"

$VSWHERE = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe"
If(Test-Path "${VSWHERE}")
{
    # detect shell to get msvcs
    $vsPath = &("${VSWHERE}") -property installationpath
} else {
    # Build tools is okay for building open source projects with a licence, whereas vs community is
    winget install --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    $vsPath = &("${VSWHERE}") -products Microsoft.VisualStudio.Product.BuildTools -property installationpath
}
# vsPath might not have a value until reboot?
If(${vsPath})
{
    echo "Found path ${vsPath}"
} else {
    $vsPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
    echo "Assuming path ${vsPath}"
}
Import-Module "${vsPath}\\Common7\\Tools\\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -Arch amd64

git --version
if(!$?) { winget install Git.Git }

git clone https://github.com/microsoft/vcpkg

# setting a VCPKG_ROOT means cmake ends up in a directory that isn't part of the path?
# $env:VCPKG_ROOT=Resolve-Path vcpkg
# mkdir $env:VCPKG_ROOT\\installed
# $env:VCPKG_INSTALLED=Resolve-Path vcpkg\\installed

# avoid dlls
$env:VCPKG_TARGET_TRIPLET="x64-windows-static"
pushd vcpkg
$env:VCPKG_ROOT=pwd
git checkout 2025.12.12
.\bootstrap-vcpkg.bat -disableMetrics
popd
# need to run install so cmake exists
vcpkg\vcpkg install
cmake --preset Release
cmake --build --parallel --preset Release

ctest --preset Release

# .\test_duff.exe -v
# .\test_fbp.exe -v
# .\test_fwi.exe -v

# .\firestarr.exe -h

# # this needs too many resources for most computers right now,
# # but you can cancel with ctrl + c at any point
# # mkdir dir_test; .\firestarr.exe test dir_test all

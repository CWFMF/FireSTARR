echo ENSURE YOU ARE RUNNING IN AN ADMIN COMMAND PROMPT
PAUSE
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
pushd ..

git clone https://github.com/Microsoft/vcpkg.git
pushd vcpkg
call  .\bootstrap-vcpkg.bat
@rem ~ vcpkg.exe install sqlite3[core,tool]:x86-windows tiff:x86-windows curl:x86-windows libjpeg-turbo:x86-windows
vcpkg.exe install sqlite3[core,tool]:x64-windows tiff:x64-windows curl:x64-windows libjpeg-turbo:x64-windows
popd

git clone https://github.com/OSGeo/PROJ.git
pushd PROJ
@rem ~ git checkout tags/6.3.1
git checkout tags/5.2.0
@rem ~ git checkout tags/6.2.1
mkdir build
cd build
@rem ~ cmake -DCMAKE_TOOLCIN_FILE=..\..\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_PREFIX_PATH=..\..\vcpkg\installed\x64-windows\ -DTIFF_INCLUDE_DIR=..\..\vcpkg\installed\x64-windows\include -DTIFF_LIBRARY_RELEASE=..\..\vcpkg\installed\x64-windows\lib\tiff.lib -DCURL_LIBRARY=..\..\vcpkg\installed\x64-windows\lib\libcurl.lib -DBUILD_SHARED_LIBS=ON ..
SET CMAKE=cmake -G "Visual Studio 15 2017 Win64"
SET CMAKE=%CMAKE% -DBUILD_TESTING=OFF
SET CMAKE=%CMAKE% -DCMAKE_TOOLCHAIN_FILE=..\..\vcpkg\scripts\buildsystems\vcpkg.cmake
SET CMAKE=%CMAKE% -DCMAKE_PREFIX_PATH=..\..\vcpkg\installed\x64-windows\
SET CMAKE=%CMAKE% -DTIFF_INCLUDE_DIR=..\..\vcpkg\installed\x64-windows\include
SET CMAKE=%CMAKE% -DTIFF_LIBRARY_RELEASE=..\..\vcpkg\installed\x64-windows\lib\tiff.lib
SET CMAKE=%CMAKE% -DCURL_LIBRARY=..\..\vcpkg\installed\x64-windows\lib\libcurl.lib
SET CMAKE=%CMAKE% -DBUILD_SHARED_LIBS=ON
SET CMAKE=%CMAKE% -DCMAKE_BUILD_TYPE=Release
SET CMAKE=%CMAKE% -DJPEG_INCLUDE_DIR=..\..\vcpkg\installed\x64-windows\include
SET CMAKE=%CMAKE% -DJPEG_LIBRARY=..\..\vcpkg\installed\x64-windows\lib\jpeg.lib
SET CMAKE=%CMAKE% -DLZMA_LIBRARY=..\..\vcpkg\installed\x64-windows\lib\lzma.lib
SET CMAKE=%CMAKE% -DLibLZMA_DIR=..\..\vcpkg\installed\x64-windows\bin
SET CMAKE=%CMAKE% -DCMAKE_PREFIX_PATH=..\..\vcpkg\installed\x64-windows\share\curl
SET CMAKE=%CMAKE% ..

%CMAKE%
cmake --build . --config Debug
cmake --build . --config Release
popd

git clone https://gitlab.com/libtiff/libtiff.git
git clone https://github.com/OSGeo/libgeotiff.git
@rem ~ wget https://download.osgeo.org/osgeo4w/osgeo4w-setup-x86_64.exe
@rem ~ @rem fix wrong permissions after download
@rem ~ icacls osgeo4w-setup-x86_64.exe /t /q /c /reset

@rem ~ osgeo4w-setup-x86_64.exe -q -k -r -A -s http://download.osgeo.org/osgeo4w/ -a x86_64 -R c:\OSGeo4W -P proj,libtiff,libgeotiff
pushd libtiff
nmake /f Makefile.vc
popd

pushd libgeotiff\libgeotiff
nmake /f Makefile.vc
popd

popd


./configure --target=x86-win32-vs12 --enable-static-msvcrt --disable-mmx --disable-sse --disable-sse2 --disable-sse3 --disable-ssse3 --disable-sse4_1 --disable-runtime_cpu_detect --disable-unit_tests --disable-libyuv --disable-postproc

make

sed -i 's/aom_ports_emms_asm/emms/g' aom.vcxproj
sed -i 's+/c/Data/Projects/WebM/aom+.+g' aom.vcxproj
sed -i 's+/c/Data/Projects/WebM/aom+.+g' aomdec.vcxproj
sed -i 's+/c/Data/Projects/WebM/aom+.+g' aomenc.vcxproj
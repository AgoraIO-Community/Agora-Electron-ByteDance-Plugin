ls -alt
ls -alt ./bytedance
cp ./bytedance/glew/bin/Release/Win32/*.dll ./Release/.
cp ./bytedance/effect/libs/msvc14/Win32/Release/*.dll ./Release/.
7z a -r -tzip bytedance-win.zip ./Release/.

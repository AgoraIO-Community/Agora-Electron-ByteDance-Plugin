ls -alt
ls -alt ./bytedance
cp ./bytedance/glew/**/*.dll ./Release/.
cp ./bytedance/effect/**/*.dll ./Release/.
7z a -r -tzip bytedance-win.zip ./Release

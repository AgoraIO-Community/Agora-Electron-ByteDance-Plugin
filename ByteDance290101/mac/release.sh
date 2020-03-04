rm -rf byted_effect*
rm -rf build
rm -rf bytedance

unzip -u bytedance.zip
mkdir -p bytedance/byted_effect_mac_headers
unzip -u byted_effect*/byted_effect_mac_headers.zip -d bytedance/byted_effect_mac_headers

mv byted_effect*/libeffect.dylib bytedance/

rm -rf build
xcodebuild -project ByteDancePlugin.xcodeproj
cd build
zip -r bytedance-mac.zip Release

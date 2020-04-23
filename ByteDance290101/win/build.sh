
rm -rf lab_tob*
rm -rf glew
rm -rf openssl
rm -rf bytedance
rm -rf Release

unzip -u bytedance.zip
unzip -u glew.zip
unzip -u openssl.zip
mkdir bytedance
mv lab_tob*/* bytedance/.
mv glew bytedance/.
set -e

sudo apt update
sudo apt install -y pip
pip install -U conan==1.60.0
conan profile new screen-viewer-profile --detect --force
conan profile update settings.compiler.libcxx=libstdc++11 screen-viewer-profile
conan profile update conf.tools.system.package_manager:mode=install screen-viewer-profile
conan profile update conf.tools.system.package_manager:sudo=True screen-viewer-profile

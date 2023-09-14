### 0. Install the Latest CMake

Please follow https://apt.kitware.com/ to get latest cmake. Ensure ```cmake --version``` >= 3.27.

### 1. Install System Dependencies for VRS and OpenCV

#### OpenCV dependencies
```
sudo apt-get install -y libcurl4 build-essential pkg-config cmake \
    libopenblas-dev libeigen3-dev libtbb-dev \
    libavcodec-dev libavformat-dev \
    libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev \
    libswscale-dev libgtk-3-dev libpng-dev libjpeg-dev \
    libcanberra-gtk-module libcanberra-gtk3-module
```

#### VRS dependencies
```
sudo apt-get install cmake git ninja-build ccache libgtest-dev libfmt-dev libturbojpeg-dev libpng-dev
sudo apt-get install liblz4-dev libzstd-dev libxxhash-dev
sudo apt-get install libboost-system-dev libboost-filesystem-dev libboost-thread-dev libboost-chrono-dev libboost-date-time-dev
sudo apt-get install qtbase5-dev portaudio19-dev
sudo apt-get install npm doxygen
```

### 2. Build OpenCV (w. opencv_contrib)
```
wget -O opencv.zip https://github.com/opencv/opencv/archive/4.8.0.zip
wget -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/4.8.0.zip
unzip opencv.zip
unzip opencv_contrib.zip
cd opencv-4.8.0
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON \
	-DCMAKE_INSTALL_PREFIX=/usr/local \
	-DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.8.0/modules -DOPENCV_ENABLE_NONFREE=ON\
	-DBUILD_opencv_python3=OFF -DWITH_1394=OFF \
	-DWITH_IPP=ON -DWITH_TBB=ON -DWITH_OPENMP=ON -DWITH_PTHREADS_PF=ON \
	-DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DOPENCV_GENERATE_PKGCONFIG=ON \
	-DWITH_CUDA=ON -DENABLE_FAST_MATH=ON -DCUDA_FAST_MATH=ON -D WITH_CUBLAS=ON \
make all -j$(grep -c ^processor /proc/cpuinfo)
sudo make install -j$(grep -c ^processor /proc/cpuinfo)
```

### 3. Build VRS with Face Mosaic
Note: return to the root folder after opencv installed.

```
cd vrs
mkdir -p build && cd build
cmake ..
make all -j$(grep -c ^processor /proc/cpuinfo)
ctest -j8
sudo make install -j$(grep -c ^processor /proc/cpuinfo)
```

### 4. Running

- Download model from https://github.com/opencv/opencv_zoo/blob/main/models/face_detection_yunet/face_detection_yunet_2023mar.onnx, and 

```
sudo mv face_detection_yunet_2023mar.onnx /usr/local/share/opencv4/
```

- (Optional? Do it for safety)
```
sudo vi /etc/ld.so.conf.d/opencv.conf
``` 
(should be an empty file), write ```/usr/local/lib``` into the file and save. Then 
```
sudo ldconfig -v
```

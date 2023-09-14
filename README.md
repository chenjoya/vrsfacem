## VRSFaceM: Face Mosaic for VRS

A C++ implementation to mask human face in .vrs file, which is really helpful for privacy protection.

Face detection algorithm used: https://github.com/ShiqiYu/libfacedetection:

> Performance on WIDER Face AP_easy=0.887, AP_medium=0.871, AP_hard=0.768

### 1. Install System Dependencies for VRS and OpenCV

#### OpenCV dependencies
```
sudo apt-get install -y libcurl4 build-essential pkg-config \
    libopenblas-dev libeigen3-dev libtbb-dev \
    libavcodec-dev libavformat-dev \
    libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev \
    libswscale-dev libgtk-3-dev libpng-dev libjpeg-dev \
    libcanberra-gtk-module libcanberra-gtk3-module
```

#### VRS dependencies
```
sudo apt-get install -y git ccache libgtest-dev libfmt-dev libturbojpeg-dev libpng-dev
sudo apt-get install -y liblz4-dev libzstd-dev libxxhash-dev
sudo apt-get install -y libboost-system-dev libboost-filesystem-dev libboost-thread-dev libboost-chrono-dev libboost-date-time-dev
sudo apt-get install -y qtbase5-dev portaudio19-dev
sudo apt-get install -y npm doxygen
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
	-DWITH_CUDA=ON -DENABLE_FAST_MATH=ON -DCUDA_FAST_MATH=ON -D WITH_CUBLAS=ON
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

- (Do it if encounter opencvlib not found error)
```
sudo vi /etc/ld.so.conf.d/opencv.conf
``` 
(should be an empty file), write ```/usr/local/lib``` into the file and save. Then 
```
sudo ldconfig -v
```

- Apply this face mosaic to .vrs file
```
./vrs/build/tools/vrs/vrs copy --facem-vrs input_test.vrs --to output_test.vrs
```

### Contact

joyachen@u.nus.edu
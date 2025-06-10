# HaBaGa 图像处理软件
[切换到中文](#中文版) | [Switch to English](#english-version)
## 中文版
### 功能描述
本软件是一个基于MFC框架开发的医学图像处理工具，主要功能包括：
- 图像增强与灰度调整
- 频域滤波处理（使用FFTW库实现）
- 噪声添加与去除
- 直方图分析与均衡化
- 频谱分析与显示

### 技术原理
1. **图像增强**：通过线性/非线性灰度变换调整图像对比度
2. **频域处理**：使用快速傅里叶变换(FFT)将图像转换到频域进行滤波
3. **噪声模型**：实现高斯噪声、椒盐噪声等常见噪声模型
4. **直方图处理**：统计像素分布并实现直方图均衡化算法

### 开发团队（按姓氏首字母排序）
- 何毕阳
- 胡竣斐
- 尚舒林
- 王颢天

---

## English Version
### Features
This is a medical image processing tool based on MFC framework with following features:
- Image enhancement and grayscale adjustment
- Frequency domain filtering (using FFTW library)
- Noise addition and removal
- Histogram analysis and equalization
- Spectrum analysis and visualization

### Technical Principles
1. **Image Enhancement**: Adjust image contrast through linear/non-linear grayscale transformation
2. **Frequency Processing**: Use Fast Fourier Transform (FFT) to convert images to frequency domain for filtering
3. **Noise Models**: Implement common noise models including Gaussian noise and salt-and-pepper noise
4. **Histogram Processing**: Pixel distribution statistics and histogram equalization algorithm

### Development Team (sorted by surname initial)
- Biyang He
- Junfei Hu
- Shulin Shang
- Haotian Wang


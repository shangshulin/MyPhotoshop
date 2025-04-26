
#pragma once
#include "pch.h"
#include <vector>
#include <complex>

enum class FilterType {
    Mean,
    Median,
    Max
};

class CImageProc {
public:
    CImageProc();
    ~CImageProc();
    bool IsValid() const { return m_hDib != NULL && pDib != NULL; }
    void CleanUp();

	//计算对齐后的宽度
    int GetAlignedWidthBytes() const {
        // 计算每行对齐后的字节数 (每行必须4字节对齐)
        return ((nWidth * nBitCount / 8) + 3) & ~3;
    }

    //图片加载与显示
    void OpenFile();
    void LoadBmp(CString stFileName);
    void ShowBMP(CDC* pDC, int x, int y, int destWidth, int destHeight);
    void DisplayColor(CClientDC* pDC,int imgX, int imgY, int x, int y);
    void GetColor(int x, int y, BYTE& red, BYTE& green, BYTE& blue);

    //灰度处理
	std::vector<int> CalculateHistogramMix(); // 计算灰度直方图
    std::vector<std::vector<int>> CalculateHistogramRGB();// 计算RGB直方图
	std::vector<std::vector<int>> Balance_Transformations();    // 直方图均衡化
    bool HistogramMatching(CImageProc& targetImageProc); // 直方图规格化

    //风格变换
    void ApplyBlackAndWhiteStyle();// 黑白风格
	void ApplyVintageStyle();  // 复古风格

    //添加噪声
    void AddSaltAndPepperNoise(double noiseRatio, double saltRatio = 0.5); // 添加椒盐噪声，默认噪声比例为5%
    void AddImpulseNoise(double noiseRatio = 0.05, BYTE noiseValue1 = 0, BYTE noiseValue2 = 255); // 添加脉冲噪声
    void AddGaussianNoise(double mean = 0.0, double sigma = 30.0); // 添加高斯噪声
    void AddGaussianWhiteNoise(double sigma = 30.0); // 添加高斯白噪声

	//空域滤波
    BYTE ProcessKernel(int x, int y, int c, int radius, FilterType type);
    void MeanFilter(int filterSize);
    void MedianFilter(int filterSize);
    void MaxFilter(int filterSize);
    int CalculatePitch(int width);
    void ApplyMeanFilter(); // 均值滤波

    //边缘检测
    void ApplySobelEdgeDetection();// Sobel算子边缘检测
    void ApplyPrewittEdgeDetection();// Prewitt算子边缘检测
    void ApplyRobertEdgeDetection();// Robert算子边缘检测
    void ApplyLaplaceEdgeDetection();// Laplace算子边缘检测
    void ApplyCannyEdgeDetection(); // Canny边缘检测
    void ApplyLoGEdgeDetection(); // LoG边缘检测

    // 图像操作
    void Add(CImageProc& img, double weight1, double weight2); // 图像相加
    void Multiply(CImageProc& img);    // 图像相乘
    void PowerTransform(double gamma); // 幂律变换

    // 快速傅里叶变换
    bool IsFFTPerformed() const { return m_bFFTPerformed; }
    bool FFT2D(bool bForward = true, bool bSaveState = true); // true=FFT, false=IFFT
    void DisplayFFTResult(CDC* pDC, int xOffset = 0, int yOffset = 0,
        int destWidth = -1, int destHeight = -1);
    void CImageProc::FFT1D(std::complex<double>* data, int n, int direction); //一维FFT
	void CImageProc::BitReverse(std::complex<double>* data, int n); // 位反转重排
    void SaveCurrentState();  // 保存当前状态
    //void RestoreState();      // 恢复保存的状态
    bool HasFFTData() const { return m_bFFTPerformed; }
	bool RestoreState(); // 恢复保存的状态
	void ApplyFFTLogTransform(double logBase = 10.0, double scaleFactor = 1.0); // FFT对数变换
private:
    std::vector<std::complex<double>> m_fftData; // 存储频域数据
    bool m_bFFTPerformed = false;
    std::vector<BYTE> m_originalPixels;  // 保存原始像素数据
    bool m_bStateSaved;                  // 状态保存标志

    void FFTShift(); // 频谱移中
    void CalculateFFT(bool bForward);
public:
    
    BYTE* pDib;
    BITMAPFILEHEADER* pBFH;
    BITMAPINFOHEADER* pBIH;
    RGBQUAD* pQUAD;
    BYTE* pBits;
    int nWidth;
    int nHeight;
    int nBitCount;
    bool m_bIs565Format;
    bool isPaletteDarkToLight;
    HANDLE m_hDib;

    void GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x, int y, CDC* pDC);
    void GetColor4bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor8bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor16bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor24bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor32bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);

public:
    // 深拷贝赋值运算符
    CImageProc& operator=(const CImageProc& other);
};
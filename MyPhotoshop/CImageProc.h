
#pragma once
#include "pch.h"
#include <vector>
#include <complex>

enum class FilterType {
    Mean,
    Median,
    Max,
};

enum class HighPassFilterType {
    IdealHighPass,
    ButterworthHighPass
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
    BYTE* GetPixelPtr(int x, int y);
    void DisplayColor(CClientDC* pDC,int imgX, int imgY, int x, int y);
    void GetColor(int x, int y, BYTE& red, BYTE& green, BYTE& blue);
    void SetColor(BYTE* pixel, int x, int y, BYTE r, BYTE g, BYTE b);
    void ResetFFTState();  // 新增方法声明
    //灰度处理
	std::vector<int> CalculateHistogramMix(); // 计算灰度直方图
    std::vector<std::vector<int>> CalculateHistogramRGB();// 计算RGB直方图
	std::vector<std::vector<int>> Balance_Transformations();    // 直方图均衡化
    void HistogramMatching(CImageProc& targetImageProc); // 直方图规格化

    //风格变换
    void ApplyBlackAndWhiteStyle();// 黑白风格
	void ApplyVintageStyle();  // 复古风格

    //添加噪声
    void AddSaltAndPepperNoise(double noiseRatio, double saltRatio = 0.5); // 添加椒盐噪声，默认噪声比例为5%
    void AddImpulseNoise(double noiseRatio = 0.05, BYTE noiseValue1 = 0, BYTE noiseValue2 = 255); // 添加脉冲噪声
    void AddGaussianNoise(double mean = 0.0, double sigma = 30.0); // 添加高斯噪声
    void AddGaussianWhiteNoise(double sigma = 30.0); // 添加高斯白噪声

	//空域滤波
    void SpatialFilter(int filterSize, FilterType type);

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

	// 频域滤波
    void FreqPassFilter(double D0, int step, int filterType);  //高通/低通滤波
    void HomomorphicFiltering();    //同态滤波

    // 图像编码与解码
    bool HuffmanEncodeImage(const CString& savePath);
    bool HuffmanDecodeImage(const CString& openPath);
    
    // LZW编码与解码
    bool LZWEncodeImage(const CString& savePath);
    bool LZWDecodeImage(const CString& openPath);

    // 基于余弦变换和量化的编码与解码
    bool CosineEncodeImage(const CString& savePath);
    bool CosineDecodeImage(const CString& openPath);
    void DCT2D(double block[8][8]);
    void IDCT2D(double block[8][8]);
    void Quantize(double block[8][8]);
    void Dequantize(double block[8][8]);

    //综合编码与解码函数
    bool ComprehensiveEncodeImage(const CString& savePath);
    bool ComprehensiveDecodeImage(const CString& openPath);

    // 快速傅里叶变换
    bool IsFFTPerformed() const { return m_bFFTPerformed; }
    bool FFT2D(bool bForward = true, bool bSaveState = true); // true=FFT, false=IFFT
    bool IFFT2D(bool bSaveState = true);

	// FFT与IFFT显示
    void ShowSpectrumDialog(CWnd* pParent);
    void DisplayFullSpectrum(CDC* pDC, int xOffset = 0, int yOffset = 0,
        int destWidth = -1, int destHeight = -1);
    void CImageProc::DisplayIFFTResult(CDC* pDC, int xOffset, int yOffset, int destWidth, int destHeight);

    // 新增方法：获取/设置复数频谱数据
    const std::vector<std::complex<double>>& GetFFTData() const { return m_fftData; }
    void SetFFTData(const std::vector<std::complex<double>>& data, int w, int h);
    void CImageProc::FFT1D(std::complex<double>* data, int n, int direction); //一维FFT
	void CImageProc::BitReverse(std::complex<double>* data, int n); // 位反转重排
	int CImageProc::FindTargetBit(int i, int n); // 查找目标位
    void SaveCurrentState();  // 保存当前状态
    bool HasFFTData() const { return m_bFFTPerformed; }
	bool RestoreState(); // 恢复保存的状态

private:
    std::vector<std::complex<double>> m_fftData; // 存储频域数据
    std::vector<std::complex<double>> m_fftDataCopy; // 存储FFT结果的副本
    bool m_bFFTPerformed = false;
    std::vector<BYTE> m_originalPixels;  // 保存原始像素数据
    bool m_bStateSaved;                  // 状态保存标志
    std::vector<std::complex<double>> m_originalFFTData; // 保存原始FFT数据
    void FFTShift(std::complex<double>* data, int w, int h); // 频谱移中

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
    bool m_bIFFTPerformed;
    HANDLE m_hDib;
    
    //获取像素颜色
    void GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor4bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor8bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor16bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor24bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor32bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);

public:
    // 深拷贝赋值运算符
    CImageProc& operator=(const CImageProc& other);

    // 新增成员变量
    std::vector<BYTE> m_originalImageData;  // 保存原始图像数据
    std::vector<std::complex<double>> m_fftDisplayData; // 保存FFT显示数据
    std::vector<BYTE> m_ifftResult;        // 保存IFFT结果

    // 新增方法
    bool HasOriginalImageData() const;
    bool HasIFFTResult() const;
    void DisplayOriginalImage(CDC* pDC, int xOffset, int yOffset, int destWidth, int destHeight);
public:
    // 计算大于等于n的最小2的幂次
    int nextPowerOfTwo(int n) const {
        if (n <= 0) return 1;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        return n + 1;
    }

    // 检查是否为2的幂次
    bool isPowerOfTwo(int n) const {
        return (n & (n - 1)) == 0;
    }

public:
    int m_fftWidth = 0;
    int m_fftHeight = 0;
    std::vector<std::complex<double>> m_fullSpectrum;  // 存储完整频谱数据
    std::pair<int, int> m_paddedSize;
    std::vector<std::vector<std::complex<double>>> m_fullSpectrumRGB; // 存储RGB三个通道的频谱
};

// 霍夫曼树节点结构体
struct HuffmanNode {
    BYTE value;
    int freq;
    HuffmanNode* left;
    HuffmanNode* right;
    HuffmanNode(BYTE v, int f) : value(v), freq(f), left(nullptr), right(nullptr) {}
    HuffmanNode(HuffmanNode* l, HuffmanNode* r) : value(0), freq(l->freq + r->freq), left(l), right(r) {}
};

// 比较器
struct CompareNode {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->freq > b->freq;
    }
};

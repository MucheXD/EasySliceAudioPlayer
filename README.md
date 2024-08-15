# EasySliceAudioPlayer

![image](https://github.com/user-attachments/assets/a6d59d56-1c1a-42fb-849b-d96624824657)

本程序将自动分析打开的高考听力音频文件，并为您提供合适的切分，使您可以快速定位并跳转到指定题目。

**本程序使用[FFMpeg](https://ffmpeg.org/)实现部分功能，使用前请确保您已经安装FFMpeg，或者程序运行目录存在[ffmpeg.exe](https://www.gyan.dev/ffmpeg/builds/ffmpeg-git-full.7z)**。

## 适用范围

适用于普通高等学校招生全国统一考试的听力文件（MP3格式），亦适用于大部分地方模拟考试的听力文件（MP3格式）。

## 使用方法

1. 程序**无法**直接打开，请将文件拖拽到程序图标上来打开文件，或者在要打开的文件上右击，选择打开方式为本程序。
  
2. 程序控制面板的三个按键分别是 播放/暂停、后退5秒、前进5秒。
  
3. 如果程序检测到打开的文件为英语听力（或类似文件），为自动执行分析，并将结果呈现在进度条上方。你可以点击分段来快速跳转。
  
## 注意事项

- 如果有两个程序实例同时进行音频分析，将可能导致崩溃，请保证每次仅存在一个程序实例。

- 本程序仅支持MP3格式的文件，其它格式可能可以使用，但效果不受保证。

## 执行原理

本程序通过调用FFMpeg的`aspectralstats`过滤器分析音频中的提示音，并结合音频中的停顿来切分音频。

## 制作与版权

+ 代码逻辑/UI界面: MucheXD 100%

+ 部分图标提供方: [iconfont](https://www.iconfont.cn/)

+ 字体:
  - [思源黑体 CN VF](https://github.com/adobe-fonts/source-han-sans)
  - [Fira Code](https://github.com/tonsky/FiraCode)
 
+ 使用库/框架：[QT 6.7.2](https://www.qt.io/)

+ 使用外部依赖：[FFMpeg](https://ffmpeg.org/)

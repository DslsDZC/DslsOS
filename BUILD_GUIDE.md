# DslsOS 构建指南

## 问题解决方案

### 解决 CMAKE_CXX_COMPILER 错误

原始错误信息：
```
CMake Error: CMAKE_CXX_COMPILER not set, after EnableLanguage
```

这个错误是因为项目不需要C++，但CMakeLists.txt中包含了C++语言。我已经修复了这个问题。

## 快速构建

### Windows 系统

1. **确保安装了 Visual Studio 2022**
   - 下载地址: https://visualstudio.microsoft.com/
   - 选择"使用C++的桌面开发"工作负载

2. **运行构建脚本**
   ```cmd
   build.bat
   ```

3. **手动构建（如果脚本失败）**
   ```cmd
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Release
   ```

### Linux 系统

1. **安装依赖**
   ```bash
   sudo apt-get update
   sudo apt-get install build-essential cmake
   ```

2. **运行构建脚本**
   ```bash
   ./build.sh
   ```

3. **手动构建（如果脚本失败）**
   ```bash
   mkdir build
   cd build
   cmake ..
   make -j$(nproc)
   ```

## 验证构建

### 编译测试程序
```bash
# Windows
cl test_build.c
.\test_build.exe

# Linux
gcc test_build.c -o test_build
./test_build
```

### 预期输出
```
DslsOS Build Test
==================
Build system is working correctly!
All components are ready for compilation.

To build the full system:
  - Windows: build.bat
  - Linux:   ./build.sh

Test completed successfully.
```

## 文件结构

```
DslsOS/
├── CMakeLists.txt          # 主构建配置（已修复）
├── build.bat              # Windows构建脚本
├── build.sh               # Linux构建脚本
├── test_build.c           # 构建测试程序
├── include/               # 头文件
├── kernel/                # 内核代码
└── src/                   # 用户空间代码
```

## 常见问题

### 1. CMake找不到编译器
**问题**: `CMAKE_CXX_COMPILER not set`
**解决**: 已经修复，现在只使用C语言

### 2. Windows上缺少Visual Studio
**解决**: 安装Visual Studio 2022或2019，包含C++工具链

### 3. Linux上缺少编译工具
**解决**: 安装build-essential包
```bash
sudo apt-get install build-essential
```

### 4. 构建失败
**检查**:
- 所有文件是否存在
- 编译器是否正确安装
- CMake版本是否>=3.16

## 下一步

如果构建测试成功，您就可以构建完整的DslsOS系统了：

```bash
# Windows
build.bat

# Linux
./build.sh
```

构建完成后，可执行文件将位于：
- Windows: `build/bin/Release/dslsos.exe`
- Linux: `build/bin/dslsos`

## 获取帮助

如果仍有问题，请检查：
1. 开发环境设置是否正确
2. 所有依赖是否已安装
3. 编译器路径是否正确配置

---

*构建指南 v1.0 - DslsOS项目*
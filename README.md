# GeometryArtist

个人用的几何可视化与编辑工具。

## 特性

- **现代化界面**：基于`Qt6`的跨平台GUI
- **几何核心**：支持`CGAL`和`OCCT`双后端（规划中）
- **渲染**：OpenGL 4.3（可能maybe会玩玩vulkan？）
- **网格处理**：布尔运算、简化、细分、修复等算法
- **稳定性**：Windows崩溃转储自动收集

## 快速开始

### 环境要求

- **CMake** 3.20+
- **Qt** 6.0+
- **C++20**

### Windows构建

```powershell
# 1. 克隆项目
git clone https://github.com/AstroJoke/GeometryArtist.git
cd GeometryArtist

# 2. 配置项目
mkdir build
cd build

# 根据你的环境配置
cmake .. -G "Visual Studio 18 2026" -A x64

# 3. 构建 (在VS中打开GeometryArtist.sln(x))
```

## 依赖管理

### 必需依赖

- **Qt6**: Core, Widgets, OpenGL, OpenGLWidgets, Xml, PrintSupport
- **OpenGL**: 4.3+
- **CGAL**: 计算几何算法库

### 可选后端（规划中）

- **OCCT**: 开源CAD内核

## 许可证

本项目采用 **GNU General Public License v3.0** - 查看 [LICENSE](LICENSE) 文件了解详情。

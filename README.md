# RHEA Baremetal

RHEA Baremetal 是一个嵌入式系统裸机开发项目，提供了完整的硬件抽象层和丰富的示例应用。该项目支持多种硬件外设，采用模块化设计，并使用Modern CMake构建系统。

## 目录结构

```
rhea-baremetal/
├── apps/                    # 应用程序目录
│   ├── hello/
│   ├── freertos/
│   ├── axi_dma/
│   ├── cipher/
│   ├── pcie_ep/
│   ├── pcie_rc/
│   └── ...
│
├── hw/                     # 硬件抽象层
│   ├── cpu/               # - CPU架构支持
│   │   ├── aarch64/      #   · ARM 64位架构
│   │   └── ax65/         #   · RISCV 64位架构
│   ├── uart/             # - UART控制器驱动
│   ├── gpio/             # - GPIO控制器驱动
│   ├── spi/              # - SPI控制器驱动
│   ├── dma/              # - DMA控制器驱动
│   ├── pcie/             # - PCIe接口驱动
│   ├── mmc/              # - MMC/SD卡控制器
│   ├── timer/            # - 定时器驱动
│   ├── wdt/              # - 看门狗驱动
│   ├── gic/              # - ARM GIC中断控制器
│   └── mailbox/          # - 邮箱通信驱动
│
├── libs/                  # 公共库
│   ├── freertos/         # - FreeRTOS内核
│   ├── arm_gnu/          # - ARM GNU工具链支持
│   ├── crc/              # - CRC校验库
│   ├── flash/            # - Flash存储支持
│   │   ├── nand/        #   · NAND Flash
│   │   └── nor/         #   · NOR Flash
│   ├── xmodem/           # - Xmodem协议支持
│   └── newlib_stubs/     # - newlib C库支持
│
├── common/                # 通用代码
├── cmake_helper/          # CMake构建辅助文件
├── docs/                  # 文档
└── tools/                # 工具脚本
```

## 构建系统

项目使用CMake作为构建系统，每个应用都有独立的构建目录。主要构建文件：

- `CMakeLists.txt`: 主构建配置文件
- `targets.cmake`: 目标配置文件
- `build_all.sh`: 构建所有目标的脚本

## Requirements

| Program          | Min supported version |
| ---------------- | --------------------- |
| Arm GNU Compiler | 13.2.rel1             |
| Clang/LLVM       | 18.1.7                |
| CMake            | 3.15                  |
| GNU make         | 3.81                  |
| Ninja            | 1.12.1                |

## Build Options

只提及工程配置相关options, 通过CMake Cache传入, 不包含cmake options(如-S, -B, -G...)

- BOARD: VIRT, RTL, FPGA, EVB. VIRT为qemu virt平台.
- CMAKE_BUILD_TYPE: Debug, Release, RelWithDebInfo, MinSizeRel. 默认为Release.

- MEM_SCHEME: ROM, SRAM, DRAM, CUSTOM

- PROJECT_NAME: 设置工程名字(生成的目标文件名字前缀)

- APP_CMAKE: 设置app.cmake路径

- ...

可以通过ccmake和cmake-gui查看和修改build options, 如下：

![](docs/images/ccmake.png)

## Usage

为了代码复用，提高开发效率，Rhea BareMetal使用模块化思想搭建了整改工程，简单来说该工程就是一个模块集合(这里的模块，在cmake术语中称其为target，很多情况下也可称为静态库，三者在本文中等价)

可以通过--graphviz选项在config阶段生成targets的依赖关系图，如下：

![](docs/images/dep.png)

### 对于App开发者

对于app新手，推荐按照如下步骤入手

1. 尝试直接编译和运行apps/hello这个示例工程

2. 通过ccmake查看和尝试各个build options(cmake cache)

3. 新建app工程
   
   1. 如何调用模块，添加app.c和头文件路径，请查看apps/hello/app.cmake

### 对于模块开发者

总的来说只需要两步：

1. 封装好自己的模块

2. 放到Rhea BareMetal模块集合中

#### 封装好自己的模块

1. 准备好模块源代码。硬件驱动应放在hw/，软件库放在libs/

2. 告诉Rhea BareMetal模块的源文件，头文件路径，依赖的模块，编译参数...
   
   1. 首先需要明白一个模块的**2个组成部分**：
      
      1. Interface - 提供给使用者的
      
      2. Private   - 模块内部使用的
   
   2. 一个封装良好的模块，应该尽可能吝啬的给出必须的Interface
   
   3. 可以参考libs/flash/nand/CMakeLists.txt
   
   ![](docs/images/how_to_write_a_target_cmake.png)

#### 放到Rhea BareMetal模块集合中

见targets.cmake，该文件存放了Rhea BareMetal所有模块

![](docs/images/targets.png)

## Note

1. build_all.sh会运行所有apps/目录下的build.sh脚本，上传代码前请运行build_all.sh，检查所有工程是否正常运行

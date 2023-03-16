# 指定运行此配置文件所需的CMake最低版本
cmake_minimum_required(VERSION 3.25)

project(FTPClient
  LANGUAGES  CXX
)

# 自动添加BINARY和SOURCE到当前CMakeLists.txt
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 规定为C++14标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出编译指令，方便调试
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 针对MacOS和IOS的特殊架构
set(CMAKE_OSX_ARCHITECTURES "x86_64")

# 开启全局UIC
set(CMAKE_AUTOUIC ON)
# 开启全局MOC
set(CMAKE_AUTOMOC ON)

# Qt
message("正在寻找Qt...")
find_package(Qt5 REQUIRED Widgets)
if (${Qt5_FOUND})
  message("找到 Qt " ${Qt5_VERSION})
else()
  message("找不到Qt")
endif()

# 查找指定package
find_package(Qt5Widgets)
find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Network)

# 分类放入list
FILE(GLOB_RECURSE SRC_FILES "./src/*.cpp")
FILE(GLOB_RECURSE HEAD_FILES "./src/*.h")
FILE(GLOB_RECURSE UI_FILES "./src/*.ui")

# 添加resource文件
qt5_add_resources(QRC_FTP_FILES "./src/ftp.qrc")

# 划分分组
#source_group("Src Files" FILES ${SRC_FILES})
source_group("Head Files" FILES ${HEAD_FILES})
source_group("Form Files" FILES ${UI_FILES})

# 生成指定文件
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEAD_FILES} ${UI_FILES} ${QRC_FTP_FILES})

# 链接库
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Network)

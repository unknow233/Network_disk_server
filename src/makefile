# 目标名和路径配置
TARGET=server
CC=g++
INCLUDE_PATH=../include
INSTALL_PATH=/usr/bin/
LIBRARY_PATH=
LIBRARY=-lpthread -lmysqlclient

# 编译选项
CFLAGS=-I$(INCLUDE_PATH) -c -g -Wall
CPPFLAGS=-I$(INCLUDE_PATH) -c -pipe -g -std=c++17 -Wall -W -fPIC

# 工具命令
RM=sudo rm -rf
COPY=sudo cp

# 源文件和目标文件
SFILE=$(wildcard *.cpp)
OBJ_DIR=obj
DEP_DIR=dep
DFILE=$(addprefix $(OBJ_DIR)/, $(patsubst %.cpp,%.o,$(SFILE)))
DEPFILES=$(addprefix $(DEP_DIR)/, $(patsubst %.cpp,%.d,$(SFILE)))

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(DFILE)
	$(CC) $^ -o $@ $(LIBRARY)

# 编译规则：从 .cpp 生成 .o 和 .d 文件
$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CPPFLAGS) -MMD -MP $< -o $@
	mv $(OBJ_DIR)/$*.d $(DEP_DIR)/$*.d

# 包含依赖文件
-include $(DEPFILES)

# 清理目标
clean:
	$(RM) $(OBJ_DIR) $(DEP_DIR) $(TARGET)

# 输出安装路径
output:
	echo $(INSTALL_PATH)$(TARGET)

# 安装目标
install: $(TARGET)
	$(COPY) $(TARGET) $(INSTALL_PATH)

# 彻底清理（包括安装的目标文件）
distclean: clean
	$(RM) $(INSTALL_PATH)$(TARGET)
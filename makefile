# 目标名和路径配置
TARGET=server
CC=g++
INCLUDE_PATH=./include
INSTALL_PATH=/usr/bin/
LIBRARY_PATH=
LIBRARY=-lpthread -lmysqlclient

# 编译选项
CFLAGS=-I$(INCLUDE_PATH) -c -g -Wall
CPPFLAGS=-I$(INCLUDE_PATH) -c -pipe -g -std=gnu++11 -Wall -W -fPIC

# 工具命令
RM=sudo rm -rf
COPY=sudo cp

# 源文件和目标文件
SFILE=$(wildcard src/*.cpp)
OBJ_DIR=obj
DEP_DIR=dep

DFILE=$(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SFILE))
DEPFILES=$(patsubst src/%.cpp,$(DEP_DIR)/%.d,$(SFILE))
# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(DFILE)
	$(CC) $^ -o $@ $(LIBRARY)

# 编译规则：从 .cpp 生成 .o 和 .d 文件
$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -MMD -MP $< -o $@
	@mv $(basename $@).d $(DEP_DIR)/$(notdir $(basename $@)).d
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
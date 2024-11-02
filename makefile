# 定義變數
CXX = g++
# CXXFLAGS = -Wall -O2
LDFLAGS = -lm              # 需要的話可添加其他庫
SRC_DIR = src
TEST_DIR = test
OBJ_DIR = obj
BIN_DIR = bin

# 只選取 test 資料夾下的 .cpp 檔案
SRCS = $(wildcard $(TEST_DIR)/*.cpp)

# 為每個 .cpp 文件生成對應的可執行檔案名稱
BINS = $(patsubst $(TEST_DIR)/%.cpp, $(BIN_DIR)/%, $(SRCS))

# 預設目標：編譯所有可執行檔案
all: $(BINS)

# 為每個 .cpp 文件生成對應的可執行檔案
$(BIN_DIR)/%: $(TEST_DIR)/%.cpp | $(BIN_DIR) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# 創建目錄以存放 .o 檔案和可執行檔案
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# 清理編譯產生的檔案
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# 運行特定可執行檔案，例如 `make run file=bmp_load_file`
run: $(BIN_DIR)/$(file)
	./$(BIN_DIR)/$(file)

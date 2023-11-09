CXXFLAGS = -Wall -Wextra -Wno-error=format-security
TARGET=DbusSmsForwardCPlus
SRCS = main.cpp mail.cpp
LDFLAGS = -ldbus-1 -lssl -lcrypto -lcurl -lpthread
INC_DIRS = -I$(STAGING_DIR)/usr/include/dbus-1.0 -I$(STAGING_DIR)/usr/lib/dbus-1.0/include -I$(STAGING_DIR)/usr/include
LIB_DIRS = -L$(STAGING_DIR)/usr/lib

# 生成目标文件
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INC_DIRS) $(LIB_DIRS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIRS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)


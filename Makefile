CC = gcc   #代表所使用的编译器

TOP = $(CURDIR)

SRC_DIR = $(TOP)

src = $(wildcard $(SRC_DIR)/*.c)

INCLUDE_PATH = -I $(TOP)
		
LIBS = -lpthread -lrt            #动态库

OUTPUT = rtcp   #输出程序名称

OBJ = $(src:.c = .o)

$(warning $(src))

all:$(OUTPUT)

$(OUTPUT) : $(OBJ)
		$(CC) $^ -o $@ $(INCLUDE_PATH) $(LIBS) 

%.o : %.c
		$(CC) -c $< 
		
.PHONY: clean
clean:	
		rm  $(OUTPUT)

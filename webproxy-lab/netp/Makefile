# ===== 변수 정의 =====
# Makefile에서 변수는 $(변수명)으로 참조. 반복되는 값을 한 곳에서 관리하려는 목적.

CC = gcc
# 컴파일러 지정. 다른 컴파일러 쓰려면 여기만 바꾸면 됨 (예: clang, arm-linux-gnueabi-gcc).

CFLAGS = -O0 -Wall -I . -g
# 컴파일 플래그 모음. 각각 의미:
#   -O0  : 최적화 수준 0 (최적화 안 함). 디버깅에 유리 — 코드 순서가 안 바뀜.
#   -Wall: 모든 경고 켜기 (warning all). 코드 품질 체크에 필수.
#   -I . : include 검색 경로에 현재 디렉토리(.) 추가. "#include "csapp.h"" 찾을 때 여기도 봄.
#   -g   : 디버깅 심볼 포함. gdb로 디버깅할 때 변수명, 줄번호 보임.

# CFLAGS = -O2 -Wall -I . -g
# 주석 처리된 대안. -O2는 최적화 레벨 2 (실제 배포용). 
# 학습용에서는 -O0이 나아서 위 줄이 활성, 이 줄은 비활성.

LIB = -lpthread
# 링크 플래그. -l은 "라이브러리 링크". pthread는 POSIX 스레드 라이브러리.
# csapp.c가 pthread 함수(Pthread_create 등)를 쓰기 때문에 필요.
# 없으면 "undefined reference to pthread_create" 에러 남.


# ===== 타겟 규칙 =====
# 규칙 형식:
#   타겟: 의존파일들
#   <탭>명령어
#
# 의미: "타겟 파일을 만들려면 이 의존파일들이 필요하고, 이 명령으로 만든다"

all: echoclient echoserveri hostinfo
# 'all'은 관례적 기본 타겟. 'make'만 치면 첫 번째 타겟이 실행되는데, 관례상 all을 최상단에 둠.
# all 자체는 파일이 아닌 "가상 타겟" (phony target). 실제 파일 안 만듦.
# 단지 "echoclient와 echoserveri를 빌드하라"는 의미.


echoclient: echoclient.c csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.c csapp.o $(LIB)
# "echoclient라는 실행 파일은 echoclient.c와 csapp.o가 있어야 만들 수 있다"
# 
# 명령어 해석:
#   gcc -O0 -Wall -I . -g -o echoclient echoclient.c csapp.o -lpthread
#        └────────변수 전개───┘   └─출력 이름─┘ └──소스/객체 파일들──┘ └라이브러리┘
#
#   -o echoclient : 결과물 이름을 "echoclient"로
#   echoclient.c csapp.o : 이 둘을 컴파일+링크해서 실행 파일 생성


hostinfo: hostinfo.c csapp.o
	$(CC) $(CFLAGS) -o hostinfo hostinfo.c csapp.o $(LIB)


echoserveri: echoserveri.c csapp.o echo.o
	$(CC) $(CFLAGS) -o echoserveri echoserveri.c csapp.o echo.o $(LIB)
# 서버는 echo 함수를 호출하므로 echo.o도 링크 필요.
# echoclient와 구조 같지만 echo.o가 추가됨.

echo.o: echo.c 
	$(CC) $(CFLAGS) -c echo.c
# "echo.o는 echo.c에서 만들어진다"
#
# -c 플래그가 핵심:
#   -c : 컴파일만, 링크 안 함. 결과물은 오브젝트 파일(.o).
#   -o 없이도 echo.c를 -c 하면 echo.o가 기본 이름으로 생성됨.
#
# echo.c에 main이 없으므로 직접 실행 파일 못 만듦. 그래서 .o로 두고, 
# 나중에 echoserveri 만들 때 링크에 포함됨.


csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c
# csapp 라이브러리도 같은 방식. .o로 만들어두고 여러 실행 파일이 재사용.
# 한 번 컴파일해두면 echoclient와 echoserveri 빌드 시 각각 재활용돼서 효율적.


# ===== 청소 규칙 =====
clean:
	rm -f *.o echoclient echoserveri hostinfo *~
# 'make clean' 실행 시 호출. 빌드 산출물 제거.
#   *.o         : 모든 오브젝트 파일 (csapp.o, echo.o)
#   echoclient  : 실행 파일
#   echoserveri : 실행 파일  
#   *~          : 편집기(emacs 등)가 만드는 백업 파일
# 
# rm -f 의 -f 플래그: 파일이 없어도 에러 없이 통과. clean은 "있으면 지우자"라서 -f가 안전.
# Web Proxy Lab

CS:APP Web Proxy Lab — HTTP 프록시 서버를 밑바닥부터 구현하며 네트워크 프로그래밍, 동시성, 캐싱을 학습합니다.

---

## 📚 학습 목표

### 네트워크 기초
- [x] 네트워크 개념 & TCP/IP 계층 이해
- [x] 클라이언트-서버 모델 이해
- [x] Datagram Socket vs Stream Socket 차이 설명
- [x] 파일 디스크립터(fd) 개념 체화

### 소켓 인터페이스
- [x] `socket()` — 소켓 생성, fd 반환
- [x] `bind()` — 주소(IP+포트) 부여 (서버 전용)
- [x] `listen()` — 연결 대기 모드 전환 (서버 전용)
- [x] `accept()` — 연결 수락, 새 fd 반환 (서버 전용)
- [x] `connect()` — 서버 접속 (클라이언트 전용)
- [x] `close()` — 소켓 종료, 자원 회수

### 웹서버 & HTTP
- [x] HTTP 요청/응답 메시지 구조 이해 (시작 라인 + 헤더 + 빈 줄 + 바디)
- [x] HTTP 메서드 (GET, HEAD, POST) 구분
- [x] HTTP 상태 코드 (2xx/4xx/5xx) 의미
- [x] MIME 타입 개념
- [x] 정적 컨텐츠 vs 동적 컨텐츠
- [x] CGI 동작 원리 (fork + dup2 + execve)

### 프록시 서버
- [x] Forward Proxy vs Reverse Proxy 차이
- [x] 프록시가 "서버이자 클라이언트"인 이유
- [ ] 요청 헤더 재작성 (Host, User-Agent, Connection, Proxy-Connection)
- [x] HTTP/1.1 → HTTP/1.0 변환 의미

---

## 🎯 실습 체크리스트

### Part 0: 환경 세팅

- [x] Docker 또는 Ubuntu 22.04 환경 준비
- [x] `webproxy-lab` 레포 clone
- [x] 학습용 디렉토리 생성 (`netp/`)
- [x] csapp.c, csapp.h 학습용 디렉토리로 복사
- [x] VSCode IntelliSense 설정 (`_GNU_SOURCE` 매크로 추가)
- [x] 포트 할당 도구 동작 확인 (`./port-for-user.pl`, `./free-port.sh`)

### Part 1: Echo Server/Client (소켓 기초)

- [x] CSAPP 11.1 ~ 11.4 읽기
- [x] `echoclient.c` 책 따라 작성 (CSAPP Fig 11.20)
- [x] `echoserveri.c` 책 따라 작성 (CSAPP Fig 11.21)
- [x] `echo.c` 책 따라 작성 (CSAPP Fig 11.22)
- [x] Makefile 작성 (타겟, 의존성, 컴파일 플래그 이해)
- [x] 빌드 성공 (`make clean && make`)
- [x] 두 터미널로 에코 통신 동작 확인
- [x] (선택) `hostinfo.c` 작성해서 getaddrinfo 체화

### Part 2: Tiny Web Server

- [x] CSAPP 11.5 (웹 서버) 읽기
- [x] `tiny.c` 함수 하나씩 구현 (책 Figure 11.29 ~ 11.35)
  - [x] `clienterror` — 에러 응답
  - [x] `read_requesthdrs` — 헤더 읽기
  - [x] `parse_uri` — URI 파싱
  - [x] `get_filetype` — MIME 타입 매핑
  - [x] `serve_static` — 정적 컨텐츠
  - [x] `serve_dynamic` — CGI 실행
  - [x] `doit` — 요청 처리 본체
- [x] `./tiny <port>` 빌드 및 실행
- [x] 브라우저로 `home.html` 접근 성공
- [x] 이미지 파일 (`godzilla.gif/jpg`) 렌더링 확인
- [x] CGI 동작 확인 (`/cgi-bin/adder?15213&18213`)
- [x] curl로 요청/응답 바이트 관찰 (`curl -v`)
- [x] netcat으로 수동 HTTP 요청 보내기
- [x] 에러 케이스 테스트 (존재하지 않는 파일, 미지원 메서드)

### Part 3: CSAPP Homework (숙제 3문제 이상)

- [x] **11.6c** — MPG 비디오 타입 지원 추가 -> 브라우저가 mpeg-1 지원 안해서 mp4로 넣어서 확인
- [x] **11.7** — HEAD 메서드 지원 추가
- [x] **11.9** — `mmap` 대신 `malloc + rio_readn` 사용
- [x] **11.10** — POST 메서드 지원
- [x] **11.11** — Path Traversal 방어

### Part 4: Proxy — Sequential (순차 처리)

- [x] Proxylab PDF 정독
- [ ] `proxy.c` 구조 설계 (어떤 함수로 나눌지)
- [ ] 리스닝 소켓 열기 (`Open_listenfd`)
- [ ] accept 루프 구현
- [ ] 클라이언트 요청 라인 파싱
- [ ] URI에서 host/port/path 분리
- [ ] 요청 헤더 읽기 및 재작성
  - [ ] Host 헤더 처리
  - [ ] User-Agent 고정 문자열로 치환
  - [ ] Connection: close 설정
  - [ ] Proxy-Connection: close 설정
  - [ ] 나머지 헤더는 그대로 전달
- [ ] HTTP/1.1 → HTTP/1.0 변환
- [ ] 타겟 서버 접속 (`Open_clientfd`)
- [ ] 요청 전달
- [ ] 응답 수신 및 클라이언트로 전달 (바이너리 데이터 주의)
- [ ] curl로 테스트 (`curl -v --proxy http://localhost:PORT http://...`)
- [ ] 브라우저 프록시 설정으로 실사이트 테스트
- [ ] `./driver.sh` Basic 테스트 통과

### Part 5: Proxy — Concurrency (동시성)

- [ ] CSAPP 12장 (동시 프로그래밍) 읽기
- [ ] `pthread_create` + `pthread_detach` 패턴 이해
- [ ] `connfd`를 힙에 복사하여 스레드에 전달 (race condition 방지)
- [ ] 멀티스레드 요청 처리 구현
- [ ] SIGPIPE 무시 처리 (`Signal(SIGPIPE, SIG_IGN)`)
- [ ] EPIPE, ECONNRESET 에러 graceful 처리
- [ ] 메모리 누수 없음 (valgrind 확인 권장)
- [ ] fd 누수 없음 (모든 경로에서 close)
- [ ] `./driver.sh` Concurrency 테스트 통과

### Part 6: Proxy — Cache (캐시)

- [ ] CSAPP 12.5 (스레드 동기화) 읽기
- [ ] 캐시 자료구조 설계 (연결 리스트 권장)
- [ ] MAX_CACHE_SIZE (1MiB), MAX_OBJECT_SIZE (100KiB) 상수 준수
- [ ] 캐시 key 정의 (URI 기반)
- [ ] `cache_get` — 캐시 조회
- [ ] `cache_put` — 캐시 저장
- [ ] LRU (혹은 근사 LRU) eviction 정책
- [ ] 응답 전달 중 캐시 버퍼에 누적
- [ ] MAX_OBJECT_SIZE 초과 시 캐시 포기 (전달은 계속)
- [ ] Readers-Writers 동기화 구현
  - [ ] `pthread_rwlock` 사용 또는 세마포어 직접 구현
  - [ ] 다중 reader 동시 접근 허용
  - [ ] writer는 단독 접근
- [ ] (선택) 모듈 분리 (`cache.c`, `cache.h`)
- [ ] `./driver.sh` Cache 테스트 통과

### Part 7: Robustness & Final

- [ ] 서버가 어떤 에러에도 종료되지 않음
- [ ] malformed 요청 처리
- [ ] 바이너리 데이터 전송 검증 (이미지, 비디오)
- [ ] `./driver.sh` 전체 70/70 점수
- [ ] WIL (Weekly I Learned) 작성

---

## 🛠 도구 & 레퍼런스

### 자주 쓰는 명령

```bash
# 빌드
make clean && make

# 테스트
./tiny <port>
./proxy <port>
curl -v --proxy http://localhost:<proxy_port> http://localhost:<tiny_port>/home.html
./driver.sh

# 디버깅
netstat -tn | grep <port>    # 소켓 상태 확인
lsof -i :<port>              # 포트 사용 프로세스 확인
valgrind ./proxy <port>      # 메모리 누수 체크
```

### 참고 자료

- **CSAPP 3판** 11장 (네트워크 프로그래밍), 12장 (동시 프로그래밍)
- **RFC 1945** — HTTP/1.0 명세
- **Proxylab PDF** — CMU 과제 명세
- **CSAPP 저자 코드**: [csapp.cs.cmu.edu/3e/code.html](https://csapp.cs.cmu.edu/3e/code.html)

---

## 🚨 흔한 함정

- [ ] `strlen`, `strcpy` 대신 RIO (`rio_readn`, `rio_writen`) 사용 — 바이너리 데이터 때문
- [ ] `strtok` 재진입성 없음 — 스레드에서 `strtok_r` 사용
- [ ] 전역 변수에 쓰기 — race condition 주의
- [ ] `accept` 후 `connfd` 주소 직접 전달 → 힙 복사 필수
- [ ] `close` 빠뜨림 → fd 누수 → "Too many open files"
- [ ] HTTP/1.1 keep-alive 구현 시도 → 복잡도 폭발, 포기하고 `Connection: close`
- [ ] TIME_WAIT로 인한 재시작 실패 → `SO_REUSEADDR` (csapp에 이미 있음)

---

## 📝 기술 정리 (WIL용)

- [ ] 파일 디스크립터란 무엇인가
- [ ] 소켓 API 6개의 역할과 서버/클라이언트 비대칭성
- [ ] TCP 3-way handshake와 소켓 API의 매핑
- [ ] Short count와 RIO 패키지의 존재 이유
- [ ] HTTP 프로토콜의 바이트 레벨 구조
- [ ] CGI의 `dup2` 트릭 — stdout을 소켓으로 리다이렉트
- [ ] Forward Proxy와 Reverse Proxy의 차이 — 누가 프록시의 존재를 아는가
- [ ] Readers-Writers 문제와 해결 전략
- [ ] iterative vs concurrent 서버의 성능 특성
- [ ] 프록시가 HTTP 요청 헤더를 수정하는 이유

---

# 📘 Docker + VSCode DevContainer 기반 C 개발 환경 구축 가이드 (WebProxyLab)

이 문서는 **Windows**와 **macOS** 사용자가 Docker와 VSCode DevContainer 기능을 활용하여 C 개발 및 디버깅 환경을 빠르게 구축할 수 있도록 도와줍니다.

[**주의**] 지난 주차와 다른 점만 하시려면 4장부터 7장만 보세요.
[**주의**] webproxy-lab의 경우 tiny 웹 서버와 proxy 서버 두가지를 구현해야 해서 두가지 debugging 설정을 제공합니다. 이에 대한 설명은 7장에서 설명하니 꼭 읽어보시기 바랍니다.

---

## 1. Docker란 무엇인가요?

**Docker**는 애플리케이션을 어떤 컴퓨터에서든 **동일한 환경에서 실행**할 수 있게 도와주는 **가상화 플랫폼**입니다.  

Docker는 다음 구성요소로 이루어져 있습니다:

- **Docker Engine**: 컨테이너를 실행하는 핵심 서비스
- **Docker Image**: 컨테이너 생성에 사용되는 템플릿 (레시피 📃)
- **Docker Container**: 이미지를 기반으로 생성된 실제 실행 환경 (요리 🍜)

### ✅ AWS EC2와의 차이점

| 구분 | EC2 같은 VM | Docker 컨테이너 |
|------|-------------|-----------------|
| 실행 단위 | OS 포함 전체 | 애플리케이션 단위 |
| 실행 속도 | 느림 (수십 초 이상) | 매우 빠름 (거의 즉시) |
| 리소스 사용 | 무거움 | 가벼움 |

---

## 2. VSCode DevContainer란 무엇인가요?

**DevContainer**는 VSCode에서 Docker 컨테이너를 **개발 환경**처럼 사용할 수 있게 해주는 기능입니다.

- 코드를 실행하거나 디버깅할 때 **컨테이너 내부 환경에서 동작**
- 팀원 간 **환경 차이 없이 동일한 개발 환경 구성** 가능
- `.devcontainer` 폴더에 정의된 설정을 VSCode가 읽어 자동 구성

---

## 3. Docker Desktop 설치하기

1. Docker 공식 사이트에서 설치 파일 다운로드:  
   👉 [https://www.docker.com/products/docker-desktop](https://www.docker.com/products/docker-desktop)

2. 설치 후 Docker Desktop 실행  
   - Windows: Docker 아이콘이 트레이에 떠야 함  
   - macOS: 상단 메뉴바에 Docker 아이콘 확인

---

## 4. 프로젝트 파일 다운로드 (히스토리 없이)

터미널(CMD, PowerShell, zsh 등)에서 아래 명령어로 프로젝트 폴더만 내려받습니다:

```bash
git clone --depth=1 https://github.com/krafton-jungle/webproxy_lab_docker.git
```

- `--depth=1` 옵션은 git commit 히스토리를 생략하고 **최신 파일만 가져옵니다.**

### 📂 다운로드 후 폴더 구조 설명

```
webproxy_lab_docker/
├── .devcontainer/
│   ├── devcontainer.json      # VSCode에서 컨테이너 환경 설정
│   └── Dockerfile             # C 개발 환경 이미지 정의
│
├── .vscode/
│   ├── launch.json            # 디버깅 설정 (F5 실행용)
│   └── tasks.json             # 컴파일 자동화 설정
│
├── webproxy-lab
│   ├── tiny                    # tiny 웹 서버 구현 폴더
│   │  ├── cgi-bin              # tiny 웹 서버를 테스트하기 위한 동적 컨텐츠를 구현하기 위한 폴더
│   │  ├── home.html            # tiny 웹 서버를 테스트하기 위한 정적 HTML 파일
│   │  ├── tiny.c               # tiny 웹 서버 구현 파일
│   │  └── Makefile             # tiny 웹 서버를 컴파일하기 위한 파일
│   ├── Makefile                # proxy 웹 서버를 컴파일하기 위한 파일
│   └── proxy.c                 # proxy 웹 서버 구현 파일
│
└── README.md  # 설치 및 사용법 설명 문서
```
---

## 5. VSCode에서 해당 프로젝트 폴더 열기

1. VSCode를 실행
2. `파일 → 폴더 열기`로 방금 클론한 `webproxy_lab_docker` 폴더를 선택

---

## 6. 개발 컨테이너: 컨테이너에서 열기

1. VSCode에서 `Ctrl+Shift+P` (Windows/Linux) 또는 `Cmd+Shift+P` (macOS)를 누릅니다.
2. 명령어 팔레트에서 `Dev Containers: Reopen in Container`를 선택합니다.
3. 이후 컨테이너가 자동으로 실행되고 빌드됩니다. 처음 컨테이너를 열면 빌드하는 시간이 오래걸릴 수 있습니다. 빌드 후, 프로젝트가 **컨테이너 안에서 실행됨**.

---

## 7. C 파일에 브레이크포인트 설정 후 디버깅 (F5)

이제 본격적으로 문제를 풀 시간입니다. `webproxy-lab/README.md` 파일을 참조하셔서 webproxy 문제를 풀어보세요.
구현 순서는 tiny 웹서버(`webproxy-lab/tiny/tiny.c`)를 CSApp책에 있는 코드를 이용해서 구현하고, proxy서버(`webproxy-lab/proxy.c`)를 구현한 뒤에 최종 `webproxy-lab/mdriver`를 실행하여 70점 만점을 목표로 구현하세요.

C 언어로 문제를 풀다가 디버깅이 필요하시면 소스코드에 BreakPoint를 설정한 뒤에 키보드에서 `F5`를 눌러 디버깅을 시작할 수 있습니다. 디버깅은 tiny 서버와 proxy 서버용 2가지로 제공되며 각각 "Debug Tiny Server", "Debug Proxy Server" 이름을 가집니다. 두가지 중 원하는 디버깅 설정을 선택한 뒤에 `F5`를 누르면 해당 서버가 디버깅모드로 실행됩니다. 

* 기본적으로 "Debug Tiny Server"는 tiny 서버를 실행할때 포트를 `8000`을, "Debug Proxy Server"는 `4500`를  사용합니다. 해당 포트를 이미 다른 프로세스가 사용중이라면 새로운 포트로(`launch.json`파일에서 가능) 변경한 뒤에 디버깅을 진행합니다.


---

## 8. 새로운 Git 리포지토리에 Commit & Push 하기

금주 프로젝트를 개인 Git 리포와 같은 다른 리포지토리에 업로드하려면, 기존 Git 연결을 제거하고 새롭게 초기화해야 합니다.

### ✅ 완전히 새로운 Git 리포로 업로드하는 방법

아래 명령어를 순서대로 실행하세요:

```bash
rm -rf .git
git init
git remote add origin https://github.com/myusername/my-new-repo.git
git add .
git commit -m "Clean start"
git push -u origin main
```

### 📌 설명

- `rm -rf .git`: 기존 Git 기록과 연결을 완전히 삭제합니다.
- `git init`: 현재 폴더를 새로운 Git 리포지토리로 초기화합니다.
- `git remote add origin ...`: 새로운 리포지토리 주소를 origin으로 등록합니다.
- `git add .` 및 `git commit`: 모든 파일을 커밋합니다.
- `git push`: 새로운 리포에 최초 업로드(Push)합니다.

이 과정을 거치면 기존 리포와의 연결은 완전히 제거되고, **새로운 독립적인 프로젝트로 관리**할 수 있습니다.

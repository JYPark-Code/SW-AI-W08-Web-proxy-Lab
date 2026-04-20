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

### Part 3: CSAPP Homework

- [x] **11.6c** — MPG 비디오 타입 지원 추가
- [x] **11.7** — HEAD 메서드 지원 추가
- [x] **11.9** — `mmap` 대신 `malloc + rio_readn` 사용
- [x] **11.10** — POST 메서드 지원
- [x] **11.11** — Path Traversal 방어

#### Part 3 — 구현 플로우 & 배운 점

**11.6c (MPG 서빙) — 생각보다 브라우저 코덱이 변수였다**

접근 자체는 간단했다. `get_filetype`에 `.mpg → video/mpeg` 분기 한 줄만 추가하면 끝. `serve_static`은 이미 `Rio_writen`으로 `filesize` 바이트를 그대로 쏘기 때문에 바이너리 안전하다.

MPG 파일이 없어서 ffmpeg로 만들었는데, 첫 시도는 실패했다.
```bash
# 실패: MPEG-1은 15fps를 지원 안 함
ffmpeg -f lavfi -i testsrc=duration=10:size=320x240:rate=15 -c:v mpeg1video test.mpg
# 성공: 25fps로 생성
ffmpeg -f lavfi -i testsrc=duration=10:size=320x240:rate=25 -c:v mpeg1video test.mpg
```

MPEG-1은 1993년 표준이라 프레임레이트가 23.976/25/29.97/30/50/60 등으로 고정돼 있다.

더 큰 함정은 **브라우저가 MPEG-1을 기본 지원하지 않는다는 것**. `video.html`에서 `<video>` 태그로 띄우면 플레이어 UI는 뜨지만 재생이 안 된다. 이건 tiny 서버 문제가 아니라 브라우저 코덱 한계다. 검증은 두 갈래로:
- 서버가 올바르게 서빙하는지 → DevTools Network 탭에서 `Content-Type: video/mpeg`, 200 OK, 바이트 일치 확인
- 실제 재생 감 → MP4로도 생성해서 `.mp4 → video/mp4` 분기를 추가, `<source>` 체인으로 MP4 우선 재생

배운 점: "서버가 서빙하는 것"과 "브라우저가 렌더링하는 것"은 별개의 문제다. 과제 요구사항은 전자다.

---

**11.7 (HEAD) + 11.9 (malloc+rio_readn) — 시그니처부터 고치면 자연스럽게 따라온다**

이 둘은 `serve_static` 한 함수 안에서 같이 처리된다.
- 11.9: `mmap/Munmap` 블록을 `Malloc + Rio_readn + free`로 교체. Close 호출 순서가 바뀐다는 게 포인트 — mmap은 매핑 후 fd를 닫아도 매핑이 유지되지만, `Rio_readn`은 "읽고 나서" Close.
- 11.7: `serve_static` 시그니처에 `method` 파라미터를 추가하고, 헤더 전송 **직후 바디 전송 직전**에 `if (!strcasecmp(method, "HEAD")) return;` early return.

`doit`의 메서드 검증도 `GET && HEAD` 둘 다 허용하도록 확장.

최초 테스트 때 `curl -I`가 501 Not Implemented로 돌아와서 잠깐 당황했는데, **`-I`는 HEAD 요청**이라는 걸 상기. tiny가 아직 HEAD를 구현하지 않은 상태였기 때문에 정확히 기대대로 동작한 것. 11.7 구현 후 동일 명령으로 200 OK 떠서 회귀 확인.

한 가지 실수: malloc 버전을 추가하면서 기존 mmap 블록을 지우지 않고 남겨둬서 파일이 두 번 전송됐다. 다행히 `diff`에서 크기 불일치로 바로 잡혔다. **replace는 add+delete의 두 동작**임을 다시 깨달음.

동적 컨텐츠(CGI)의 HEAD는 구현하지 않았다. CGI 자식 프로세스가 stdout(소켓)에 바디를 쓰는 구조라, HEAD를 지원하려면 파이프로 CGI 출력을 캡처해서 바디를 잘라내야 한다. 복잡도 대비 가치가 낮아서 범위 외로 뒀다.

---

**11.10 (POST) — pipe로 CGI stdin을 연결하는 게 핵심**

POST는 tiny와 CGI 프로그램(adder.c) **둘 다** 손대야 한다. CGI/1.1 스펙상:
- GET: 쿼리 → `QUERY_STRING` 환경변수
- POST: 바디 → **stdin**, 길이 → `CONTENT_LENGTH` 환경변수, 메서드 → `REQUEST_METHOD`

tiny 쪽 흐름:
1. `read_requesthdrs`에서 `Content-Length:` 헤더 파싱 (out 파라미터로 전달)
2. `serve_dynamic`에서 POST면 `Rio_readnb`로 바디를 읽어 pipe에 기록
3. fork 후 자식에서 `Dup2(pipefd[0], STDIN_FILENO)` — stdin이 pipe read end를 가리키게 됨
4. `Dup2(fd, STDOUT_FILENO)` — 기존 GET 로직 그대로. **이걸 빼먹으면 응답이 아예 안 감**

adder.c 쪽:
- `getenv("REQUEST_METHOD")` 분기
- POST면 `fgets(query, n+1, stdin)` (n은 `CONTENT_LENGTH`)
- 이후 파싱은 GET/POST 공통

첫 시도에서 헤맨 포인트:
1. `adder.c`에 `char content[MAXLINE]`을 실수로 **두 번 선언**해버림. 전체 교체한다는 걸 깜빡하고 패치만 얹어서 생긴 문제.
2. POST 경로에서 `n1, n2`를 파싱하는 로직이 누락돼 항상 `0 + 0 = 0`이 나옴. GET/POST 공통 파싱 경로로 합쳐서 해결.
3. `sprintf(content, ..., buf)`에서 `buf`가 GET 브랜치에서만 초기화되는데 POST 경로에서도 참조돼 UB 발생 → 출력에 `QUERY_STRING=@�]�` 쓰레기 문자. `query[MAXLINE] = ""`로 공용 버퍼 도입하고 GET도 `strcpy(query, buf)`로 저장하도록 수정.
4. `strchr(query, '&')` 후 `*p = '\0'`로 잘라 파싱한 뒤 `*p = '&'`로 원상 복구. 안 그러면 QUERY_STRING 출력이 `n1=15213`에서 잘린다 (원본 GET도 같은 버그였음).

제약: pipe 버퍼가 리눅스 기본 64KB라서, 현재 구현은 바디가 64KB 넘으면 부모가 write에서 블록된다. 숙제 수준 폼 데이터(<1KB)엔 무관하지만 프로덕션 프록시라면 fork 후 부모/자식이 동시에 주고받는 구조로 가야 한다.

검증:
```bash
# GET
curl "http://localhost:8000/cgi-bin/adder?n1=15213&n2=18213"
# POST
curl -v -X POST -d "n1=15213&n2=18213" http://localhost:8000/cgi-bin/adder
# 둘 다 "The answer is: 15213 + 18213 = 33426"
```

---

**11.11 (Path Traversal) — 검증 도구가 오히려 방어를 우회한다**

구현은 `doit`의 `parse_uri` 직전에 한 줄.
```c
if (strstr(uri, "..")) {
  clienterror(fd, uri, "403", "Forbidden", "Path traversal not allowed");
  return;
}
```

재미있었던 건 검증 방식. 처음에 `curl "/../../etc/passwd"`로 테스트했더니 403이 아니라 404가 떴다. 처음엔 방어 로직이 틀린 줄 알았는데, tiny 쪽 로그를 보니:
```
GET /etc/passwd HTTP/1.1
```

**curl이 요청을 보내기 전에 URI를 정규화**해서 `..`를 이미 없앤 후 보낸 것. 그래서 서버의 `strstr(uri, "..")` 방어까지 도달하지도 못하고, `./etc/passwd`로 stat → 파일 없음 → 404 경로를 탔다.

서버 방어 자체는 맞다는 걸 확인하려면 raw 요청을 보내야 한다:
```bash
# 방법 1: --path-as-is로 curl의 정규화 비활성화
curl -v --path-as-is "http://localhost:8000/../../etc/passwd"
# 방법 2: netcat
printf 'GET /../../etc/passwd HTTP/1.0\r\n\r\n' | nc localhost 8000
```

이때 비로소 403 Forbidden이 떴다. 배운 점: **클라이언트 도구가 서버 동작을 가린다**. 서버 테스트할 때는 raw 레벨 도구(netcat, `--path-as-is`, 수제 HTTP 요청)를 확보해둬야 한다.

방어 수준 자체는 최소 방어다. `strstr(uri, "..")`는 정상 파일명에 `..`가 포함된 경우(예: `my..file.txt`)도 차단해버리는 오버블로킹이 있고, 진짜 안전한 방어는 document root 기반으로 `realpath`를 비교하는 것. 숙제 범위에서는 최소 방어로 충분.

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

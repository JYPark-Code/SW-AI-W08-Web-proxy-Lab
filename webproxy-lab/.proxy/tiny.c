/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 *
 * ============================================================================
 * [학습 주석] 이 파일은 CSAPP 11.6절 "Tiny Web Server"의 구현체다.
 *
 * 전체 흐름:
 *   main → accept → doit → (parse_uri, read_requesthdrs, ...)
 *                       ├→ serve_static  (정적 컨텐츠: HTML, 이미지 등)
 *                       └→ serve_dynamic (동적 컨텐츠: CGI 프로그램 실행)
 *
 * 핵심 개념:
 *   - iterative 서버: 한 번에 한 요청씩 처리 (accept → doit → close 루프)
 *   - HTTP/1.0: Content-length나 연결 종료로 바디 끝 표시 (keep-alive X)
 *   - CGI: fork + dup2 + execve 조합으로 외부 프로그램 실행해 동적 응답 생성
 *
 * 함수 역할 요약:
 *   main            : 서버 초기화, accept 루프
 *   doit            : 한 요청 처리 본체 (요청 라인 파싱 → 정적/동적 분기)
 *   read_requesthdrs: 요청 헤더 읽고 버림 (Tiny는 헤더 내용 안 씀)
 *   parse_uri       : URI → 파일 경로 + CGI 인자 분리
 *   serve_static    : 정적 파일 응답 (헤더 + mmap으로 바디 전송)
 *   get_filetype    : 파일 확장자 → MIME 타입 매핑
 *   serve_dynamic   : CGI 실행 (fork 후 dup2로 stdout 리다이렉트)
 *   clienterror     : 4xx/5xx 에러 응답 HTML 생성 및 전송
 * ============================================================================
 */
#include "csapp.h"

/* 함수 프로토타입 선언 (구현은 아래에) */
void doit(int fd);
void read_requesthdrs(rio_t *rp, int *content_length);           /* 11.10 POST 메서드 */
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);  /* 11.7 HEAD 메서드 */
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs,
                   char *method, rio_t *rp, int content_length); /* 11.7 + 11.10 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/*
 * main - 서버 진입점
 *
 * 동작:
 *   1. 명령줄 인자에서 포트 번호 받음
 *   2. Open_listenfd로 리스닝 소켓 생성 (socket + bind + listen 조합)
 *   3. 무한 루프: accept → doit → close
 *
 * 이 구조는 echoserveri.c와 거의 동일. 차이는 echo 대신 doit을 호출한다는 것.
 * "iterative 서버"의 전형적 패턴.
 */
int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;  /* IPv4/IPv6 모두 수용 가능한 크기 */

  /* 명령줄 인자 검증: ./tiny <port> 형식 요구 */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  /* 리스닝 소켓 생성 (csapp.c의 래퍼 함수)
   * 내부에서 socket() + setsockopt(SO_REUSEADDR) + bind() + listen() 수행 */
  listenfd = Open_listenfd(argv[1]);

  /* accept 루프: 연결 하나 받고 처리하고 닫고, 반복 */
  while (1)
  {
    clientlen = sizeof(clientaddr);

    /* accept: 연결 대기. 클라이언트 접속 시 새 fd(connfd) 반환.
     * listenfd는 계속 살아있어 다음 연결을 기다림.
     * clientaddr에 클라이언트 주소 정보가 채워짐. */
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept

    /* 클라이언트 주소를 사람이 읽을 수 있는 형태로 변환 (로그 용도) */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    doit(connfd);   /* 실제 요청 처리 */ // line:netp:tiny:doit
    Close(connfd);  /* 연결 종료, fd 반환. 빼먹으면 fd 누수! */ // line:netp:tiny:close
  }
}

/*
 * doit - 한 HTTP 요청/응답 트랜잭션 처리
 *
 * 처리 순서:
 *   1. 요청 라인 읽기 (예: "GET /home.html HTTP/1.1")
 *   2. 메서드 검증 (Tiny는 GET만 지원, 나머지는 501)
 *   3. 요청 헤더 소비 (내용은 안 쓰고 빈 줄까지 읽기만 함)
 *   4. URI 파싱 → 정적/동적 판별
 *   5. 파일 존재/권한 확인
 *   6. 정적이면 serve_static, 동적이면 serve_dynamic 호출
 */
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  int content_length = 0;
  rio_t rio;  /* RIO 버퍼 구조체 (내부 8KB 버퍼로 읽기 효율화) */

  /* RIO 버퍼 초기화: 이 fd에서 읽을 때 이 rio_t 사용하도록 연결 */
  Rio_readinitb(&rio, fd);

  /* 요청 라인 읽기 (한 줄 = \n까지)
   * 반환 0이면 EOF (클라이언트가 바로 연결 끊음) → 그냥 종료 */
  if (!Rio_readlineb(&rio, buf, MAXLINE))
    return;
  printf("%s", buf);  /* 받은 요청 로그 */

  /* 요청 라인 파싱: "GET /path HTTP/1.1" → method, uri, version으로 분리 */
  sscanf(buf, "%s %s %s", method, uri, version);

  /* 메서드 검증: GET 외에는 501 Not Implemented 반환
   * strcasecmp: 대소문자 무시 비교 (GET == get == Get) */
  /* 11.7 + 11.10: GET / HEAD / POST 모두 허용 */
  if (strcasecmp(method, "GET") &&
      strcasecmp(method, "HEAD") &&
      strcasecmp(method, "POST"))
  {
    clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
    return;
  }

  /* 11.11: Path traversal 방어. parse_uri 전에 URI 원본에서 거른다. */
  if (strstr(uri, ".."))
  {
    clienterror(fd, uri, "403", "Forbidden",
                "Path traversal not allowed");
    return;
  }

  /* 요청 헤더 전부 읽고 버림 (Tiny는 헤더 내용 활용 안 함)
   * 단, 헤더를 다 소비해야 다음 요청과 섞이지 않음 */
  /* 11.10: 헤더에서 Content-Length 파싱해서 받아옴 */
  read_requesthdrs(&rio, &content_length);

  /* URI 파싱: URI → 파일 경로(filename) + CGI 인자(cgiargs) 분리
   * 반환값: 1이면 정적, 0이면 동적 */
  is_static = parse_uri(uri, filename, cgiargs);

  /* 파일 존재 확인: stat으로 파일 메타데이터 조회
   * sbuf.st_mode: 파일 타입/권한 비트
   * sbuf.st_size: 파일 크기
   * 실패 시 404 Not Found 반환 */
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }

  if (is_static)
  { /* 정적 컨텐츠 처리 */
    /* S_ISREG: 일반 파일인지 (디렉토리/장치파일 아님)
     * S_IRUSR: 소유자 읽기 권한
     * 둘 다 만족해야 서빙 가능, 아니면 403 Forbidden */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    /* 11.7: method 전달 */
    serve_static(fd, filename, sbuf.st_size, method);
  }
  else
  { /* 동적 컨텐츠 (CGI) 처리 */
    /* S_IXUSR: 소유자 실행 권한
     * CGI는 실행 가능해야 함, 아니면 403 */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    /* 11.7 + 11.10: method, rp, content_length 전달 */
    serve_dynamic(fd, filename, cgiargs, method, &rio, content_length);
  }
}

/*
 * read_requesthdrs - HTTP 요청 헤더 전부 읽고 버림
 *
 * HTTP 메시지 구조:
 *   요청 라인\r\n
 *   헤더: 값\r\n
 *   헤더: 값\r\n
 *   \r\n        ← 빈 줄 (헤더 끝 표시) ★
 *   바디(선택)
 *
 * Tiny는 헤더 내용을 쓰지 않지만, TCP 스트림에서 헤더 영역을
 * 소비해야 다음 처리가 꼬이지 않음.
 *
 * 종료 조건: 읽은 줄이 "\r\n"(빈 줄)일 때.
 */
void read_requesthdrs(rio_t *rp, int *content_length)
{
  char buf[MAXLINE];

  /* 첫 헤더 한 줄 읽기 */
  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);

  /* 빈 줄("\r\n") 만날 때까지 계속 읽기
   * strcmp가 0 반환 = 같다 = 빈 줄 = 루프 종료 */
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);

     /* 11.10: Content-Length 헤더 파싱 (대소문자 무시) */
    if (!strncasecmp(buf, "Content-Length:", 15))
      *content_length = atoi(buf + 15);
  }
  return;
}

/*
 * parse_uri - URI를 파일 경로(filename)와 CGI 인자(cgiargs)로 분리
 *
 * 반환값: 1 = 정적, 0 = 동적
 *
 * 판별 기준: URI에 "cgi-bin" 문자열이 있으면 동적, 없으면 정적.
 *
 * 정적 예시:
 *   uri = "/home.html"
 *   → filename = "./home.html", cgiargs = ""
 *
 *   uri = "/" (끝이 /)
 *   → filename = "./home.html" (기본 파일 추가), cgiargs = ""
 *
 * 동적 예시:
 *   uri = "/cgi-bin/adder?n1=15213&n2=18213"
 *   → filename = "./cgi-bin/adder", cgiargs = "n1=15213&n2=18213"
 *   (? 위치에 \0 삽입해서 filename을 잘라냄)
 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  { /* 정적 컨텐츠 */
    strcpy(cgiargs, "");       /* 정적은 CGI 인자 없음 */
    strcpy(filename, ".");     /* 현재 디렉토리 기준 상대 경로로 시작 */
    strcat(filename, uri);     /* "./home.html" 형태 완성 */

    /* URI가 "/"로 끝나면 기본 파일 "home.html" 추가
     * 예: "/" → "./home.html", "/dir/" → "./dir/home.html" */
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  { /* 동적 컨텐츠 */
    /* index() = strchr()의 BSD 버전 (같은 동작) */
    ptr = index(uri, '?');     /* ? 위치 찾기 */
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1); /* ? 이후를 cgiargs로 복사 */
      *ptr = '\0';              /* ? 위치에 널 문자 → uri 여기서 잘림 */
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);      /* 이제 uri는 ? 이전만 남음 */
    return 0;
  }
}

/*
 * serve_static - 정적 파일을 클라이언트로 전송
 *
 * 동작:
 *   1. 파일 타입 결정 (MIME)
 *   2. HTTP 응답 헤더 조립 (버퍼에 누적) 후 한 번에 전송
 *   3. 파일을 mmap으로 메모리에 매핑
 *   4. Rio_writen으로 한 번에 전송
 *   5. mmap 해제
 *
 * [성능 기법 1] snprintf + 포인터 전진:
 *   공식 원본은 sprintf 후 즉시 Rio_writen을 반복(4번 시스템 콜).
 *   여기서는 buf에 누적한 후 한 번에 Rio_writen(1번 시스템 콜).
 *   - 시스템 콜 횟수 절감 → 성능 이점
 *   - snprintf가 remaining으로 크기 체크 → 오버플로우 방어
 *   - aliasing 위험도 없음
 *
 * [성능 기법 2] mmap:
 *   파일을 메모리 주소 공간에 매핑 → read/copy 없이 포인터로 바로 전송.
 *   대용량 파일에 유리 (커널이 페이지 단위로 효율 관리).
 */
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE];

  char buf[MAXBUF];
  char *p = buf;               /* 버퍼의 현재 쓰기 위치 */
  int n;                       /* snprintf가 쓴 바이트 수 */
  int remaining = sizeof(buf); /* 버퍼의 남은 공간 */

  /* MIME 타입 결정 (예: "text/html", "image/gif") */
  get_filetype(filename, filetype);

  /* HTTP 응답 헤더를 buf에 순차 누적
   * 각 snprintf 호출 후 p를 쓴 만큼 전진시키고 remaining 차감 */
  n = snprintf(p, remaining, "HTTP/1.0 200 OK\r\n");
  p += n;
  remaining -= n;

  n = snprintf(p, remaining, "Server: Tiny Web Server\r\n");
  p += n;
  remaining -= n;

  n = snprintf(p, remaining, "Connection: close\r\n");
  p += n;
  remaining -= n;

  n = snprintf(p, remaining, "Content-length: %d\r\n", filesize);
  p += n;
  remaining -= n;

  /* 마지막 헤더 뒤에 \r\n\r\n: 두 번째 \r\n이 "헤더 끝" 신호 */
  n = snprintf(p, remaining, "Content-type: %s\r\n\r\n", filetype);
  p += n;
  remaining -= n;

  /* 조립된 헤더를 한 번에 전송 */
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* 11.7: HEAD는 여기서 종료 (헤더만 보내고 바디 생략) */
  if (!strcasecmp(method, "HEAD"))
    return;

  /* 11.9: mmap/Munmap → Malloc + Rio_readn + free */
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = (char *)Malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);

  /* 응답 바디 전송 (이전 mmap 방식) */
  // srcfd = Open(filename, O_RDONLY, 0);  /* 파일 열기 */
  // /* mmap: 파일 내용을 메모리에 매핑
  //  * PROT_READ: 읽기 전용
  //  * MAP_PRIVATE: 복사 (원본 보호), 메모리 수정이 파일에 반영 안 됨 */
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // /* 매핑 이후에는 srcfd 필요 없음 (매핑이 자체 참조 유지) */
  // Close(srcfd);
  // /* 매핑된 메모리 내용을 소켓으로 전송 */
  // Rio_writen(fd, srcp, filesize);
  // /* 매핑 해제 (메모리 회수) */
  // Munmap(srcp, filesize);
}

/*
 * get_filetype - 파일 확장자에 따른 MIME 타입 결정
 *
 * MIME = Multipurpose Internet Mail Extensions
 *   원래 이메일용으로 만들어졌으나 HTTP에 차용됨
 *   "타입/서브타입" 형식 (예: text/html, image/png)
 *
 * 브라우저는 이 값으로 렌더링 방식 결정:
 *   text/html → HTML 파싱
 *   image/* → 이미지 표시
 *   application/octet-stream → 다운로드
 *
 * [숙제 11.6c] video/mpg 추가 (.mpg 확장자 분기)
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mpg"))           
    strcpy(filetype, "video/mpeg");
  else if (strstr(filename, ".mp4"))        
    strcpy(filetype, "video/mp4");                
  else
    strcpy(filetype, "text/plain");  /* 알 수 없으면 일반 텍스트로 */
}

/*
 * serve_dynamic - CGI 프로그램 실행하여 동적 응답 생성
 *
 * CGI = Common Gateway Interface (1993년 표준)
 *
 * 핵심 트릭: fork + dup2 + execve
 *   1. fork: 자식 프로세스 생성 (서버 본체는 계속 accept 가능)
 *   2. setenv: QUERY_STRING 환경변수로 파라미터 전달
 *   3. dup2(fd, STDOUT_FILENO): 자식의 stdout을 소켓으로 리다이렉트 ★
 *   4. execve: CGI 프로그램으로 변신 (환경변수, fd 테이블은 유지)
 *
 * dup2가 핵심:
 *   - 자식의 fd 1번(stdout)이 원래 터미널을 가리켰는데
 *   - dup2 후에는 클라이언트 소켓을 가리킴
 *   - CGI 프로그램이 printf만 해도 네트워크로 응답이 감
 *   - CGI 프로그램은 소켓 존재를 모름 (UNIX의 "모든 것은 파일" 철학)
 *
 * 응답 구조:
 *   - Tiny가 첫 두 줄 보냄: "HTTP/1.0 200 OK\r\n", "Server: Tiny...\r\n"
 *   - CGI 프로그램이 나머지 보냄: Content-type, Content-length, 빈 줄, 바디
 *
 * 주의: CGI 프로그램이 QUERY_STRING을 "key=value&key=value" 형식으로
 *       기대할 수 있음. 예: adder는 "n1=15213&n2=18213" 필요.
 */
void serve_dynamic(int fd, char *filename, char *cgiargs,
                   char *method, rio_t *rp, int content_length)
{
  char buf[MAXLINE], *emptylist[] = {NULL};  /* CGI에 넘길 argv (없음) */
  char *body = NULL;
  int pipefd[2] = {-1, -1};
  int is_post = !strcasecmp(method, "POST");

  /* HTTP 응답의 처음 두 헤더는 부모(Tiny)가 보냄 */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* 11.10: POST면 바디를 읽어 pipe에 써둠 (CGI가 stdin으로 읽도록) */
  if (is_post && content_length > 0)
  {
    body = (char *)Malloc(content_length);
    Rio_readnb(rp, body, content_length);

    if (pipe(pipefd) < 0) { free(body); return; }
    Rio_writen(pipefd[1], body, content_length);
    Close(pipefd[1]);          /* write end 닫아 EOF 전달 */
    free(body);
  }

  /* fork: 자식 프로세스 생성
   * Fork() 반환값:
   *   자식 → 0
   *   부모 → 자식 PID (양수)
   * 아래 if 블록은 자식만 실행 */
  if (Fork() == 0)
  { /* 자식 프로세스 */ // line:netp:servedynamic:fork

    /* 환경변수 설정: CGI 프로그램이 getenv("QUERY_STRING")으로 읽음
     * 세 번째 인자 1 = 기존 값 덮어쓰기 */
    setenv("QUERY_STRING", cgiargs, 1); // line:netp:servedynamic:setenv
    setenv("REQUEST_METHOD", method, 1);     /* 11.10: CGI가 method 판별 */

    /* ★ 핵심: stdout(fd 1)을 소켓(fd)으로 리다이렉트
    * 이후 자식의 printf 출력은 네트워크로 감 */
    if (is_post && content_length > 0)
    {
      char clen[32];
      sprintf(clen, "%d", content_length);
      setenv("CONTENT_LENGTH", clen, 1);
      Dup2(pipefd[0], STDIN_FILENO); // line:netp:servedynamic:dup2
      Close(pipefd[0]);
    }

    /* 프로세스 이미지를 CGI 프로그램으로 교체
     * 성공하면 이 함수는 돌아오지 않음 (자식은 이제 CGI 프로그램)
     * 환경변수와 fd 테이블은 execve 이후에도 유지됨 */
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ); // line:netp:servedynamic:execve
  }

  /*
    부모: pipe() → pipefd[0], pipefd[1] 둘 다 열림
    부모: pipefd[1]에 바디 쓰기
    부모: Close(pipefd[1])     ← 이건 닫음
    부모: Fork() → 자식 생성
    자식: Dup2(pipefd[0], STDIN_FILENO); Close(pipefd[0]) 
    -----------------------------------------------------
    까지 완료 -> fd 누수 방지 필요.
  */

  /* 부모: 사용하지 않는 pipe read end 닫기 (fd 누수 방지) */
  if (pipefd[0] != -1)
    Close(pipefd[0]);

  /* 부모 프로세스: 자식 종료 대기 (좀비 방지)
   * Wait(NULL)은 아무 자식이나 종료 시까지 블록, 종료 상태 무시 */
  Wait(NULL); // line:netp:servedynamic:wait
}

/*
 * clienterror - 에러 응답 메시지를 클라이언트에게 전송
 *
 * 처리: HTTP 상태 라인 + Content-type 헤더 + 빈 줄 + HTML 바디
 *
 * 쓰이는 에러 상황:
 *   - 501 Not Implemented: GET 외 메서드
 *   - 404 Not Found: 파일 없음
 *   - 403 Forbidden: 권한 없음
 *
 * [aliasing 이슈 주의]
 * 구버전 코드는 sprintf(body, "%s...", body) 패턴을 썼는데,
 * 이는 destination과 source가 같은 메모리를 가리켜 UB(undefined behavior).
 *
 * 2019년 droh가 수정한 공식 버전(이 코드)은 body 변수를 제거하고
 * 매번 buf에 새로 쓴 후 즉시 전송하는 방식으로 aliasing 원천 차단.
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE];

  /* HTTP 응답 헤더 전송 */

  /* 상태 라인: "HTTP/1.0 404 Not found\r\n" 같은 형식 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));

  /* Content-type 헤더 + 빈 줄 (헤더 종료)
   * "\r\n\r\n"의 두 번째 \r\n이 헤더 끝 표시 */
  sprintf(buf, "Content-type: text/html\r\n\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* HTTP 응답 바디 (HTML)
   * 공식 버전은 body 버퍼 없이 각 줄을 만들자마자 즉시 Rio_writen으로 전송.
   * 따라서 sprintf aliasing 없음. */

  sprintf(buf, "<html><title>Tiny Error</title>");
  Rio_writen(fd, buf, strlen(buf));

  /* bgcolor=ffffff: HTML4에서는 속성값 따옴표 생략 허용
   * 여기서 연속된 "" "ffffff" ""는 인접 문자열 리터럴 결합으로
   * 실제로는 bgcolor=ffffff 형태가 됨 */
  sprintf(buf, "<body bgcolor="
               "ffffff"
               ">\r\n");
  Rio_writen(fd, buf, strlen(buf));

  sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));

  sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
  Rio_writen(fd, buf, strlen(buf));

  sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
  Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */
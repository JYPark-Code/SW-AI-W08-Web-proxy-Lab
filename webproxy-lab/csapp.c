/* 
 * csapp.c - CS:APP3e 책의 헬퍼 함수 모음
 *
 * 이 파일의 주석은 한국어로 추가되었습니다 (학습용).
 * 원본 코드는 CSAPP 저자(Bryant & O'Hallaron)의 소스입니다.
 *
 * ========================================================================
 * 파일 구조 개요:
 *   1. 에러 처리 함수 (unix_error, posix_error 등)
 *   2. Unix 프로세스 제어 래퍼 (Fork, Execve, Wait) - CGI에 핵심
 *   3. 시그널 래퍼 (Signal, Sigprocmask 등) - 시그널 핸들링
 *   4. SIO (Signal-safe I/O) - 시그널 핸들러 안에서 쓸 안전한 출력
 *   5. Unix I/O 래퍼 (Read, Write, Close, Dup2)
 *   6. 디렉토리/메모리/표준 I/O 래퍼
 *   7. 소켓 인터페이스 래퍼 (Socket, Bind, Listen, Accept, Connect)
 *   8. Pthread 래퍼 (Pthread_create 등) - 동시성에 핵심
 *   9. 세마포어 래퍼 (Sem_init, P, V)
 *  10. RIO 패키지 (Robust I/O) - short count 처리, 버퍼 I/O
 *  11. 클라이언트/서버 헬퍼 (open_clientfd, open_listenfd) - 핵심!
 *
 * ========================================================================
 * 대문자 래퍼 함수의 공통 패턴:
 *   - 소문자(원본 시스템 콜) 호출
 *   - 실패하면 unix_error/posix_error/gai_error 호출 → 프로세스 종료
 *   - 성공하면 정상 결과 반환
 *
 *   예: Fork() = fork() + 실패 시 에러 메시지 + exit
 *
 * ========================================================================
 */
/* $begin csapp.c */
#include "csapp.h"

/************************** 
 * 1. 에러 처리 함수
 **************************/
/* $begin errorfuns */
/* $begin unixerror */

/*
 * unix_error - Unix 시스템 콜 실패 시 호출되는 에러 처리 함수
 * errno (전역 변수)에 저장된 에러 번호를 사람이 읽을 수 있는 메시지로 변환
 * 예: "Open error: No such file or directory"
 */
void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

/*
 * posix_error - POSIX 스레드 함수 실패 시 사용
 * pthread 함수들은 errno 대신 반환값으로 에러 코드를 돌려줌
 * 그래서 code 인자로 직접 받음
 */
void posix_error(int code, char *msg) /* Posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

/*
 * gai_error - getaddrinfo 전용 에러 처리
 * getaddrinfo는 errno가 아닌 전용 에러 코드를 반환하므로 gai_strerror 사용
 */
void gai_error(int code, char *msg) /* Getaddrinfo-style error */
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

/*
 * app_error - 애플리케이션 레벨 에러 (시스템 콜 아님)
 * 예: 잘못된 사용자 입력
 */
void app_error(char *msg) /* Application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
/* $end errorfuns */

/* dns_error: gethostbyname용 구식 에러 처리 (getaddrinfo로 대체됨) */
void dns_error(char *msg) /* Obsolete gethostbyname error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}


/*********************************************
 * 2. Unix 프로세스 제어 래퍼 (CGI에 핵심!)
 ********************************************/

/* $begin forkwrapper */
/*
 * Fork - fork의 대문자 래퍼
 *
 * fork()의 마법: 한 번 호출했는데 두 번 반환됨
 *   - 부모 프로세스에는 자식의 PID (양수) 반환
 *   - 자식 프로세스에는 0 반환
 *
 * 사용 패턴:
 *   if (Fork() == 0) {
 *       // 자식 프로세스만 실행
 *   }
 *   // 부모도, 자식도 여기 이후 계속 실행
 *
 * CGI 구현에 필수 (serve_dynamic에서 씀)
 */
pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

/*
 * Execve - 현재 프로세스를 다른 프로그램으로 교체
 *
 * 중요: 성공 시 돌아오지 않음 (프로세스 이미지가 완전히 교체됨)
 * 그래서 return문이 필요 없음
 * < 0 으로 반환됐다 = 실패 (파일 없음, 권한 없음 등)
 *
 * CGI에서 fork 후 자식이 이 함수로 CGI 프로그램으로 변신
 */
void Execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve error");
}

/* $begin wait */
/*
 * Wait - 자식 프로세스 종료 대기
 *
 * 부모가 wait 안 하면 자식이 좀비 프로세스로 남음
 * status에 자식의 종료 상태 저장 (NULL로 넘기면 무시)
 *
 * CGI에서 fork한 자식을 정리하기 위해 필수
 */
pid_t Wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}
/* $end wait */

/*
 * Waitpid - 특정 PID의 자식만 기다림 (Wait의 정밀 버전)
 * options로 논블로킹 대기 등 가능
 */
pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}

/* $begin kill */
/*
 * Kill - 프로세스에 시그널 보내기
 * 이름과 달리 "죽이기"만이 아니라 모든 시그널 전달에 쓰임
 */
void Kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}
/* $end kill */

/* Pause - 시그널 올 때까지 무한 대기 */
void Pause() 
{
    (void)pause();
    return;
}

/* Sleep - N초 동안 멈춤 */
unsigned int Sleep(unsigned int secs) 
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("Sleep error");
    return rc;
}

/* Alarm - N초 후 SIGALRM 시그널 발생 예약 */
unsigned int Alarm(unsigned int seconds) {
    return alarm(seconds);
}
 
/* Setpgid - 프로세스 그룹 ID 설정 (쉘에서 쓰임) */
void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

/* Getpgrp - 내 프로세스 그룹 ID 조회 */
pid_t Getpgrp(void) {
    return getpgrp();
}

/************************************
 * 3. 시그널 래퍼
 ***********************************/

/* $begin sigaction */
/*
 * Signal - 시그널 핸들러 등록 (sigaction의 이식성 좋은 버전)
 *
 * 프록시에서 SIGPIPE 무시할 때 씀:
 *   Signal(SIGPIPE, SIG_IGN);
 * (클라이언트가 갑자기 연결 끊어도 프록시가 죽지 않도록)
 *
 * SA_RESTART: 시그널에 의해 중단된 시스템 콜을 자동 재시작
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */

/* Sigprocmask - 시그널 마스크 변경 (특정 시그널 블록/언블록) */
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("Sigprocmask error");
    return;
}

/* Sigemptyset - 시그널 집합 비우기 */
void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
	unix_error("Sigemptyset error");
    return;
}

/* Sigfillset - 시그널 집합에 모든 시그널 추가 */
void Sigfillset(sigset_t *set)
{ 
    if (sigfillset(set) < 0)
	unix_error("Sigfillset error");
    return;
}

/* Sigaddset - 집합에 특정 시그널 하나 추가 */
void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
	unix_error("Sigaddset error");
    return;
}

/* Sigdelset - 집합에서 특정 시그널 제거 */
void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset error");
    return;
}

/* Sigismember - 시그널이 집합에 있는지 확인 */
int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
	unix_error("Sigismember error");
    return rc;
}

/* Sigsuspend - 일시적으로 마스크 바꿔 시그널 대기 */
int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

/*************************************************************
 * 4. SIO 패키지 (Signal-safe I/O)
 *
 * 시그널 핸들러 안에서 printf 같은 함수 호출은 위험함 (async-signal-unsafe)
 * SIO는 시그널 핸들러에서도 안전하게 쓸 수 있는 간단한 I/O 루틴 제공
 *************************************************************/

/* Private sio functions */

/* $begin sioprivate */
/* sio_reverse - 문자열 뒤집기 (K&R 책의 고전 예제) */
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - long 정수를 문자열로 변환 (base b진수, K&R itoa) */
static void sio_ltoa(long v, char s[], int b) 
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
	v = -v;

    do {  
        s[i++] = ((c = (v % b)) < 10)  ?  c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
	s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - 문자열 길이 (strlen의 시그널 안전 버전) */
static size_t sio_strlen(char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* Public Sio functions */
/* $begin siopublic */

/* sio_puts - 시그널 안전하게 문자열 출력 */
ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); //line:csapp:siostrlen
}

/* sio_putl - 시그널 안전하게 long 정수 출력 */
ssize_t sio_putl(long v) /* Put long */
{
    char s[128];
    
    sio_ltoa(v, s, 10); /* Based on K&R itoa() */  //line:csapp:sioltoa
    return sio_puts(s);
}

/* sio_error - 에러 메시지 출력 후 강제 종료 */
void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1);                                      //line:csapp:sioexit
}
/* $end siopublic */

/*******************************
 * SIO 래퍼 (에러 체크 추가)
 ******************************/
ssize_t Sio_putl(long v)
{
    ssize_t n;
  
    if ((n = sio_putl(v)) < 0)
	sio_error("Sio_putl error");
    return n;
}

ssize_t Sio_puts(char s[])
{
    ssize_t n;
  
    if ((n = sio_puts(s)) < 0)
	sio_error("Sio_puts error");
    return n;
}

void Sio_error(char s[])
{
    sio_error(s);
}

/********************************
 * 5. Unix I/O 래퍼
 ********************************/

/* Open - 파일 열기. fd 반환 */
int Open(const char *pathname, int flags, mode_t mode) 
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
	unix_error("Open error");
    return rc;
}

/*
 * Read - read 래퍼
 *
 * 중요: read의 반환값
 *   > 0 : 읽은 바이트 수 (요청한 count보다 적을 수 있음 = short count)
 *   = 0 : EOF
 *   < 0 : 에러
 *
 * Short count는 정상 동작이라 여기서 처리 안 함
 * 정확히 n바이트 읽으려면 rio_readn을 써야 함
 */
ssize_t Read(int fd, void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0) 
	unix_error("Read error");
    return rc;
}

/*
 * Write - write 래퍼
 * Read와 마찬가지로 short count 가능
 * 완전한 전송은 rio_writen 사용
 */
ssize_t Write(int fd, const void *buf, size_t count) 
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
	unix_error("Write error");
    return rc;
}

/* Lseek - 파일 내 위치 이동 (소켓엔 안 됨, 파일만) */
off_t Lseek(int fildes, off_t offset, int whence) 
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
	unix_error("Lseek error");
    return rc;
}

/*
 * Close - fd 닫기
 * 프록시에서 close 빼먹으면 fd 누수 발생
 * 서버가 오래 돌면 "Too many open files" 에러
 */
void Close(int fd) 
{
    int rc;

    if ((rc = close(fd)) < 0)
	unix_error("Close error");
}

/* Select - 여러 fd 중 준비된 것 감지 (I/O 멀티플렉싱) */
int Select(int  n, fd_set *readfds, fd_set *writefds,
	   fd_set *exceptfds, struct timeval *timeout) 
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
	unix_error("Select error");
    return rc;
}

/*
 * Dup2 - fd 복제 (CGI의 핵심!)
 *
 * Dup2(fd1, fd2): fd2가 fd1과 같은 것을 가리키게 함
 *   - fd2가 원래 가리키던 것은 자동으로 close됨
 *
 * CGI 활용:
 *   Dup2(connfd, STDOUT_FILENO);  // stdout을 소켓에 연결
 * → 이후 자식이 printf만 해도 클라이언트 소켓으로 전송됨
 */
int Dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("Dup2 error");
    return rc;
}

/* Stat - 파일 상태 정보 조회 (크기, 권한 등) */
void Stat(const char *filename, struct stat *buf) 
{
    if (stat(filename, buf) < 0)
	unix_error("Stat error");
}

/* Fstat - fd를 통한 파일 상태 조회 (Stat의 fd 버전) */
void Fstat(int fd, struct stat *buf) 
{
    if (fstat(fd, buf) < 0)
	unix_error("Fstat error");
}

/*********************************
 * 디렉토리 함수 래퍼
 *********************************/

/* Opendir - 디렉토리 열기 */
DIR *Opendir(const char *name) 
{
    DIR *dirp = opendir(name); 

    if (!dirp)
        unix_error("opendir error");
    return dirp;
}

/* Readdir - 디렉토리에서 다음 항목 읽기 */
struct dirent *Readdir(DIR *dirp)
{
    struct dirent *dep;
    
    errno = 0;
    dep = readdir(dirp);
    if ((dep == NULL) && (errno != 0))
        unix_error("readdir error");
    return dep;
}

/* Closedir - 디렉토리 닫기 */
int Closedir(DIR *dirp) 
{
    int rc;

    if ((rc = closedir(dirp)) < 0)
        unix_error("closedir error");
    return rc;
}

/***************************************
 * 6. 메모리 매핑 래퍼
 ***************************************/

/*
 * Mmap - 파일을 메모리 주소 공간에 매핑
 *
 * Tiny 서버의 serve_static에서 파일 전송에 쓰임
 *   - 파일 내용을 메모리 주소로 다룰 수 있음
 *   - 대용량 파일 I/O에 효율적
 *
 * 숙제 11.9는 이걸 malloc+rio_readn으로 교체하는 연습
 */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) 
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
	unix_error("mmap error");
    return(ptr);
}

/* Munmap - mmap 해제 */
void Munmap(void *start, size_t length) 
{
    if (munmap(start, length) < 0)
	unix_error("munmap error");
}

/***************************************************
 * 동적 메모리 할당 래퍼
 ***************************************************/

/* Malloc - 메모리 할당 + NULL 체크 */
void *Malloc(size_t size) 
{
    void *p;

    if ((p  = malloc(size)) == NULL)
	unix_error("Malloc error");
    return p;
}

/* Realloc - 기존 할당 크기 변경 */
void *Realloc(void *ptr, size_t size) 
{
    void *p;

    if ((p  = realloc(ptr, size)) == NULL)
	unix_error("Realloc error");
    return p;
}

/* Calloc - 할당 후 0으로 초기화 */
void *Calloc(size_t nmemb, size_t size) 
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL)
	unix_error("Calloc error");
    return p;
}

/* Free - 메모리 해제 (에러 없음) */
void Free(void *ptr) 
{
    free(ptr);
}

/******************************************
 * 표준 I/O 래퍼 (파일 포인터 FILE* 사용)
 ******************************************/

/* Fclose - 파일 닫기 (표준 I/O 버전) */
void Fclose(FILE *fp) 
{
    if (fclose(fp) != 0)
	unix_error("Fclose error");
}

/* Fdopen - fd를 FILE*로 래핑 (두 스타일 연결) */
FILE *Fdopen(int fd, const char *type) 
{
    FILE *fp;

    if ((fp = fdopen(fd, type)) == NULL)
	unix_error("Fdopen error");

    return fp;
}

/*
 * Fgets - 한 줄 읽기 (stdin 또는 파일)
 * echoclient에서 사용자 입력 받을 때 씀
 * EOF면 NULL 반환
 */
char *Fgets(char *ptr, int n, FILE *stream) 
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	app_error("Fgets error");

    return rptr;
}

/* Fopen - 파일 열기 (FILE* 반환) */
FILE *Fopen(const char *filename, const char *mode) 
{
    FILE *fp;

    if ((fp = fopen(filename, mode)) == NULL)
	unix_error("Fopen error");

    return fp;
}

/* Fputs - 문자열 출력 */
void Fputs(const char *ptr, FILE *stream) 
{
    if (fputs(ptr, stream) == EOF)
	unix_error("Fputs error");
}

/* Fread - 이진 데이터 읽기 */
size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    size_t n;

    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream)) 
	unix_error("Fread error");
    return n;
}

/* Fwrite - 이진 데이터 쓰기 */
void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
	unix_error("Fwrite error");
}


/**************************** 
 * 7. 소켓 인터페이스 래퍼 (핵심!)
 ****************************/

/*
 * Socket - 소켓 생성
 * domain: AF_INET(IPv4) / AF_INET6(IPv6)
 * type:   SOCK_STREAM(TCP) / SOCK_DGRAM(UDP)
 * 반환값: 새 소켓의 fd
 */
int Socket(int domain, int type, int protocol) 
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
	unix_error("Socket error");
    return rc;
}

/*
 * Setsockopt - 소켓 옵션 설정
 *
 * 대표 용도: SO_REUSEADDR
 *   - TIME_WAIT 상태 포트도 재사용 가능하게
 *   - 서버 재시작 시 "Address already in use" 에러 방지
 */
void Setsockopt(int s, int level, int optname, const void *optval, int optlen) 
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
	unix_error("Setsockopt error");
}

/*
 * Bind - 소켓에 주소(IP+포트) 부여
 * 서버만 호출 (클라이언트는 OS가 임시 포트 자동 할당)
 */
void Bind(int sockfd, struct sockaddr *my_addr, int addrlen) 
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
	unix_error("Bind error");
}

/*
 * Listen - 소켓을 "연결 받기 모드"로 전환
 * backlog: 백로그 큐 크기 (아직 accept 안 한 연결이 대기)
 */
void Listen(int s, int backlog) 
{
    int rc;

    if ((rc = listen(s,  backlog)) < 0)
	unix_error("Listen error");
}

/*
 * Accept - 연결 하나 수락, 새 fd 반환
 *
 * 핵심: listen fd(s)는 유지되고, 새 fd(rc)는 이 클라이언트 전용
 * 큐가 비어있으면 블록(대기)
 * addr에 클라이언트 주소 정보가 채워짐
 */
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) 
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	unix_error("Accept error");
    return rc;
}

/*
 * Connect - 클라이언트가 서버에 접속
 * 성공 시점 = TCP 3-way handshake 완료
 */
void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen) 
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
	unix_error("Connect error");
}

/*******************************
 * 프로토콜 독립 래퍼 (IPv4/IPv6 둘 다 지원)
 *******************************/
/* $begin getaddrinfo */
/*
 * Getaddrinfo - 호스트명+포트를 주소 구조체 리스트로 변환
 *
 * 예: "www.example.com" + "80" → 여러 IP 주소 구조체들
 * gethostbyname의 현대적 대체제 (IPv6 지원, 스레드 안전)
 */
void Getaddrinfo(const char *node, const char *service, 
                 const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    if ((rc = getaddrinfo(node, service, hints, res)) != 0) 
        gai_error(rc, "Getaddrinfo error");
}
/* $end getaddrinfo */

/*
 * Getnameinfo - 소켓 주소를 사람이 읽을 수 있는 호스트명/서비스명으로 변환
 * echoserveri에서 "누가 접속했는지" 로그 찍을 때 씀
 */
void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, 
                 size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv, 
                          servlen, flags)) != 0) 
        gai_error(rc, "Getnameinfo error");
}

/* Freeaddrinfo - getaddrinfo가 할당한 리스트 해제 (메모리 누수 방지) */
void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

/* Inet_ntop - 바이너리 IP → 문자열 ("192.168.1.1" 같은) */
void Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (!inet_ntop(af, src, dst, size))
        unix_error("Inet_ntop error");
}

/* Inet_pton - 문자열 IP → 바이너리 (ntop의 반대) */
void Inet_pton(int af, const char *src, void *dst) 
{
    int rc;

    rc = inet_pton(af, src, dst);
    if (rc == 0)
	app_error("inet_pton error: invalid dotted-decimal address");
    else if (rc < 0)
        unix_error("Inet_pton error");
}

/*******************************************
 * DNS 인터페이스 래퍼 (구식, getaddrinfo로 대체됨)
 *
 * NOTE: gethostbyname 계열은 스레드 안전하지 않아 사용 비권장
 * 새 코드는 getaddrinfo / getnameinfo 사용
 ***********************************/

/* $begin gethostbyname */
struct hostent *Gethostbyname(const char *name) 
{
    struct hostent *p;

    if ((p = gethostbyname(name)) == NULL)
	dns_error("Gethostbyname error");
    return p;
}
/* $end gethostbyname */

struct hostent *Gethostbyaddr(const char *addr, int len, int type) 
{
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
	dns_error("Gethostbyaddr error");
    return p;
}

/************************************************
 * 8. Pthread 래퍼 (동시성에 핵심!)
 *
 * 프록시 Part 2에서 요청마다 스레드 생성할 때 씀
 ************************************************/

/*
 * Pthread_create - 새 스레드 생성
 * tidp: 새 스레드 ID 저장할 포인터
 * attrp: 스레드 속성 (NULL이면 기본값)
 * routine: 스레드가 실행할 함수
 * argp: 함수에 넘길 인자
 *
 * 사용 패턴:
 *   pthread_t tid;
 *   Pthread_create(&tid, NULL, handler, (void *)connfd);
 */
void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, 
		    void * (*routine)(void *), void *argp) 
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
	posix_error(rc, "Pthread_create error");
}

/* Pthread_cancel - 스레드 강제 종료 */
void Pthread_cancel(pthread_t tid) {
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
	posix_error(rc, "Pthread_cancel error");
}

/* Pthread_join - 스레드 종료 대기 (wait의 스레드 버전) */
void Pthread_join(pthread_t tid, void **thread_return) {
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
	posix_error(rc, "Pthread_join error");
}

/* $begin detach */
/*
 * Pthread_detach - 스레드 분리 (join 필요 없음, 종료 시 자동 정리)
 * 프록시처럼 accept 루프에서 매번 스레드 만드는 경우 필수
 * 없으면 스레드 자원 누수
 */
void Pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
	posix_error(rc, "Pthread_detach error");
}
/* $end detach */

/* Pthread_exit - 현재 스레드 종료 */
void Pthread_exit(void *retval) {
    pthread_exit(retval);
}

/* Pthread_self - 내 스레드 ID 조회 */
pthread_t Pthread_self(void) {
    return pthread_self();
}
 
/* Pthread_once - 특정 함수가 전체 생애에서 단 한 번만 실행되게 */
void Pthread_once(pthread_once_t *once_control, void (*init_function)()) {
    pthread_once(once_control, init_function);
}

/*******************************
 * 9. POSIX 세마포어 래퍼
 *
 * 스레드 간 동기화에 사용
 * 프록시 Part 3 캐시의 readers-writers 문제에서 쓸 수 있음
 *******************************/

/*
 * Sem_init - 세마포어 초기화
 * value: 초기값 (1 = 뮤텍스처럼, N = N개의 자원 허용)
 */
void Sem_init(sem_t *sem, int pshared, unsigned int value) 
{
    if (sem_init(sem, pshared, value) < 0)
	unix_error("Sem_init error");
}

/*
 * P - 세마포어 감소 (자원 획득)
 * 값이 0이면 블록, > 0이면 1 감소시키고 진행
 * "Prolaag" (네덜란드어 "시도하고 감소")
 */
void P(sem_t *sem) 
{
    if (sem_wait(sem) < 0)
	unix_error("P error");
}

/*
 * V - 세마포어 증가 (자원 해제)
 * "Verhoog" (네덜란드어 "증가")
 * P/V 쌍으로 임계 영역 보호
 */
void V(sem_t *sem) 
{
    if (sem_post(sem) < 0)
	unix_error("V error");
}

/****************************************
 * 10. RIO 패키지 (Robust I/O) - 핵심!
 *
 * 이 섹션이 CSAPP 네트워크 프로그래밍의 알짬이에요.
 * 두 가지 버전 제공:
 *   (1) Unbuffered: rio_readn, rio_writen
 *       - 정확히 n바이트 읽기/쓰기 (short count 자동 처리)
 *   (2) Buffered: rio_readnb, rio_readlineb
 *       - 내부 8KB 버퍼 사용
 *       - "한 줄 읽기" 가능 (HTTP 파싱에 필수)
 *       - 시스템 콜 횟수 최소화로 성능 향상
 ****************************************/

/*
 * rio_readn - short count 자동 처리하는 안전한 읽기 (unbuffered)
 *
 * read()가 요청한 n보다 적게 돌려줘도 루프 돌며 n바이트 다 읽을 때까지 반복
 * 네트워크 소켓에서 이게 왜 필요한가:
 *   read(fd, buf, 1000) 호출했는데 500만 올 수 있음 (네트워크 지연)
 *   순진하게 짜면 나머지 500을 놓침 → 데이터 손실
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;              // 아직 읽지 못한 바이트 수
    ssize_t nread;                 // 이번 read() 호출이 돌려준 값
    char *bufp = usrbuf;           // 버퍼 쓰기 위치 포인터

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR)         /* 시그널로 인한 중단 */
		nread = 0;              /* 에러 아니니 계속 읽기 */
	    else
		return -1;              /* 진짜 에러 */
	} 
	else if (nread == 0)
	    break;                      /* EOF: 더 읽을 게 없음 */
	nleft -= nread;                 /* 읽은 만큼 차감 */
	bufp += nread;                  /* 버퍼 포인터 전진 */
    }
    return (n - nleft);             /* 실제로 읽은 바이트 수 (>= 0) */
}
/* $end rio_readn */

/*
 * rio_writen - short count 자동 처리하는 안전한 쓰기 (unbuffered)
 * rio_readn의 쓰기 버전. 구조 동일.
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* 시그널에 의한 중단 */
		nwritten = 0;    /* 다시 시도 */
	    else
		return -1;       /* 에러 */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/* 
 * rio_read - 버퍼 활용한 내부 읽기 헬퍼 (static이라 외부에서 못 씀)
 *
 * 핵심 아이디어: 커널에서 한 번에 큰 덩어리(8KB) 가져와 내부 버퍼에 저장
 * 이후 사용자 요청은 내부 버퍼에서 꺼내주어 시스템 콜 횟수 감소
 * 
 * 예:
 *   HTTP 요청 한 줄이 100바이트
 *   순진하게 하면: 100번 read() → 100번 시스템 콜
 *   rio_read 활용: 1번 read()로 8KB 가져오고, 내부에서 100번 메모리 접근
 *   → 시스템 콜 99번 절약
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    /* 내부 버퍼 비어있으면 커널에서 채우기 */
    while (rp->rio_cnt <= 0) {
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR)
		return -1;
	}
	else if (rp->rio_cnt == 0)       /* EOF */
	    return 0;
	else 
	    rp->rio_bufptr = rp->rio_buf; /* 버퍼 포인터 맨 앞으로 리셋 */
    }

    /* 내부 버퍼에서 min(n, rio_cnt) 바이트를 사용자 버퍼로 복사 */
    cnt = n;          
    if (rp->rio_cnt < n)   
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;              /* 버퍼 포인터 전진 */
    rp->rio_cnt -= cnt;                 /* 남은 바이트 수 감소 */
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - RIO 버퍼 초기화 (읽기 시작 전 한 번 호출)
 *
 * 한 연결(fd)마다 rio_t 하나씩 필요
 * 프록시에서는 "클라이언트 연결용"과 "서버 연결용" 두 개 쓸 수 있음
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;                /* 이 버퍼가 감시할 fd 지정 */
    rp->rio_cnt = 0;                /* 버퍼 비어있음 */
    rp->rio_bufptr = rp->rio_buf;   /* 버퍼 시작 위치 포인터 */
}
/* $end rio_readinitb */

/*
 * rio_readnb - 버퍼 활용한 n바이트 안전 읽기
 * rio_readn과 달리 내부 버퍼 활용 → 성능 향상
 * HTTP 응답 바디처럼 정해진 길이의 데이터 읽을 때 사용
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0) 
            return -1;
	else if (nread == 0)
	    break;                      /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);
}
/* $end rio_readnb */

/* 
 * rio_readlineb - 한 줄 읽기 (HTTP 파싱에 필수!)
 *
 * '\n'을 만날 때까지 한 바이트씩 읽음
 * 내부적으로는 rio_read를 써서 시스템 콜 횟수 최소화
 * 
 * HTTP 요청/응답이 라인 지향 프로토콜이라 필수
 *   "GET /home.html HTTP/1.1\r\n"을 한 번에 읽음
 *
 * 반환값:
 *   > 0: 읽은 바이트 수 (\n 포함, \0 제외)
 *   = 0: EOF (아무것도 못 읽음)
 *   < 0: 에러
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;                /* 사용자 버퍼에 저장 */
	    if (c == '\n') {            /* 줄바꿈 만나면 종료 */
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0;               /* EOF, 아무것도 못 읽음 */
	    else
		break;                  /* EOF, 일부는 읽음 */
	} else
	    return -1;	                /* 에러 */
    }
    *bufp = 0;                          /* 문자열 null 종결 */
    return n-1;                         /* 실제 읽은 바이트 수 */
}
/* $end rio_readlineb */

/**********************************
 * RIO 래퍼 (에러 시 종료 추가)
 **********************************/

/* Rio_readn - rio_readn + 에러 체크 */
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	unix_error("Rio_readn error");
    return n;
}

/* Rio_writen - rio_writen + 에러 체크 */
void Rio_writen(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
	unix_error("Rio_writen error");
}

/* Rio_readinitb - 단순 전달 (에러 없음) */
void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
} 

/* Rio_readnb - rio_readnb + 에러 체크 */
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	unix_error("Rio_readnb error");
    return rc;
}

/* Rio_readlineb - rio_readlineb + 에러 체크 */
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	unix_error("Rio_readlineb error");
    return rc;
} 

/******************************** 
 * 11. 클라이언트/서버 헬퍼 함수 (매우 중요!)
 *
 * 소켓 API 조합의 "완성판"
 * Tiny, Proxy에서 직접 호출하는 함수들
 ********************************/

/*
 * open_clientfd - 서버에 연결하여 통신 가능한 소켓 fd 반환
 *
 * 내부 흐름:
 *   1. getaddrinfo로 호스트명+포트 → 주소 리스트
 *   2. 리스트 순회하며 socket + connect 시도
 *   3. 첫 성공에서 탈출, 실패면 다음 주소 시도
 *   4. 모두 실패면 -1
 *
 * Protocol-independent (IPv4/IPv6 모두 자동 처리)
 *
 * 반환값:
 *   >= 0: 연결 성공한 소켓 fd
 *   -1: 연결 실패 (errno 설정됨)
 *   -2: getaddrinfo 실패
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, char *port) {
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* getaddrinfo 힌트 설정: "어떤 주소 원하는지" 필터 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* TCP 소켓 원함 */
    hints.ai_flags = AI_NUMERICSERV;  /* port가 "80" 같은 숫자임을 보장 */
    hints.ai_flags |= AI_ADDRCONFIG;  /* 호스트가 지원하는 프로토콜만 반환 */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }
  
    /* 주소 리스트 순회: 성공할 때까지 시도 */
    for (p = listp; p; p = p->ai_next) {
        /* 1단계: 소켓 생성 */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue; /* 실패 → 다음 주소 */

        /* 2단계: 서버에 연결 */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) 
            break; /* 성공 → 루프 탈출 */
        if (close(clientfd) < 0) { /* 연결 실패 → 소켓 닫고 다음 */  //line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        } 
    } 

    /* getaddrinfo가 할당한 리스트 해제 (메모리 누수 방지) */
    freeaddrinfo(listp);
    if (!p) /* 모든 시도 실패 */
        return -1;
    else    /* 마지막 시도 성공 */
        return clientfd;
}
/* $end open_clientfd */

/*  
 * open_listenfd - 포트에서 대기하는 리스닝 소켓 생성
 *
 * 내부 흐름:
 *   1. getaddrinfo(NULL, port, ...) → 서버용 주소
 *      (NULL: 모든 인터페이스에서 수신)
 *   2. 리스트 순회하며 socket + setsockopt + bind 시도
 *   3. bind 성공 소켓을 listen으로 전환
 *
 * 반환값:
 *   >= 0: 리스닝 소켓 fd
 *   -1: bind 실패
 *   -2: getaddrinfo 실패
 */
/* $begin open_listenfd */
int open_listenfd(char *port) 
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    /* 힌트 설정 */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* 연결 받을 TCP */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* AI_PASSIVE: 서버용 */
    hints.ai_flags |= AI_NUMERICSERV;            /* port는 숫자 */
    
    /* 서버이므로 node는 NULL (모든 IP에서 수신) */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* 주소 리스트 순회: 성공적으로 bind 되는 주소 찾기 */
    for (p = listp; p; p = p->ai_next) {
        /* 1단계: 소켓 생성 */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;

        /* "Address already in use" 에러 방지 (TIME_WAIT 우회) */
        /* 서버 종료 후 바로 재시작 가능하게 */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));

        /* 2단계: 소켓에 주소 부여 */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* 성공 */
        if (close(listenfd) < 0) { /* bind 실패 → 소켓 닫고 다음 */
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }


    /* 리스트 해제 */
    freeaddrinfo(listp);
    if (!p) /* 어느 주소도 안 됨 */
        return -1;

    /* 3단계: bind 성공 소켓을 listen 모드로 전환 */
    /* LISTENQ는 csapp.h에 정의된 백로그 큐 크기 (1024) */
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
	return -1;
    }
    return listenfd;
}
/* $end open_listenfd */

/****************************************************
 * open_clientfd / open_listenfd의 래퍼
 * (소문자 버전에 에러 체크 추가)
 ****************************************************/

/*
 * Open_clientfd - open_clientfd의 대문자 래퍼
 * Tiny/Proxy 클라이언트 사용 시 이걸 호출
 * 실패 시 에러 메시지 + 종료
 */
int Open_clientfd(char *hostname, char *port) 
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0) 
	unix_error("Open_clientfd error");
    return rc;
}

/*
 * Open_listenfd - open_listenfd의 대문자 래퍼
 * Tiny/Proxy 서버 기동 시 이걸 호출
 */
int Open_listenfd(char *port) 
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
	unix_error("Open_listenfd error");
    return rc;
}

/* $end csapp.c */

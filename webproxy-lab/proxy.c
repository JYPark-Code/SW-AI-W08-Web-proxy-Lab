/*
 * proxy.c - A simple HTTP proxy server (Part 1: Sequential)
 *
 * 흐름:
 *   1. 포트에서 리스닝
 *   2. 브라우저 접속 받음
 *   3. 브라우저의 HTTP 요청을 파싱
 *   4. URL에서 host/port/path 추출
 *   5. 요청 헤더 재작성
 *   6. 실제 서버(Tiny 등)에 접속
 *   7. 재작성된 요청 전송
 *   8. 응답 받아서 브라우저로 그대로 전달
 *   9. 연결 종료, 다음 요청 대기
 */
#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* 함수 프로토타입 */
void doit(int clientfd);
int parse_uri(char *uri, char *hostname, char *port, char *path);
void build_requesthdrs(rio_t *client_rio, char *newreq, 
                        char *hostname, char *port, char *path);

int main(int argc, char **argv)
{
    int listenfd, clientfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* 명령줄 인자 검증 */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* 리스닝 소켓 생성 (Tiny와 동일) */
    listenfd = Open_listenfd(argv[1]);

    /* 요청 받는 무한 루프 */
    while (1) {
        clientlen = sizeof(clientaddr);
        clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        /* 접속한 클라이언트 정보 로그 */
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        
        /* 요청 처리 (핵심!) */
        doit(clientfd);
        
        /* 연결 종료 */
        Close(clientfd);
    }
    return 0;
}

/*
 * doit - 한 HTTP 요청을 중계
 * 
 * 할 일:
 *   1. 브라우저 요청 라인 읽기 (GET http://... HTTP/1.1)
 *   2. parse_uri로 host/port/path 추출
 *   3. build_requesthdrs로 새 요청 조립
 *   4. Open_clientfd로 실제 서버에 접속
 *   5. 새 요청 전송
 *   6. 응답 받아서 브라우저로 전달
 */
void doit(int clientfd)
{
    /* TODO: doit 함수 채우기 */
    // printf("doit called\n");
    char buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    char newreq[MAXLINE];          // 백엔드에 보낼 요청
    int serverfd;
    ssize_t n;
    rio_t client_rio, server_rio;


    /* 1. 브라우저 요청 라인 읽기 */
    Rio_readinitb(&client_rio, clientfd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE))
        return;
    printf("Request line: %s", buf);

    /* 2. 요청 라인 파싱 */
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
        printf("Not GET method, ignoring\n");
        return;
    }
    printf("URI: %s\n", uri);

    /* 3. URL 파싱 (parse_uri는 아직 빈 함수, 나중에 채움) */
    if (parse_uri(uri, hostname, port, path) < 0) {
        printf("parse_uri failed\n");
        return;
    }

    /* 4. 새 요청 조립 (build_requesthdrs도 아직 빈 함수) */
    build_requesthdrs(&client_rio, newreq, hostname, port, path);

    /* 5. 백엔드 서버에 접속 */
    serverfd = Open_clientfd(hostname, port);
    Rio_readinitb(&server_rio, serverfd);

    /* 6. 새 요청 전송 */
    Rio_writen(serverfd, newreq, strlen(newreq));

    /* 7. 응답 받아서 브라우저로 그대로 전달 */
    while ((n = Rio_readnb(&server_rio, buf, MAXLINE)) > 0) {
        Rio_writen(clientfd, buf, n);
    }

    /* 8. 백엔드 연결 종료 */
    Close(serverfd);
}

/*
 * parse_uri - "http://host:port/path" 형태의 URL을 분해
 * 
 * 입력 예:
 *   uri = "http://www.cmu.edu:8080/index.html"
 * 출력:
 *   hostname = "www.cmu.edu"
 *   port     = "8080"
 *   path     = "/index.html"
 * 
 * port 생략 시: "http://example.com/page" → port = "80"
 * path 생략 시: "http://example.com"      → path = "/"
 * 
 * 성공: 0 반환, 실패: -1 반환
 */
int parse_uri(char *uri, char *hostname, char *port, char *path)
{
    /* TODO: 여기를 채워야 함 */
    return 0;
}

/*
 * build_requesthdrs - 백엔드로 보낼 HTTP 요청 조립
 * 
 * 입력:
 *   client_rio: 브라우저 쪽 RIO 버퍼 (나머지 헤더 읽기용)
 *   hostname, port, path: parse_uri 결과
 * 
 * 출력 (newreq에 조립):
 *   "GET /path HTTP/1.0\r\n"
 *   "Host: hostname:port\r\n"
 *   "User-Agent: 고정문자열\r\n"
 *   "Connection: close\r\n"
 *   "Proxy-Connection: close\r\n"
 *   (브라우저가 보낸 기타 헤더들 그대로)
 *   "\r\n"
 */
void build_requesthdrs(rio_t *client_rio, char *newreq, 
                        char *hostname, char *port, char *path)
{
    /* TODO: */
    newreq[0] = '\0';  // 빈 문자열로 초기화 (Rio_writen이 0바이트 쓰도록)
}

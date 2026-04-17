#include "csapp.h"

// 연결된 클라이언트와의 대화를 처리하는 echo 함수
// connfd: Accept가 반환한 클라이언트 전용 소켓 fd
void echo(int connfd)
{
    size_t n;             // 이번 라운드에 읽은 바이트 수
    char buf[MAXLINE];    // 읽은 데이터를 임시 저장할 버퍼 (8192 바이트)
    rio_t rio;            // 이 연결 전용 RIO 버퍼 구조체
                          // connfd마다 독립된 rio_t가 필요 (버퍼 꼬임 방지)

    // rio 구조체를 connfd와 연결. 내부 버퍼 상태 초기화
    // (rio_cnt=0, rio_bufptr=buf 맨 앞으로)
    Rio_readinitb(&rio, connfd);
    
    // 클라이언트가 연결 끊을 때까지 계속 읽기/쓰기 반복
    // Rio_readlineb의 반환값:
    //   > 0 : 읽은 바이트 수 (\n까지 포함)
    //   = 0 : EOF. 클라이언트가 close했거나 Ctrl+D 눌러서 연결 종료
    //   < 0 : 에러 (대문자 래퍼라 이 경우는 내부에서 처리하고 종료됨)
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
        
        // 서버 터미널에 로그 찍기. 클라이언트에는 가지 않음
        // (printf는 stdout = fd 1, 소켓은 connfd → 서로 다른 fd)
        printf("server received %d bytes\n", (int)n);
        
        // 읽은 바이트 수(n)만큼 정확히 그대로 다시 써줌
        // 이게 "echo" 동작의 본체 — 받은 대로 돌려주기
        // Rio_writen이 short count 발생해도 루프 돌며 n바이트 모두 전송 보장
        Rio_writen(connfd, buf, n);
    }
    
    // while이 종료되면 자연스럽게 함수 끝
    // main으로 돌아가서 Close(connfd)로 소켓 닫힘
}
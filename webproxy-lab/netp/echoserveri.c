#include "csapp.h"

// echo 함수의 전방 선언 (실제 구현은 echo.c에 있음)
// 이 선언 덕분에 main에서 echo(connfd) 호출이 가능
void echo(int connfd);

int main(int argc, char **argv){

    int listenfd, connfd;                // listenfd: 연결 대기용, connfd: 실제 통신용
    socklen_t clientlen;                 // 클라이언트 주소 구조체 크기 저장용
    struct sockaddr_storage clientaddr;  // 클라이언트 주소 담을 구조체
                                         // sockaddr_in(IPv4)이 아닌 _storage 쓰는 이유:
                                         // IPv4/IPv6 둘 다 담을 수 있을 만큼 커서 프로토콜 독립적
    char client_hostname[MAXLINE], client_port[MAXLINE];  // 사람이 읽을 수 있는 형태로 변환된 주소 저장용

    // 인자 검증: ./echoserveri <port> 형태여야 함
    if(argc != 2){
        fprintf(stderr, "usage : %s <port>\n", argv[0]);
        exit(0);
    }

    // 리스닝 소켓 생성: socket + bind + listen 한 방에 처리하는 CSAPP 래퍼
    // 성공 시 이 fd로 연결 요청을 받을 수 있는 상태가 됨
    // 이 시점 이후 커널은 백그라운드에서 3-way handshake 처리 및 백로그 큐 관리 시작
    listenfd = Open_listenfd(argv[1]);

    // iterative 서버의 메인 루프: 한 클라이언트씩 순차 처리
    // (동시에 여러 클라이언트 처리하려면 12장의 thread/process 서버가 필요)
    while(1) {
        // accept 호출 전 clientlen을 반드시 세팅해야 함
        // accept가 이 값을 읽어서 "주소 구조체의 최대 크기"로 사용
        // (반환 시에는 실제 채워진 크기로 업데이트됨)
        clientlen = sizeof(struct sockaddr_storage);
        
        // 백로그 큐에서 연결 하나 꺼냄 → 새 fd(connfd) 반환
        // 큐가 비어있으면 여기서 블록(대기). 클라이언트가 connect해야 깨어남
        // clientaddr에 클라이언트 주소 정보가 채워짐
        // SA는 typedef된 `struct sockaddr`. 캐스팅으로 제네릭 타입 맞춰줌
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        // clientaddr(바이트 형태 주소) → 사람이 읽을 수 있는 호스트명/포트로 변환
        // IP 주소 숫자를 "localhost"나 "example.com"으로, 포트 숫자를 서비스명으로 변환
        // (flags=0이면 기본 동작: 도메인명 변환 시도)
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                            client_port, MAXLINE, 0);
        
        // 누가 접속했는지 서버 로그에 출력
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        
        // 실제 echo 기능 수행 (echo.c에 정의됨)
        // 한 클라이언트의 모든 입력을 처리할 때까지 여기서 머무름
        // 이게 "iterative" 서버의 한계 — echo가 진행되는 동안 다른 클라이언트는 대기
        echo(connfd);
        
        // 클라이언트와의 통신 끝났으니 connfd 닫기
        // listenfd는 여전히 살아있어서 다음 accept 가능
        Close(connfd);
    }
    
    exit(0);
}
#include "csapp.h"  // CSAPP 책의 헬퍼 라이브러리. socket, RIO 등 모든 래퍼 여기 있음

int main(int argc, char **argv){
    
    int clientfd;              // 서버와 연결된 소켓의 파일 디스크립터
    char *host, *port, buf[MAXLINE];  // host/port 인자, 입출력 버퍼(8192 바이트)
    rio_t rio;                 // RIO 버퍼 구조체 (아래에서 자세히 설명)

    // 인자 검증: ./echoclient <host> <port> 형태여야 함
    if(argc != 3){
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];            // 첫 인자 = 접속할 호스트명 (예: "localhost")
    port = argv[2];            // 두번째 인자 = 포트번호 (문자열로 받음, 예: "15213")


    // getaddrinfo + socket + connect를 한 번에. 성공하면 서버와 연결된 fd 반환
    // Python: socket.create_connection((host, int(port))) 와 유사
    clientfd = Open_clientfd(host, port);

    
    // clientfd에서 읽기 작업을 RIO 버퍼 방식으로 하겠다고 초기화
    // (rio 구조체 안에 버퍼/상태 필드를 준비하는 과정)
    Rio_readinitb(&rio, clientfd);

    
    // 표준입력에서 한 줄 받을 때마다 서버로 보내고, 응답 받아서 출력하는 루프
    // Ctrl+D (EOF) 누르면 Fgets가 NULL 반환하며 종료
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        
        // 내가 친 문자열을 서버로 전송. 정확히 strlen(buf) 바이트 모두 전송 보장
        // (rio_writen은 short count 발생하면 루프 돌며 나머지도 마저 전송)
        Rio_writen(clientfd, buf, strlen(buf));
        
        // 서버로부터 한 줄 수신 (\n까지). RIO 버퍼 덕분에 효율적으로 동작
        Rio_readlineb(&rio, buf, MAXLINE);
        
        // 받은 내용을 표준출력에 출력 (= 화면에 보이게)
        Fputs(buf, stdout);
    }

    Close(clientfd);   // 소켓 닫기 → TCP 4-way handshake로 연결 종료
    exit(0);

}
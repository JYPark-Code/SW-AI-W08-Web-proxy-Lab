/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "../csapp.h"

int main(void)
{
  char *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  char *method = getenv("REQUEST_METHOD");
  char query[MAXLINE] = "";   /* GET/POST 공통 저장소, 빈 문자열로 초기화 */
  int n1 = 0, n2 = 0;

  /* 11.10: method에 따라 query를 어디서 채울지 분기 */
  if (method && !strcasecmp(method, "POST")) {
    /* POST: stdin에서 CONTENT_LENGTH 바이트 읽기 */
    char *cl = getenv("CONTENT_LENGTH");
    int n = cl ? atoi(cl) : 0;
    if (n >= MAXLINE) n = MAXLINE - 1;
    if (n > 0) {
      if (fgets(query, n + 1, stdin) == NULL)
        query[0] = '\0';  /* 읽기 실패 시 빈 문자열 유지 */
    }
  } else {
    /* GET: QUERY_STRING 환경변수 */
    char *buf = getenv("QUERY_STRING");
    if (buf != NULL)
      strncpy(query, buf, MAXLINE - 1);
  }

  /* 파싱은 GET/POST 공통 경로: "n1=15213&n2=18213" 형태를 가정 */
  if ((p = strchr(query, '&')) != NULL) {
    *p = '\0';
    strcpy(arg1, query);
    strcpy(arg2, p + 1);
    if (strchr(arg1, '=')) n1 = atoi(strchr(arg1, '=') + 1);
    if (strchr(arg2, '=')) n2 = atoi(strchr(arg2, '=') + 1);
    *p = '&';  /* 표시용으로 원상 복구 (QUERY_STRING 출력 시 온전하게) */
  }

  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s\r\n<p>", query);
  sprintf(content + strlen(content), "Welcome to add.com: ");
  sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
  sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>",
          n1, n2, n1 + n2);
  sprintf(content + strlen(content), "Thanks for visiting!\r\n");

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n");
  printf("\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
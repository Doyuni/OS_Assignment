#define _GNU_SOURCE // for strtok_r 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
// #include <sys/time.h>
#define HASHMAX 210000000
typedef struct HashNode
{
  char *word;
  int frequency;
  struct HashNode *link;
} HashNode;

int global_count = 0;
HashNode *HashTable[HASHMAX] = {
    0,
};
static char *buf;

const int num_thread = 4;
static pthread_mutex_t insert_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t visitor_mutex = PTHREAD_MUTEX_INITIALIZER;

int file_size = 0;
int portion_size = 0;

FILE *fp;
const char *WHITE_SPACE = " \t\n";
static char *tok;
char *last;

unsigned int HashFunc(const char *word) // 해시 함수
{
  unsigned int index = 0;
  int n = strlen(word);
  for (int i = 0; i < n; i++) {
    index = 31 * index + word[i]; // 31(소수)을 곱하여 가장 바르게 계산 가능
  }
  return index % HASHMAX;
}

void InsertWord(const char *word)
{
  HashNode *ptr;
  int index = HashFunc(word);
  for (ptr = HashTable[index]; ptr != NULL; ptr = ptr->link)
  { 
    if (!strcmp(word, ptr->word))
    {
      (ptr->frequency)++;
      return;
    }
  }
  global_count++;
  ptr = (HashNode *)malloc(sizeof(HashNode)); 
  ptr->frequency = 1;
  ptr->word = (char *)malloc(strlen(word) + 1);
  strcpy(ptr->word, word);
  ptr->link = HashTable[index];
  HashTable[index] = ptr;
}

void *parallel_str_transform(void *arg)
{
  int start, end;
  int tid = *(int *)(arg); // thread id
  start = tid * portion_size;
  end = (tid + 1) * portion_size;
  // matrix 곱셈 병렬화 응용
  for (int i = start; i < end; ++i)
  {
    if (!isalnum(buf[i]))
    {
      buf[i] = ' ';
    }
    else
    {
      buf[i] = tolower(buf[i]);
    }
  }
  if (tid == num_thread - 1)
  { // 나머지 처리 (스레드 개수랑 나누어 떨어지지 않을 때)
    start = end;
    end = file_size;
    for (int i = start; i < end; ++i)
    {
      if (!isalnum(buf[i]))
      {
        buf[i] = ' ';
      }
      else
      {
        buf[i] = tolower(buf[i]);
      }
    }
  }
  pthread_exit(0);
}

void *parallel_str_token(void *arg)
{
  int tid = *(int *)(arg); // thread id : parallel test용
  do
  {
    pthread_mutex_lock(&visitor_mutex); // 방문 락
    tok = strtok_r(NULL, WHITE_SPACE, &last);
    if (tok)
    {
      // printf("thread %d : %s\n",tid, tok); // test용
      pthread_mutex_lock(&insert_mutex); // word 삽입 락
      if(isalnum(tok[0])) { // owl.txt에 오류나서 추가했습니다. - 오류해결
        InsertWord(tok); // hashtable에 추가
      }
      pthread_mutex_unlock(&insert_mutex); // word 락 해제
    }
    pthread_mutex_unlock(&visitor_mutex); // 방문 락 해제
  } while (tok);
  pthread_exit(0);
}

static int compareWordCount(const void *left, const void *right)
{ // 빈도수 비교
  int cmp = (*(const HashNode **)right)->frequency - (*(const HashNode **)left)->frequency;
  if (cmp == 0) // 빈도수 같을 때
  { // 사전 순 비교
    cmp = strcmp((*(const HashNode **)right)->word, (*(const HashNode **)left)->word);
    cmp = cmp * -1; // 알파벳 순서
  }
  return cmp;
}

void printWord()
{
  HashNode **list = NULL;
  unsigned int i = 0;
  unsigned int index = 0;

  list = malloc(global_count * sizeof(HashNode *));
  // list에 node 연결
  for (i = 0, index = 0; i < HASHMAX; ++i)
  {
    HashNode *ptr = HashTable[i];
    while (ptr)
    {
      list[index++] = ptr;
      ptr = ptr->link;
    }
  }

  // quick sort, compareWordCount 기준으로 list 정렬
  qsort(list, index, sizeof(HashNode *), compareWordCount);

  for (i = 0; i < index; i++) {
    printf("%s %d\n", list[i]->word, list[i]->frequency);
  }
  free(list);
}

int main(int argc, char **argv)
{
  // struct timeval tstart, tend;
  // double exectime;
  // gettimeofday(&tstart, NULL); // start time
  if (argc != 2)
  {
    fprintf(stderr, "%s: not enough input\n", argv[0]);
    exit(1);
  }

  fp = fopen(argv[1], "r");
  if (!fp)
  {
    fprintf(stderr, "Failed to open %s\n", argv[1]);
    exit(1);
  }

  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp); // 전체 파일 크기
  // printf("file size : %d\n", file_size); // for test
  fseek(fp, 0, SEEK_SET); // 파일 포인터 처음으로 설정

  portion_size = file_size / num_thread;

  buf = malloc(sizeof(char) * file_size);
  last = malloc(sizeof(char) * file_size); // for strtok_r 
  fread(buf, file_size, 1, fp); // 전체 파일 읽기
 
  pthread_t pid[num_thread];

  // preprocessing
  for (int i = 0; i < num_thread; ++i)
  {
    int *tid;
    tid = (int *)malloc(sizeof(int));
    *tid = i;
    pthread_create(&pid[i], NULL, parallel_str_transform, (void *)tid);
  }

  for (int i = 0; i < num_thread; ++i)
  {
    pthread_join(pid[i], NULL);
  }

  // tokenizing
  tok = strtok_r(buf, WHITE_SPACE, &last);
  InsertWord(tok); // 첫 번째 단어 삽입
  for (int i = 0; i < num_thread; ++i)
  {
    int *tid;
    tid = (int *)malloc(sizeof(int));
    *tid = i;
    pthread_create(&pid[i], NULL, parallel_str_token, (void *)tid);
  }

  for (int i = 0; i < num_thread; ++i)
  {
    pthread_join(pid[i], NULL);
  }

  // print
  printWord();

  // gettimeofday(&tend, NULL); // end time
  // exectime = (tend.tv_sec - tstart.tv_sec) * 1000.0;    // sec to ms
  // exectime += (tend.tv_usec - tstart.tv_usec) / 1000.0; // us to ms
  // printf("Execution time:%.3lf sec\n", exectime / 1000.0);

  fclose(fp);

    return 0;
  }

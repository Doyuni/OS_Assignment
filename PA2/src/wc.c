#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/queue.h>
#include <pthread.h>
#include <sys/time.h>
// sys queue 부분
static LIST_HEAD(listhead, entry) head; // singly linked list (tail이 null로 끝남)

struct listhead *headp = NULL;
// 여기까지 head define

int num_entries = 0;

struct entry
{
  char name[BUFSIZ];
  int frequency;
  LIST_ENTRY(entry)
  entries; // link를 의미 entries 구조체
};

//
static char *buf;
static int buf_len = 0;

static char *buf_for_parallel;

static char buf2_for_parallel[4096] = {
    0,
};
const int k_num_thread = 4;
static int counter = 0;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

int file_size = 0;
int portion_size = 0;
int extra_size = 0;
FILE *fp;

void *parallel_str_transform(void *arg)
{
  int portion_size, start, end;
  int tid = *(int *)(arg); // get the thread ID assigned sequentially.
  start = tid * portion_size;
  end = (tid + 1) * portion_size;
  // pthread_mutex_lock(&counter_mutex);
  // pthread_mutex_unlock(&counter_mutex);
  for (int i = start; i < end; ++i)
  {
    if (!isalnum(buf[i]))
    {
      buf_for_parallel[i] = ' ';
    }
    else
    {
      buf_for_parallel[i] = tolower(buf[i]);
    }
  }
  if (tid == k_num_thread - 1)
  { // 나머지 처리 (스레드 개수랑 나누어 떨어지지 않을 때)
    start = end;
    end = file_size;
    for (int i = start; i < end; ++i)
    {
      if (!isalnum(buf[i]))
      {
        buf_for_parallel[i] = ' ';
      }
      else
      {
        buf_for_parallel[i] = tolower(buf[i]);
      }
    }
  }
  pthread_exit(0);
}

// void *parallel_str_token(void *changed_buf)
// {
//   strcpy(buf2_for_parallel, (char *)changed_buf);

//   pthread_mutex_lock(&counter_mutex);
//   int id = counter++;
//   pthread_mutex_unlock(&counter_mutex);

//   tok = strtok(buf2_for_parallel, WHITE_SPACE);
//   do
//   {
//     if (tok == NULL || strcmp(tok, "") == 0)
//       continue;
//     printf("thread %d tok : %s\n", id, tok);
//   } while (tok = strtok(NULL, WHITE_SPACE));
// }

int main()
{
  fp = fopen("./data/therepublic.txt", "r");

  if (!fp)
  {
    fprintf(stderr, "Failed to open ./data/strings.txt\n");
    exit(1);
  }
  struct timeval tstart, tend;
  double exectime;

  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  portion_size = file_size / k_num_thread;
  extra_size = file_size % k_num_thread;
  // printf("file size : %d\n", file_size);
  buf_for_parallel = malloc(sizeof(char) * file_size);
  buf = malloc(sizeof(char) * file_size);
  fseek(fp, 0, SEEK_SET);

  fread(buf, file_size, 1, fp);
  // Parallel
  gettimeofday( &tstart, NULL );
  pthread_t pid[k_num_thread];
  
  for (int i = 0; i < k_num_thread; ++i)
  {
    int *tid;
    tid = (int *)malloc(sizeof(int));
    *tid = i;
    pthread_create(&pid[i], NULL, parallel_str_transform, (void *)tid);
  }

  for (int i = 0; i < k_num_thread; ++i)
  {
    pthread_join(pid[i], NULL);
  }

  // for (int i = 0; i < k_num_thread; ++i)
  // {
  //   pthread_create(&pid[i], NULL, parallel_str_token, (void*)buf_for_parallel);
  // }

  // for (int i = 0; i < k_num_thread; ++i)
  // {
  //   pthread_join(pid[i], NULL);
  // }

  // printf("%s\n", buf_for_parallel);

  LIST_INIT(&head);

  const char *WHITE_SPACE = " \t\n";
  char* tok = strtok(buf_for_parallel, WHITE_SPACE);

  //token

  do
  {
    // 이걸 더 깔끔하게 바꿀 수 있을 것임
    	if (tok == NULL || strcmp(tok, "") == 0) {
			continue;
		}

    if (num_entries == 0)
    {
      struct entry *e = malloc(sizeof(struct entry));

      strncpy(e->name, tok, strlen(tok)); // copy string
      e->frequency = 1;

      LIST_INSERT_HEAD(&head, e, entries);
      num_entries++;
      continue;
    }
    else if (num_entries == 1)
    {
      int cmp = strcmp(tok, head.lh_first->name);

      if (cmp == 0)
      {
        head.lh_first->frequency++;
      }
      else if (cmp > 0)
      {
        struct entry *e = malloc(sizeof(struct entry));

        strncpy(e->name, tok, strlen(tok));
        e->frequency = 1;

        LIST_INSERT_AFTER(head.lh_first, e, entries);
        num_entries++;
      }
      else if (cmp < 0)
      {
        struct entry *e = malloc(sizeof(struct entry));

        strncpy(e->name, tok, strlen(tok));
        e->frequency = 1;

        LIST_INSERT_HEAD(&head, e, entries);
        num_entries++;
      }

      continue;
    }

    // Reduce: actual word-counting
    struct entry *np = head.lh_first;
    struct entry *final_np = NULL;

    int last_cmp = strcmp(tok, np->name); // 삽입정렬을 사용하였기에 아래 부분(앞에가 정렬되어 있다는 전제)

    if (last_cmp < 0)
    { // tok이 name의 이전에 와야한다.
      struct entry *e = malloc(sizeof(struct entry));

      strncpy(e->name, tok, strlen(tok));
      e->frequency = 1;

      LIST_INSERT_HEAD(&head, e, entries);
      num_entries++;

      continue;
    }
    else if (last_cmp == 0)
    {
      np->frequency++;

      continue;
    }
    // 삽입 정렬 -> 병렬화를 했을 때 가능한지 봐야함
    for (np = np->entries.le_next; np != NULL; np = np->entries.le_next)
    {
      int cmp = strcmp(tok, np->name);

      if (cmp == 0)
      {
        np->frequency++;
        break;
      }
      else if (last_cmp * cmp < 0)
      { // sign-crossing occurred,  A B D E 에 C를 넣고 싶을 때  cmp하면 1 1 -1 에서 1 * -1이면 즉 부호가 바뀜
        struct entry *e = malloc(sizeof(struct entry));

        strncpy(e->name, tok, strlen(tok));
        e->frequency = 1;

        LIST_INSERT_BEFORE(np, e, entries);
        num_entries++;

        break;
      }

      if (np->entries.le_next == NULL)
      {
        final_np = np;
      }
      else
      {
        last_cmp = cmp;
      }
    }

    if (!np && final_np)
    { // 맨 마지막에 넣어줌, 마지막 tail이 NULL이기에 prev를 찾을 방법이 필요
      struct entry *e = malloc(sizeof(struct entry));

      strncpy(e->name, tok, strlen(tok));
      e->frequency = 1;

      LIST_INSERT_AFTER(final_np, e, entries);
      num_entries++;
    }
  } while (tok = strtok(NULL, WHITE_SPACE));

  int max_frequency = 0;

  for (struct entry* np = head.lh_first; np != NULL; np = np->entries.le_next) {
    if (max_frequency < np->frequency) {
      max_frequency = np->frequency;
    }
  }
  // print할 수 있는 방법은 많다. qsort 함수 쓰는게 좋을 수 있음 하지만 unstable sort이기에 잘 생각해야함
  // stable sort: D E A-34 A-4 B를 알파벳 순서대로 정렬 A-34 A-4 B D E를 보장, unstable sort는 보장 안함
  // red-black tree도 가능
  for (int it = max_frequency; it > 0; --it) {
    for (struct entry* np = head.lh_first; np != NULL; np = np->entries.le_next) {
      if (np->frequency == it) {
        printf("%s %d\n", np->name, np->frequency);
      }
    }
  }
  gettimeofday( &tend, NULL );
  exectime = (tend.tv_sec - tstart.tv_sec) * 1000.0; // sec to ms
  exectime += (tend.tv_usec - tstart.tv_usec) / 1000.0; // us to ms   
  printf( "Number of threads: %d\tExecution time:%.3lf sec\n",
          k_num_thread, exectime/1000.0);
  // Release
  while (head.lh_first != NULL) {
    LIST_REMOVE(head.lh_first, entries);
  }
  
  fclose(fp);

  return 0;
}


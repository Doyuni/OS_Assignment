#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/queue.h>
// gdb로 debug해보는게 좋다.
// 멀티 프로세싱 디버그는 힘들기에 각 스레드마다 log를 남기는게 좋다. print 할 때 시간을 정확히 찍는 식으로

// static 
static LIST_HEAD(listhead, entry) head; // singly linked list (tail이 null로 끝남)

struct listhead *headp = NULL;
// 여기까지 head define

int num_entries = 0;

struct entry {
	char name[BUFSIZ];
	int frequency;
	LIST_ENTRY(entry) entries; // link를 의미 entries 구조체
};

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "%s: not enough input\n", argv[0]);
		exit(1);
	}

	FILE* fp = fopen(argv[1], "r");
	char buf[4096];

	LIST_INIT(&head); // list 사용 가능
// 단계별로 나누어서 단계별로 병렬화 시켜도 될듯
// 1. File read
// 2. Preprocessing
// 3. Tokenizing -> 지금은 naive, 원래 형태소 분석하면서 해야함
// 4. Counting
// 5. Print
// pipelining(단, 4-5단계는 하기 힘들다.)
// MapReduce
// thread = 2
// t0
// A 1
// B 1
// B 1
// -> A : 1 B : 2
// t1
// A 1
// B 1
// C 1
// C 1
// -> A : 1 B : 1 C : 2

// 이걸 또 섞어

// t0'
// A : 1
// C : 2

// t1'
// A : 1
// B : 2
// B : 1
// -> A : 1 B : 3
	while (fgets(buf, 4096, fp)) {
		// Preprocess: change all non-alnums into white-space,
		//             alnums to lower-case.
		int line_length = strlen(buf);

		for (int it = 0; it < line_length; ++it) {
			if (!isalnum(buf[it])) {
				buf[it] = ' ';
			} else {
				buf[it] = tolower(buf[it]);
			}
		}

    // Tokenization
		const char* WHITE_SPACE =" \t\n";
		char* tok = strtok(buf, WHITE_SPACE);

		if (tok == NULL || strcmp(tok, "") == 0) {
			continue;
		}

		do {
      // 이걸 더 깔끔하게 바꿀 수 있을 것임
      if (num_entries == 0) {
        struct entry* e = malloc(sizeof(struct entry));

        strncpy(e->name, tok, strlen(tok)); // copy string
        e->frequency = 1;

        LIST_INSERT_HEAD(&head, e, entries);
        num_entries++;
        continue;
      } else if (num_entries == 1) {
        int cmp = strcmp(tok, head.lh_first->name);

        if (cmp == 0) {
          head.lh_first->frequency++;
        } else if (cmp > 0) {
          struct entry* e = malloc(sizeof(struct entry));

          strncpy(e->name, tok, strlen(tok));
          e->frequency = 1;


          LIST_INSERT_AFTER(head.lh_first, e, entries);
          num_entries++;
        } else if (cmp < 0) {
          struct entry* e = malloc(sizeof(struct entry));

          strncpy(e->name, tok, strlen(tok));
          e->frequency = 1;

          LIST_INSERT_HEAD(&head, e, entries);
          num_entries++;
        }

        continue;
      }

      // Reduce: actual word-counting
      struct entry* np = head.lh_first;
      struct entry* final_np = NULL;

      int last_cmp = strcmp(tok, np->name); // 삽입정렬을 사용하였기에 아래 부분(앞에가 정렬되어 있다는 전제) 

      if (last_cmp < 0) { // tok이 name의 이전에 와야한다.
        struct entry* e = malloc(sizeof(struct entry));

        strncpy(e->name, tok, strlen(tok));
        e->frequency = 1;

        LIST_INSERT_HEAD(&head, e, entries);
        num_entries++;

        continue;

      } else if (last_cmp == 0) {
        np->frequency++;

        continue;
      }
      // 삽입 정렬 -> 병렬화를 했을 때 가능한지 봐야함
      for (np = np->entries.le_next; np != NULL; np = np->entries.le_next) {
        int cmp = strcmp(tok, np->name);

        if (cmp == 0) {
          np->frequency++;
          break;
        } else if (last_cmp * cmp < 0) { // sign-crossing occurred,  A B D E 에 C를 넣고 싶을 때  cmp하면 1 1 -1 에서 1 * -1이면 즉 부호가 바뀜
          struct entry* e = malloc(sizeof(struct entry));

          strncpy(e->name, tok, strlen(tok));
          e->frequency = 1;

          LIST_INSERT_BEFORE(np, e, entries);
          num_entries++;

          break;
        }

        if (np->entries.le_next == NULL) {
          final_np = np;
        } else {
          last_cmp = cmp;
        }
      }

      if (!np && final_np) { // 맨 마지막에 넣어줌, 마지막 tail이 NULL이기에 prev를 찾을 방법이 필요
        struct entry* e = malloc(sizeof(struct entry));

        strncpy(e->name, tok, strlen(tok));
        e->frequency = 1;

        LIST_INSERT_AFTER(final_np, e, entries);
        num_entries++;
      }
    } while (tok = strtok(NULL, WHITE_SPACE));
  }

  // 위의 word counting이 더 중요...
  // Print the counting result very very slow way.
  // 병렬화하면 print 구문은 어차피 바꿔야 함
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

  // Release
  while (head.lh_first != NULL) {
    LIST_REMOVE(head.lh_first, entries);
  }

  fclose(fp);

  return 0;
}
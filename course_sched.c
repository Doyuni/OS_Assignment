#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_LENGTH 4096

struct CourseEntry {
	char name[MAX_LENGTH]; // ������ �̸�
	struct CourseEntry* prerequisites; // ���� �����
	int n_prerequisites; // ���� ������ ��
	float difficulty; // ������ ���̵�
};

typedef struct node *nodePointer;
typedef struct node {
	int vertex;
	struct CourseEntry lecture;
}node;

typedef struct Queue{
	int max;
	int num; // ���� ����� ����
	int front;
	int rear;
	int *que;
}Queue;

nodePointer graph[10];
int id = -1; // index of graph

Queue* init() {
	Queue * q = (Queue*)malloc(sizeof(Queue));
	q->que = (Queue*)malloc(sizeof(Queue));
	q->max = 6;
	q->num = 0;
	q->front = 0;
	q->rear = 0;
	return q;
}
int empty(const Queue *q) {
	if (q->num == 0) return 1; // ������� ��
	return 0;
}
void dequeue(Queue* q) { // ����
	if (empty(q)) return;
	q->num--;
	q->que[q->front++] = -1;
}
void enqueue(Queue* q, int value) { // ����
	if (q->num == q->max) return;
	q->num++;
	q->que[q->rear++] = value;
}
int front(Queue* q) { // ť �� ���� �ڷ� ����
	if (empty(q)) return;
	return q->que[q->front];
}
nodePointer createNode(int v, char *name) {
	nodePointer newNode = (nodePointer*)malloc(sizeof(node));
	strcpy(newNode->lecture.name, name);
	newNode->lecture.prerequisites = NULL;
	newNode->lecture.difficulty = 5; // ���̵� �⺻�� 5
	newNode->lecture.n_prerequisites = 0;
	newNode->vertex = v;
	return newNode;
}

// name�� �����ϴ� ��� ã��
int nameToInt(char *name) {
	id++;
	for (int i = 0; i < id; ++i) {
		if (graph[i] != NULL) {
			if (strcmp(graph[i]->lecture.name, name) == 0) {
				id--;
				return i; // �̸��� �����ϴ� ��� �ε��� ����
			}
		}
	}
	return id;
}
// ��� ��ȯ
void swap(int i, int j) {
	nodePointer temp;
	temp = graph[i];
	graph[i] = graph[j];
	graph[j] = temp;
}
// ���̵�, ���ĺ� ������ ���� (��, ���� �켱������ ���̵��� ����.)
void sort() {
	for (int i = 0; i < id; ++i) {
		for (int j = i+1; j < id; ++j)
		{	
			float a = graph[i]->lecture.difficulty;
			float b = graph[j]->lecture.difficulty;
			if (a < b) {
				swap(i, j);
			}
			else { 
				char *tmp = graph[i]->lecture.name;
				char *tmp2 = graph[j]->lecture.name;
				if (strcmp(tmp, tmp2) > 0) {
					swap(i, j);
				}
			}
		}
	}
}
int main(int argc, char** argv)
{
	int num = 0; // node ����
	char *token = NULL; // �ڸ� ���ڿ�
	char line[256]; // ���ڿ� �� �� �б�
	char temp[30];  

	FILE* fp = fopen("./in/database.csv", "r");
	if (fp == NULL) {
		printf("������ ���� �� �����ϴ�.\n");
		return 1;
	}
	while (fgets(line, sizeof(line), fp)) {
		int cnt = 0;
		token = strtok(line, ",");
		while (token != NULL) {
			if (token[0] == ' ') {
				int len = strlen(token);
				strcpy(temp, token);
				for (int i = 1; i < len; ++i) {
					temp[i - 1] = temp[i];
				}
				temp[len - 1] = '\0';
			}
			// ���� ���� ��
			if (token[0] >= '0' && token[0] <= '9') {
				graph[num]->lecture.n_prerequisites = token[0] - '0';
			}  // ���� ���� �̸�
			else if (token[0] == ' ') {
				graph[nameToInt(temp)]->lecture.prerequisites = &graph[num]->lecture;
			}
			else { // �����̸�
				graph[num] = createNode(nameToInt(token), token);
			}
 			if(cnt < 1) token = strtok(NULL, ", \n"); // ���� ���� ������ �Ľ��� ��
			else if (cnt >= 1) token = strtok(NULL, ",\n"); // ���� ����� �̸� �Ľ��� ��
			++cnt;
		}
		++num;
	}
	if (fp != NULL) fclose(fp);
	// ���̵� ���� �ֱ� 
	for (int i = 0; i < argc; ++i) {
		FILE* fin = fopen(argv[i], "r");
		if (fin == NULL) {
			printf("������ ���� �� �����ϴ�.\n");
			return 1;
		}
		while (fgets(line, sizeof(line), fp)) {
			int flag = 0;
			token = strtok(line, ",");
			while (token != NULL) {
				if (flag == 0) {
					strcpy(temp, token);
				}
				else {
					graph[nameToInt(temp)]->lecture.difficulty = atof(token);
				}
				token = strtok(NULL, " \n");
				flag++;
			}
		}
		if (fin != NULL) fclose(fin);
	}
	// ���̵� ��, ���ĺ� ������ ����
	sort();
	// topological sorting (���� ����)
	Queue* q = init();
	for (int i = 0; i < num; ++i) {
		if (!graph[i]->lecture.n_prerequisites) enqueue(q, i);
	}
	while (!empty(q)) {
		int node = front(q);
		dequeue(q);
		printf("%s\n", graph[node]->lecture.name);
		if (graph[node]->lecture.prerequisites != NULL) {
			if (graph[node]->lecture.prerequisites->n_prerequisites) {
				graph[node]->lecture.prerequisites->n_prerequisites--;
			}
			if (!graph[node]->lecture.prerequisites->n_prerequisites) {
				int n = nameToInt(graph[node]->lecture.prerequisites->name);
				enqueue(q, n);
			}
		}
	}
	return 0;
}
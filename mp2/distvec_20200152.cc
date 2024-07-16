#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NODES 100 // 최대 노드 수를 정의
#define NOT_EXIST -1 // 존재하지 않음을 나타내는 상수
#define INFINITY_COST 999 // 무한 비용을 나타내는 상수
#define MAX_MESSAGE_LENGTH 1000 // 최대 메시지 길이를 정의

typedef struct {
    int source; // 링크의 출발 노드
    int destination; // 링크의 도착 노드
    int cost; // 링크의 비용
} Link;

typedef struct {
    int previous; // 이전 노드 (미사용)
    int next_hop; // 다음 홉
    int cost; // 비용
} Route;

FILE *topology_file; // 토폴로지 파일 포인터
FILE *message_file; // 메시지 파일 포인터
FILE *change_file; // 변경 파일 포인터
FILE *output_file; // 출력 파일 포인터

Link link_table[MAX_NODES]; // 링크 정보를 저장할 테이블
int link_count = 0; // 링크 개수
int node_count; // 노드 개수

Route routing_table[MAX_NODES][MAX_NODES]; // 라우팅 테이블

int has_changes; // 변화가 있었는지 여부를 나타내는 플래그

// 함수 선언
int initialize(int argc, char **argv);
void print_routing_table();
void distance_vector();
void initialize_routing_table();
void read_topology();
void process_messages();
void update_link_cost(int source, int destination, int new_cost);
void apply_changes();

int initialize(int argc, char **argv) {
    if (argc != 4) { // 인자 개수가 올바른지 확인
        printf("usage: distvec topologyfile messagesfile changesfile\n");
        return -1;
    }

    topology_file = fopen(argv[1], "r"); // 토폴로지 파일 열기
    if (topology_file == NULL) {
        printf("Error: open input file %s.\n", argv[1]);
        return -1;
    }

    message_file = fopen(argv[2], "r"); // 메시지 파일 열기
    if (message_file == NULL) {
        printf("Error: open input file %s.\n", argv[2]);
        return -1;
    }

    change_file = fopen(argv[3], "r"); // 변경 파일 열기
    if (change_file == NULL) {
        // printf("Warning: could not open change file %s. Continuing without changes.\n", argv[3]);
        change_file = NULL;
    }

    output_file = fopen("output_dv.txt", "w"); // 출력 파일 열기
    if (output_file == NULL) {
        printf("Error: open output file.\n");
        return -1;
    }

    return 0; // 초기화 성공
}

void print_routing_table() {
    char buffer[1024]; // 출력 버퍼
    for (int i = 0; i < node_count; i++) { // 모든 노드에 대해
        int offset = 0; // 버퍼 오프셋
        for (int j = 0; j < node_count; j++) { // 모든 목적지에 대해
            if (routing_table[i][j].cost != INFINITY_COST) { // 유효한 경로인지 확인
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d %d %d\n", j, routing_table[i][j].next_hop, routing_table[i][j].cost); // 버퍼에 경로 저장
            }
        }
        snprintf(buffer + offset, sizeof(buffer) - offset, "\n"); // 버퍼에 빈 줄 삽입
        fputs(buffer, output_file); // 한 번에 출력
    }
}

void distance_vector() {
    has_changes = 0; // 변화 플래그 초기화
    for (int i = 0; i < node_count; i++) { // 모든 출발 노드에 대해
        for (int j = 0; j < node_count; j++) { // 모든 목적지 노드에 대해
            int current_cost = routing_table[i][j].cost;
            int best_cost = current_cost;
            int best_next_hop = routing_table[i][j].next_hop;
            for (int k = 0; k < node_count; k++) { // 모든 노드에 대해
                int new_cost = routing_table[i][k].cost + routing_table[k][j].cost;
                if (new_cost < best_cost) { // 더 짧은 경로를 찾으면
                    best_cost = new_cost;
                    best_next_hop = routing_table[i][k].next_hop;
                    has_changes = 1; // 변화 플래그 설정
                } else if (new_cost == best_cost && routing_table[i][k].next_hop < best_next_hop && i != k) { // 비용이 같은 경로가 있으면
                    best_next_hop = routing_table[i][k].next_hop; // 더 작은 홉 값을 선택
                    has_changes = 1; // 변화 플래그 설정
                }
            }
            if (best_cost != current_cost) {
                routing_table[i][j].cost = best_cost; // 비용 업데이트
                routing_table[i][j].next_hop = best_next_hop; // 다음 홉 업데이트
            }
        }
    }
}

void initialize_routing_table() {
    // 초기화 루프를 한 번으로 줄이기
    for (int i = 0; i < node_count; i++) { // 모든 노드에 대해
        for (int j = 0; j < node_count; j++) { // 모든 목적지에 대해
            routing_table[i][j].cost = (i == j) ? 0 : INFINITY_COST; // 초기 비용 설정
            routing_table[i][j].next_hop = (i == j) ? i : NOT_EXIST; // 초기 다음 홉 설정
        }
    }

    // 링크 테이블을 사용하여 초기화
    for (int i = 0; i < link_count; i++) { // 모든 링크에 대해
        int source = link_table[i].source;
        int destination = link_table[i].destination;
        int cost = link_table[i].cost;

        // 링크 비용 및 다음 홉 설정
        routing_table[source][destination].cost = cost;
        routing_table[source][destination].next_hop = destination;
        routing_table[destination][source].cost = cost;
        routing_table[destination][source].next_hop = source;
    }
}

void read_topology() {
    fscanf(topology_file, "%d", &node_count); // 노드 수 읽기
    while (fscanf(topology_file, "%d %d %d", &link_table[link_count].source, &link_table[link_count].destination, &link_table[link_count].cost) == 3) {
        link_count++; // 링크 수 증가 및 다음 링크로 이동
    }
}

void process_messages() {
    rewind(message_file); // 메시지 파일의 처음으로 이동
    int source, destination;
    char message[MAX_MESSAGE_LENGTH];
    char buffer[1024]; // 출력 버퍼
    while (fscanf(message_file, "%d %d %[^\n]", &source, &destination, message) == 3) {
        int offset = 0;
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "from %d to %d cost ", source, destination); // 메시지 정보 출력 시작
        int next = routing_table[source][destination].next_hop; // 다음 홉 가져오기
        if (next == -1) { // 경로가 없으면
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "infinite hops unreachable "); // 도달 불가 메시지 출력
        } else {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d hops ", routing_table[source][destination].cost); // 총 비용 출력
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d ", source); // 출발 노드 출력
            while (next != destination) { // 도착지에 도달할 때까지
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d ", next); // 다음 홉 출력
                next = routing_table[next][destination].next_hop; // 다음 노드로 이동
            }
        }
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "message %s\n", message); // 메시지 내용 출력
        fputs(buffer, output_file); // 버퍼 내용을 출력 파일에 기록
    }
    fputs("\n", output_file); // 메시지 사이에 빈 줄 삽입
}

void update_link_cost(int source, int destination, int new_cost) {
    if (new_cost == -999) new_cost = INFINITY_COST; // -999는 링크가 없음을 의미

    int found = 0;
    for (int i = 0; i < link_count; i++) { // 모든 링크에 대해
        if ((link_table[i].source == source && link_table[i].destination == destination) || 
            (link_table[i].source == destination && link_table[i].destination == source)) {
            link_table[i].cost = new_cost; // 링크 비용 업데이트
            found = 1; // 링크 찾음 플래그 설정
            break;
        }
    }
    if (!found && new_cost != INFINITY_COST) { // 링크를 찾지 못했으면
        link_table[link_count].source = source; // 새로운 링크 추가
        link_table[link_count].destination = destination; // 새로운 링크 추가
        link_table[link_count].cost = new_cost; // 새로운 링크 비용 설정
        link_count++; // 링크 수 증가
    }
}

void apply_changes() {
    if (change_file == NULL) return; // change_file이 NULL인 경우 바로 반환

    int source, destination, cost;
    while (fscanf(change_file, "%d %d %d", &source, &destination, &cost) == 3) {
        update_link_cost(source, destination, cost); // 링크 비용 업데이트
        initialize_routing_table(); // 라우팅 테이블 초기화

        int iterations = 0;
        do {
            distance_vector(); // 거리 벡터 알고리즘 수행
            iterations++;
        } while (has_changes != 0 && iterations < node_count); // 변화가 없거나 최대 반복 횟수 도달 시 종료

        print_routing_table(); // 라우팅 테이블 출력

        if (message_file) {
            process_messages(); // 메시지 처리
        }
    }
}

int main(int argc, char **argv) {
    if (initialize(argc, argv) == -1) { // 초기화 실패 시 종료
        return -1;
    }

    read_topology(); // 토폴로지 읽기
    initialize_routing_table(); // 라우팅 테이블 초기화

    int iterations = 0;
    do {
        distance_vector(); // 거리 벡터 알고리즘 수행
        iterations++;
    } while (has_changes != 0 && iterations < node_count); // 변화가 없거나 최대 반복 횟수 도달 시 종료

    print_routing_table(); // 라우팅 테이블 출력

    if (message_file) {
        process_messages(); // 메시지 처리
    }

    if (change_file) {
        apply_changes(); // 변경 사항 적용
    }

    fclose(topology_file);
    fclose(message_file);
    if (change_file) fclose(change_file); // change_file이 NULL이 아닌 경우에만 닫기
    fclose(output_file);

    printf("Complete. Output file written to output_dv.txt.\n");

    return 0; // 프로그램 종료
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NODES 100 // 최대 노드 수를 정의
#define NOT_EXIST -1 // 존재하지 않음을 나타내는 상수
#define INFINITY_COST 999 // 무한 비용을 나타내는 상수
#define MAX_MESSAGE_LENGTH 1000 // 최대 메시지 길이를 정의

FILE *topology_file; // 토폴로지 파일 포인터
FILE *message_file; // 메시지 파일 포인터
FILE *change_file; // 변경 파일 포인터
FILE *output_file; // 출력 파일 포인터

typedef struct {
    int source; // 링크의 출발 노드
    int destination; // 링크의 도착 노드
    int cost; // 링크의 비용
} Link;

typedef struct {
    int next_hop; // 다음 홉
    int cost; // 비용
    int past; // 이전 노드를 저장하는 변수
} Route;

Link link_table[MAX_NODES]; // 링크 정보를 저장할 테이블
int link_count = 0; // 링크 개수
int node_count; // 노드 개수

Route routing_table[MAX_NODES][MAX_NODES]; // 라우팅 테이블

#define UNVISITED 0 // 방문하지 않음을 나타내는 상수
#define VISITED 1 // 방문했음을 나타내는 상수
short visit_status[MAX_NODES][MAX_NODES]; // 방문 상태를 저장할 배열

int initialize(int argc, char **argv); // 초기화 함수 선언
void read_topology(); // 토폴로지 파일 읽기 함수 선언
void initialize_routing_table(); // 라우팅 테이블 초기화 함수 선언
int find_min_cost_unvisited_node(int source); // 최소 비용의 방문하지 않은 노드 찾기 함수 선언
void update_routes_by_chosen_node(int source, int chosen); // 선택된 노드에 의해 경로 업데이트 함수 선언
void run_dijkstra(int source); // 다익스트라 알고리즘 실행 함수 선언
void print_routing_table(); // 라우팅 테이블 출력 함수 선언
void process_messages(); // 메시지 처리 함수 선언
void update_link_cost(int source, int destination, int new_cost); // 링크 비용 업데이트 함수 선언
void apply_changes(); // 변경 사항 적용 함수 선언

int initialize(int argc, char **argv) {
    if (argc != 4) { // 인자 개수가 올바른지 확인
        printf("usage: linkstate topologyfile messagesfile changesfile\n");
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
    }

    output_file = fopen("output_ls.txt", "w"); // 출력 파일 열기
    if (output_file == NULL) {
        printf("Error: open output file.\n");
        return -1;
    }

    return 0; // 초기화 성공
}

void read_topology() {
    fscanf(topology_file, "%d", &node_count); // 노드 수 읽기
    while (fscanf(topology_file, "%d %d %d", &link_table[link_count].source, &link_table[link_count].destination, &link_table[link_count].cost) == 3) {
        link_count++; // 링크 수 증가 및 다음 링크로 이동
    }
}

void initialize_routing_table() {
    for (int i = 0; i < node_count; i++) {
        for (int j = 0; j < node_count; j++) {
            if (i == j) {
                routing_table[i][j].cost = 0; // 자기 자신으로의 비용은 0
                routing_table[i][j].next_hop = i; // 자기 자신을 다음 홉으로 설정
                routing_table[i][j].past = i; // 이전 노드를 자기 자신으로 설정
                visit_status[i][j] = VISITED; // 자기 자신으로의 방문 상태는 VISITED
            } else {
                routing_table[i][j].cost = INFINITY_COST; // 초기 비용 설정
                routing_table[i][j].next_hop = NOT_EXIST; // 초기 다음 홉 설정
                routing_table[i][j].past = NOT_EXIST; // 초기 이전 노드 설정
                visit_status[i][j] = UNVISITED; // 초기 방문 상태는 UNVISITED
            }
        }
    }

    // 링크 테이블을 사용하여 초기화
    for (int i = 0; i < link_count; i++) {
        int source = link_table[i].source;
        int destination = link_table[i].destination;
        int cost = link_table[i].cost;

        // 링크 비용 및 다음 홉 설정
        routing_table[source][destination].cost = cost;
        routing_table[source][destination].next_hop = destination;
        routing_table[source][destination].past = source;
        routing_table[destination][source].cost = cost;
        routing_table[destination][source].next_hop = source;
        routing_table[destination][source].past = destination;
    }
}

int find_min_cost_unvisited_node(int source) {
    int min_cost = INFINITY_COST; // 최소 비용 초기화
    int min_node = -1; // 최소 비용 노드 초기화
    for (int destination = 0; destination < node_count; destination++) {
        if (visit_status[source][destination] == UNVISITED) { // 방문하지 않은 노드에 대해
            int cost = routing_table[source][destination].cost; // 현재 노드에서 목적지까지의 비용
            if (cost < min_cost) { // 최소 비용 노드 찾기
                min_node = destination;
                min_cost = cost;
            }
        }
    }
    return min_node; // 최소 비용 노드 반환
}

void update_routes_by_chosen_node(int source, int chosen) {
    visit_status[source][chosen] = VISITED; // 선택된 노드를 방문으로 표시
    int chosen_cost = routing_table[source][chosen].cost;

    for (int destination = 0; destination < node_count; destination++) { // 모든 목적지에 대해
        if (visit_status[source][destination] == UNVISITED) { // 방문하지 않은 노드에 대해
            int new_cost = chosen_cost + routing_table[chosen][destination].cost; // 새로운 비용 계산
            Route *current_route = &routing_table[source][destination];

            if (new_cost < current_route->cost) { // 더 낮은 비용이 있으면
                current_route->cost = new_cost; // 비용 업데이트
                current_route->next_hop = routing_table[source][chosen].next_hop; // 다음 홉 업데이트
                current_route->past = chosen; // 이전 노드 업데이트
            } else if (new_cost == current_route->cost && chosen < current_route->past) { // 비용이 같고 더 작은 홉 값을 선택
                current_route->next_hop = routing_table[source][chosen].next_hop; // 다음 홉 업데이트
                current_route->past = chosen; // 이전 노드 업데이트
            }
        }
    }
}

void run_dijkstra(int source) {
    while (1) {
        int chosen = find_min_cost_unvisited_node(source);
        if (chosen == -1) break; // 더 이상 방문할 노드가 없으면 종료
        update_routes_by_chosen_node(source, chosen); // 선택된 노드에 의해 경로 업데이트
    }
}

void print_routing_table() {
    char buffer[4096]; // 출력 버퍼
    for (int i = 0; i < node_count; i++) {
        int offset = 0; // 버퍼 오프셋 초기화
        for (int j = 0; j < node_count; j++) {
            if (routing_table[i][j].cost != INFINITY_COST) {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d %d %d\n", j, routing_table[i][j].next_hop, routing_table[i][j].cost);
            }
        }
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");
        fputs(buffer, output_file); // 버퍼 내용을 출력 파일에 기록
    }
}

void process_messages() {
    rewind(message_file); // 메시지 파일의 처음으로 이동
    int source, destination;
    char message[MAX_MESSAGE_LENGTH];
    char buffer[4096]; // 출력 버퍼
    while (fscanf(message_file, "%d %d %[^\n]", &source, &destination, message) == 3) {
        int offset = 0; // 버퍼 오프셋 초기화
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "from %d to %d cost ", source, destination);
        int next = routing_table[source][destination].next_hop; // 다음 홉 가져오기
        if (next == -1) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "infinite hops unreachable ");
        } else {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d hops ", routing_table[source][destination].cost);
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d ", source);
            while (next != destination) {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d ", next);
                next = routing_table[next][destination].next_hop;
            }
        }
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "message %s\n", message);
        fputs(buffer, output_file); // 버퍼 내용을 출력 파일에 기록
    }
    fputs("\n", output_file); // 메시지 사이에 빈 줄 삽입
}

void update_link_cost(int source, int destination, int new_cost) {
    if (new_cost == -999) new_cost = INFINITY_COST; // -999는 링크가 없음을 의미

    for (int i = 0; i < link_count; i++) { // 모든 링크에 대해
        if ((link_table[i].source == source && link_table[i].destination == destination) || 
            (link_table[i].source == destination && link_table[i].destination == source)) {
            link_table[i].cost = new_cost; // 링크 비용 업데이트
            return; // 링크 찾음 플래그 설정 후 함수 종료
        }
    }

    // 링크를 찾지 못했으면 새로운 링크 추가
    if (new_cost != INFINITY_COST) {
        link_table[link_count].source = source;
        link_table[link_count].destination = destination;
        link_table[link_count].cost = new_cost;
        link_count++; // 링크 수 증가
    }
}

void apply_changes() {
    if (change_file == NULL) return; // change_file이 NULL인 경우 바로 반환

    int source, destination, cost;
    while (fscanf(change_file, "%d %d %d", &source, &destination, &cost) == 3) { // 변경 파일에서 데이터를 읽어옴
        update_link_cost(source, destination, cost); // 링크 비용 업데이트
        initialize_routing_table(); // 라우팅 테이블 초기화

        for (int i = 0; i < node_count; i++) { // 모든 노드에 대해
            run_dijkstra(i); // 다익스트라 알고리즘 실행
        }

        print_routing_table(); // 라우팅 테이블 출력
        process_messages(); // 메시지 처리
    }
}

int main(int argc, char **argv) {
    if (initialize(argc, argv) == -1) return -1; // 초기화 실패 시 종료
    
    read_topology(); // 토폴로지 읽기
    initialize_routing_table(); // 라우팅 테이블 초기화
    
    for (int i = 0; i < node_count; i++) { // 모든 노드에 대해
        run_dijkstra(i); // 다익스트라 알고리즘 실행
    }
    print_routing_table(); // 라우팅 테이블 출력

    message_file = fopen(argv[2], "r"); // 메시지 파일 열기
    if (message_file) {
        process_messages(); // 메시지 처리
    }

    if (change_file) {
        apply_changes(); // 변경 사항 적용
    }

    fclose(topology_file);
    fclose(message_file);
    if (change_file) fclose(change_file);
    fclose(output_file);

    printf("Complete. Output file written to output_ls.txt.\n");

    return 0; // 프로그램 종료
}

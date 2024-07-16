#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define MAX 2 * 20 * 20 * 512 * 512

// 프레임과 생성기 다항식을 사용하여 CRC 나머지 계산
int calculate_crc_remainder(char *frame, char *generator) {
    int frame_len = strlen(frame);  // 프레임의 길이 계산
    int generator_len = strlen(generator);  // 생성기 다항식의 길이 계산
    for (int i = 0; i <= frame_len - generator_len; i++) {  // 프레임 전체를 순회
        if (frame[i] == '1') {  // 프레임의 i번째 비트가 1인 경우
            for (int j = 0; j < generator_len; j++) {  // 생성기 다항식의 길이만큼 XOR 연산 수행
                frame[i + j] = (frame[i + j] == generator[j] ? '0' : '1');
            }
        }
    }
    // 나머지 확인
    for (int i = frame_len - generator_len + 1; i < frame_len; i++) {
        if (frame[i] == '1') return 1; // 나머지가 1이면 에러 발견
    }
    return 0; // 나머지가 0이면 에러 없음
}

// 10진수를 8비트 이진 문자열로 변환
char *convert_decimal_to_binary_string(int decimal)
{
    char *binary = (char *)malloc(sizeof(char) * 9); // 8비트 + NULL 문자('\0')를 위한 메모리 할당
    if (!binary) {
        perror("memory allocation error");  // 메모리 할당 실패시 에러 메시지 출력
        exit(EXIT_FAILURE);  // 프로그램 비정상 종료
    }
    int i;
    for (i = 0; i < 8; i++)
    {
        *(binary + i) = ((decimal >> (7 - i)) & 1) + '0';  // 비트 연산을 통한 이진수 변환
    }
    *(binary + i) = '\0'; // 문자열 끝에 NULL 문자 삽입
    return binary;  // 변환된 이진 문자열 반환
}

// 이진 문자열을 unsigned char 값으로 변환
unsigned char convert_binary_string_to_char(char *binary)
{
    unsigned char result = 0;
    for (int i = 7; i >= 0; i--) {
        result = (result << 1) + (binary[i] == '1');  // 이진 문자열을 unsigned char로 변환
    }
    return result;
}

// 디코딩된 데이터를 파일에 쓰기
void write_binary_data_to_file(char *decoded, FILE *fout)
{
    char copy[9];
	while (*decoded) {  // 디코딩된 데이터가 끝날 때까지 반복
		unsigned char num = 0;
		strncpy(copy, decoded, 8);  // 8비트 복사
		copy[8] = '\0';
        num = convert_binary_string_to_char(copy);  // 이진 문자열을 unsigned char로 변환
		decoded += 8;  // 다음 8비트로 이동
		fwrite(&num, sizeof(char), 1, fout);  // 파일에 한 바이트씩 쓰기
	}
}

FILE *open_file(const char *filename, const char *mode) {
    FILE *file = fopen(filename, mode);  // 파일 열기
    if (!file) {
        fprintf(stderr, "%s file open error.\n", mode[0] == 'r' ? "input" : "output");  // 파일 열기 실패시 에러 메시지 출력
        exit(EXIT_FAILURE);  // 프로그램 비정상 종료
    }
    return file;  // 열린 파일의 포인터 반환
}

int main(int argc, char *argv[])
{
    // 초기 설정
    if (argc != 6)
    {
        fprintf(stderr, "usage: ./crc_decoder input_file output_file result_file generator dataword_size\n");
        exit(1);
    }

    FILE *input_file = open_file(argv[1], "rb");
    FILE *output_file = open_file(argv[2], "wb");

    FILE *result_file = fopen(argv[3], "w"); // 결과 파일 열기
    if (result_file == NULL)
    {
        fprintf(stderr, "result file open error.\n");
        exit(1);
    }

    int dataword_size = atoi(argv[5]); // 데이터워드 크기 파싱
    if (dataword_size != 4 && dataword_size != 8) // 데이터워드 크기 유효성 검사
    {
        fprintf(stderr, "dataword size must be 4 or 8.\n");
        exit(1);
    }

    int count = 0; // 프레임 카운트
    int pad_n = 0; // 패딩 크기
    unsigned char buf; // 버퍼
    char *generator = (char*)calloc(strlen(argv[4]) + 1, 1); // 생성기 메모리 할당
    if (!generator) {
        fclose(input_file);
        fclose(output_file);
        fclose(result_file);
        perror("memory allocation error");
        exit(EXIT_FAILURE);
    }
    strcpy(generator, argv[4]); // 생성기 복사
    char *input_binary = (char*)malloc(MAX); // 입력 데이터를 이진 문자열로 저장할 메모리 할당
    if (!input_binary) {
        fclose(input_file);
        fclose(output_file);
        fclose(result_file);
        free(generator);
        perror("memory allocation error");
        exit(EXIT_FAILURE);
    }
    input_binary[0] = '\0';
    int input_n = 0; // 입력 데이터 길이
    while (fread(&buf, 1, 1, input_file) != 0)
    {
        if (count == 0)
            pad_n = buf; // 첫 바이트는 패딩 크기
        else 
        {
            char *buf_binary = convert_decimal_to_binary_string((int)buf); // 버퍼를 이진 문자열로 변환
            strcat(input_binary, buf_binary); // 이진 문자열 추가
        }
        count++;
    }
    input_binary += pad_n; // 패딩 제거
    int frame_size = dataword_size + strlen(generator) - 1; // 프레임 크기 계산
    char *frame = (char*)calloc(frame_size + 1, 1); // 프레임 메모리 할당
    if (!frame) {
        fclose(input_file);
        fclose(output_file);
        fclose(result_file);
        free(generator);
        free(input_binary);
        perror("memory allocation error");
        exit(EXIT_FAILURE);
    }
    count = 0;
    int error = 0; // 에러 카운트
    char *decoded = (char*)calloc(MAX, 1); // 디코딩된 데이터 저장
    if (!decoded) {
        fclose(input_file);
        fclose(output_file);
        fclose(result_file);
        free(generator);
        free(input_binary);
        free(frame);
        perror("memory allocation error");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; input_binary[i]; i += frame_size) // 전체 입력 데이터를 프레임 크기만큼 반복
    {
        strncpy(frame, input_binary + i, frame_size);
        frame[frame_size] = '\0';
        if(calculate_crc_remainder(frame, generator)) // CRC 검사
            error++;
        count++;
        strncat(decoded, frame, dataword_size); // 디코딩된 데이터 추가
    }
    write_binary_data_to_file(decoded, output_file); // 파일에 디코딩된 데이터 쓰기
    fprintf(result_file,"%d %d\n", count, error); // 결과 파일에 총 프레임 수와 에러 수 기록
    return (0);
}

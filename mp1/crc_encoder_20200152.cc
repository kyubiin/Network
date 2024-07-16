#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define MAX 2 * 20 * 20 * 512 * 512

// 정수를 이진 문자열로 변환
char *convert_decimal_to_binary_string(int n)
{
    char *binary_str = (char *)malloc(9 * sizeof(char)); // 9바이트 메모리 할당 (8비트 + 종료문자)
    if (!binary_str) {
        perror("memory allocation error"); // 메모리 할당 실패 오류 출력
        exit(EXIT_FAILURE);
    }
    binary_str[8] = '\0'; // 문자열 종료 문자 설정

    for (int i = 7; i >= 0; i--) // 8비트까지
    {
        binary_str[i] = (n & 1) + '0'; // 최하위 비트를 문자로 변환
        n = n >> 1; // 오른쪽으로 한 비트 시프트
    }

    return binary_str;
}

// 나머지를 계산하는 함수
char *calculate_crc_remainder(char *dataword, char *generator) {
    int d_len = strlen(dataword);
    int g_len = strlen(generator);
    int r_len = g_len - 1;
    char *dividend = (char *)malloc(d_len + r_len + 1);
    if (!dividend) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    strcpy(dividend, dataword);
    memset(dividend + d_len, '0', r_len);
    dividend[d_len + r_len] = '\0';

    for (int i = 0; i < d_len; i++) {
        if (dividend[i] == '1') {
            for (int j = 0; j < g_len; j++) {
                dividend[i + j] ^= generator[j] ^ '0';
            }
        }
    }

    char *remainder = strdup(dividend + d_len);
    free(dividend);
    if (!remainder) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    return remainder;
}

// 코드워드 생성 함수
char *generate_codeword(char *dataword, char *generator) {
    char *remainder = calculate_crc_remainder(dataword, generator);
    char *codeword = (char *)malloc(strlen(dataword) + strlen(remainder) + 1);
    if (!codeword) {
        free(remainder);
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    strcpy(codeword, dataword);
    strcat(codeword, remainder);
    free(remainder);
    return codeword;
}

int convert_binary_to_decimal(char *binary) {
    int num = 0, bin = 1;
    for (int i = 7; i >= 0; i--) {
        num += (binary[i] == '1') ? bin : 0;
        bin <<= 1;
    }
    return num;
}

// 이진 파일에 쓰기
void write_binary_data_to_file(char *code, FILE *fout) {
    while (*code) {
        char byte_segment[9];
        strncpy(byte_segment, code, 8);
        byte_segment[8] = '\0';
        code += 8;
        int num = convert_binary_to_decimal(byte_segment);
        fputc(num, fout);
    }
}

FILE *open_file(const char *filename, const char *mode) {
    FILE *file = fopen(filename, mode);
    if (!file) {
        fprintf(stderr, "%s file open error.\n", mode[0] == 'r' ? "input" : "output");
        exit(EXIT_FAILURE);
    }
    return file;
}

int main(int argc, char *argv[])
{
    // 초기 설정
    if (argc != 5) // 인자 개수가 5개가 아니면
    {
        fprintf(stderr, "usage: ./crc_encoder input_file output_file generator dataword_size\n"); // 사용법 출력
        exit(1);
    }

    FILE *input_file = open_file(argv[1], "rb");
    FILE *output_file = open_file(argv[2], "wb");

    int dataword_size = atoi(argv[4]); // 데이터워드 크기 읽기
    if (dataword_size != 4 && dataword_size != 8) // 4 또는 8이 아니면
    {
        fprintf(stderr, "dataword size must be 4 or 8.\n"); // 오류 메시지 출력
        exit(1);
    }

    char *generator = (char *)malloc(strlen(argv[3]) + 1); // 생성기 메모리 할당
    if (!generator) {
        fclose(input_file);
        fclose(output_file);
        perror("memory allocation error"); // 메모리 할당 실패 오류 출력
        exit(EXIT_FAILURE);
    }
    strcpy(generator, argv[3]); // 생성기 복사

    char buf;
    char *final_code = (char *)calloc(MAX, 1); // 최종 코드 메모리 할당
    if (!final_code) {
        fclose(input_file);
        fclose(output_file);
        free(generator);
        perror("memory allocation error"); // 메모리 할당 실패 오류 출력
        exit(EXIT_FAILURE);
    }
    char *code_word;

    // 메인 루프
    while (fread(&buf, 1, 1, input_file) != 0) // 파일에서 1바이트씩 읽기
    {
        char *buf_binary = convert_decimal_to_binary_string((int)buf); // 버퍼의 이진 문자열 변환
        if (dataword_size == 8)
            code_word = generate_codeword(buf_binary, generator); // 8비트 데이터워드로 코드워드 계산
        else
        {
            char *temp_code_word[2];
            for (int i = 0; i < 2; i++)
            {
                char dataword[5];
                strncpy(dataword, buf_binary + i * 4, 4); // 4비트 추출
                dataword[4] = '\0'; // 문자열 종료 문자 설정
                temp_code_word[i] = generate_codeword(dataword, generator); // 코드워드 계산
            }
            // 두 개의 코드워드를 하나로 합치기
            code_word = (char *)malloc(strlen(temp_code_word[1]) * 2 + 1);
            if (!code_word) 
            {
                fclose(input_file);
                fclose(output_file);
                free(final_code);
                free(generator);
                perror("memory allocation error"); // 메모리 할당 실패 오류 출력
                exit(EXIT_FAILURE);
            }

            code_word[0] = '\0';
            strcat(code_word, temp_code_word[0]); // 첫 번째 코드워드 추가
            strcat(code_word, temp_code_word[1]); // 두 번째 코드워드 추가
            free(temp_code_word[0]); // 사용한 메모리 해제
            free(temp_code_word[1]); // 사용한 메모리 해제
        }
        strcat(final_code, code_word); // 최종 코드에 추가
        free(code_word); // 사용한 메모리 해제
        free(buf_binary); // 사용한 메모리 해제
    }
    int pad_size = 16 - (strlen(final_code) % 16); // 패딩 크기 계산
    if (pad_size == 16)
        pad_size = 0;
    char *padding = (char *)malloc(pad_size + 1);
    memset(padding, '0', pad_size); // 패딩으로 0 채우기
    padding[pad_size] = '\0'; // 문자열 종료 문자 설정
    char *pad_n = convert_decimal_to_binary_string(pad_size); // 패딩 크기 이진 문자열 변환
    char *print = (char *)malloc(strlen(pad_n) + strlen(padding) + strlen(final_code) + 1); // 최종 문자열 메모리 할당
    if (!print) {
        perror("memory allocation error"); // 메모리 할당 실패 오류 출력
        exit(EXIT_FAILURE);
    }
    print[0] = '\0';
    strcat(print, pad_n); // 패딩 크기 추가
    strcat(print, padding); // 패딩 추가
    strcat(print, final_code); // 최종 코드 추가
    write_binary_data_to_file(print, output_file); // 이진 파일에 쓰기
    free(pad_n); // 사용한 메모리 해제
    free(padding); // 사용한 메모리 해제
    free(final_code); // 사용한 메모리 해제
    fclose(input_file); // 파일 닫기
    fclose(output_file); // 파일 닫기
}

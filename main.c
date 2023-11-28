//
// Created by stefkoff on 28.11.23.
//


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BARCODE_TYPE_EAN 0x01
#define BARCODE_TYPE_UPC 0x02

#define EAN_MAX 13
#define UPC_MAX 12

#define QUIET_ZONE_SIZE 4
#define START_ZONE_SIZE 3
#define END_ZONE_SIZE 3
#define MIDDLE_ZONE_SIZE 5

#define BARCODE_PRINT_HEIGHT 20

void PrintHelp();

static const int CODES[2][10] = {
        {
                0b0001101, // 0
                0b0011001, // 1
                0b0010011, // 2
                0b0111101, // 3
                0b0100011, // 4
                0b0110001, // 5
                0b0101111, // 6
                0b0111011, // 7
                0b0110111, // 8
                0b0001011  // 9
        },
        {
                0b0100111, // 0
                0b0110011, // 1
                0b0011011, // 2
                0b0100001, // 3
                0b0011101, // 4
                0b0111001, // 5
                0b0000101, // 6
                0b0010001, // 7
                0b0001001, // 8
                0b0010111  // 9
        },

};

static const int CODE_TYPES[10][6] = {
        {0, 0, 0, 0, 0, 0},
        {0, 0, 1, 0, 1, 1},
        {0, 0, 1, 1, 0, 1},
        {0, 0, 1, 1, 1, 0},
        {0, 1, 0, 0, 1, 1},
        {0, 1, 1, 0, 0, 1},
        {0, 1, 1, 1, 0, 0},
        {0, 1, 0, 1, 0, 1},
        {0, 1, 0, 1, 1, 0},
        {0, 1, 1, 0, 1, 0}
};

int GetSizeByBarcodeType(int barcodeType) {
    if (barcodeType == BARCODE_TYPE_UPC) {
        return UPC_MAX;
    } else if (barcodeType == BARCODE_TYPE_EAN) {
        return EAN_MAX;
    }

    return 0;
}

char GetCheckDigit(const char *string) {
    int sum = 0;

    for (int i = 1; i <= 11; i += 2) {
        sum += 3 * (string[i] - '0');
    }

    for (int i = 0; i <= 10; i += 2) {
        sum += string[i] - '0';
    }

    int result = sum % 10;

    if (result > 0) {
        result = 10 - result;
    }

    return result + '0';
}

int TestCheckDigit(const char *string) {
    int sum = 0;
    unsigned long sLen = strlen(string);
    for (int i = 1; i <= sLen - 2; i += 2) {
        sum += 3 * (string[i] - '0');
    }

    for (int i = 0; i <= sLen - 3; i += 2) {
        sum += string[i] - '0';
    }
    int controlCheck = string[strlen(string) - 1] - '0';

    if (((sum + controlCheck) % 10) == 0) {
        return 1;
    }

    return 0;
}

const char *FixPadding(const char *string, int barcodeType) {
    int len = 0;

    if (barcodeType == BARCODE_TYPE_EAN) {
        len = 13;
    } else if (barcodeType == BARCODE_TYPE_UPC) {
        len = 12;
    }

    if (barcodeType == 0) {
        return "";
    }

    if (strlen(string) >= len - 1) {
        return string;
    }

    int neededToAdd = (len - 1) - strlen(string);

    char *result = malloc(sizeof(char) * len - 1);
    for (int i = 0; i < neededToAdd; i++) {
        result[i] = '0';
    }

    strcat(result, string);

    return result;
}

int GetBarcodeType(int argc, const char **argv) {
    int result = 0;
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-e") == 0) {
            result = BARCODE_TYPE_EAN;
        } else if (strcmp(argv[i], "-u") == 0) {
            result = BARCODE_TYPE_UPC;
        }
    }
    return result;
}

void ValidateBarcodeLen(int barcodeType, const char *barcode) {
    if (barcodeType == BARCODE_TYPE_EAN && strlen(barcode) > EAN_MAX) {
        printf("Invalid EAN-13 barcode provided\n");
        PrintHelp();
        exit(2);
    } else if (barcodeType == BARCODE_TYPE_UPC && strlen(barcode) > UPC_MAX) {
        printf("Invalid UPC barcode prided\n");
        PrintHelp();
        exit(3);
    }
}

const char *ValidateCheckDigit(int barcodeType, const char *barcode) {
    unsigned long bLen = strlen(barcode);
    int testResult = TestCheckDigit(barcode);
    if (bLen == UPC_MAX && barcodeType == BARCODE_TYPE_UPC) {
        if (testResult == 0) {
            printf("Your UPC barcode does not match the correct check digit\n");
            PrintHelp();
            exit(4);
        }

        return barcode;
    } else if (bLen == EAN_MAX && barcodeType == BARCODE_TYPE_EAN) {
        if (testResult == 0) {
            printf("Your EAN-13 barcode does not match the correct check digit\n");
            PrintHelp();
            exit(5);
        }

        return barcode;
    }

    char *result = malloc(bLen + 1);

    if (result == NULL) {
        printf("Could not allocate resources.");
        exit(6);
    }

    char controlC = GetCheckDigit(barcode);
    strcpy(result, barcode);
    result[bLen] = controlC;

    if (TestCheckDigit(result) == 0) {
        printf("Could not calculate the check digit");
        PrintHelp();
        exit(7);
    }

    return result;
}

const char *AddExtraZones(int barcodeType, const char *barcode) {
    char *result = malloc((6 *GetSizeByBarcodeType(barcodeType)) + (2 * QUIET_ZONE_SIZE) + START_ZONE_SIZE + END_ZONE_SIZE +
                          MIDDLE_ZONE_SIZE);

    if (result == NULL) {
        printf("Could not allocate resources");
        exit(6);
    }

    result[0] = '0';
    result[1] = '0';
    result[2] = '0';
    result[3] = '0';
    result[4] = '1';
    result[5] = '0';
    result[6] = '1';

    int codeType = barcode[0] - '0';
    const int *pattern = CODE_TYPES[codeType];
    int index = 7;
    for (int i = 1; i <= 6; i++) {
        for (int j = 0; j < 7; j++) {
            if ((CODES[pattern[i - 1]][barcode[i] - '0'] & (1 << (6 - j)))) {
                result[index] = '1';
            } else {
                result[index] = '0';
            }
            index++;
        }
    }

    for (int i = 1; i <= MIDDLE_ZONE_SIZE; i++) {
        result[index++] = i % 2 == 0 ? '1' : '0';
    }

    for (int i = 7; i <= 12; i++) {
        for (int j = 0; j < 7; j++) {
            if ((~(CODES[0][barcode[i] - '0']) & (1 << (6 - j)))) {
                result[index] = '1';
            } else {
                result[index] = '0';
            }
            index++;
        }
    }

    for(int i = 1; i <= END_ZONE_SIZE; i++) {
        result[index++] = i % 2 == 0 ? '0' : '1';
    }

    for(int i = 0; i < QUIET_ZONE_SIZE; i++) {
        result[index++] = '0';
    }

    return result;

}

void PrintHelp() {
    printf("Generate barcode in the console by a given number.\n");
    printf("The command will validate the check digit. If provided, it will be validate. If not, it will be calculated\n");
    printf("\nUsage: c-barcode-console-print [type] code\n");
    printf("Types:\n");
    printf("\t-e: EAN-13 - Default\n");
    printf("\t-u: UPC-A");
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        PrintHelp();
        return 1;
    }

    int barcodeType = GetBarcodeType(argc, argv);

    if (barcodeType == 0) {
        barcodeType = BARCODE_TYPE_EAN;
    }

    const char *barcode = argv[argc - 1];
    ValidateBarcodeLen(barcodeType, barcode);
    const char *barcodePadded = FixPadding(barcode, barcodeType);
    const char *barcodeWithCheckDigit = ValidateCheckDigit(barcodeType, barcodePadded);
    const char* barcodeData = AddExtraZones(barcodeType,barcodeWithCheckDigit );

    int targetSize = (2 * QUIET_ZONE_SIZE) + (6 * GetSizeByBarcodeType(barcodeType)) * MIDDLE_ZONE_SIZE + END_ZONE_SIZE;

    printf("\n");
    for(int x = 0; x <= BARCODE_PRINT_HEIGHT; x++) {
        for(int y = 0; y < targetSize; y++) {
            if((barcodeData[y] - '0') == 1){
                printf("#");
            } else {
                printf(" ");
            }
        }
        printf("\n");

        if(x == BARCODE_PRINT_HEIGHT) {
            int index = 0;
            int toShow = 2;
            for(int i = 0; i < targetSize; i++) {
                if(toShow == i && index < strlen(barcodeWithCheckDigit)) {
                    int toAdd = 7;
                    if(index == 6) {
                        toAdd = 11;
                    } else if(index > 6) {
                        toAdd = 7;
                    }
                    toShow += toAdd;
                    printf("%c", barcodeWithCheckDigit[index++]);
                } else {
                    if(
                            i == 4 ||
                            i == 6 ||
                            i == 48 ||
                            i == 50 ||
                            i == 52 ||
                            i == 96 ||
                            i == 98
                            ) {
                        printf("#");
                    } else {
                        printf(" ");
                    }
                }
            }
        }
    }

    barcodePadded = NULL;
    barcodeWithCheckDigit = NULL;
    barcodeData = NULL;

    return 0;
}
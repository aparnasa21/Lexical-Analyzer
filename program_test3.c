#include <stdio.h>

#define MAX 10

int main() {
    int nums[MAX];
    int i, sum = 0;
    float avg = 0.5;
    char grade = 'A';

    for (i = 0; i < MAX; i++) {
        nums[i] = i * 2;
        sum += nums[i];
    }

    if (sum > 50 && avg != 1.0) {
        printf("Sum is %d\n", sum);
    }

    return 0;
}
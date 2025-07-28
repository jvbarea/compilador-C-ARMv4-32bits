int x;
int y = 123;

int add (int a, int b) {
    return a + b;
}

int main() {
    x = 2;
    for (int i = 0; i < 5; i++) {
        x = x * 2;
    }
    return add(x, y);
}

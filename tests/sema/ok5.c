int add(int a, int b) {
    return a + b;                 // OK
}

void main() {
    return add(4, 5);            // deve passar no Sema
}
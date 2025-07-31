int __attribute__((naked)) add(int a, int b)
{
    return a + b;
}

int __attribute__((naked)) main()
{
    int a = 1, b = 4;
    int c = add(a, b);
    return c;
}

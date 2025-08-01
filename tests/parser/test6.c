int main(){
    int x = 4;
    int *p = &x;
    return *p + 1;   // deve passar no Sema
}
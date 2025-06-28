void print(ArrayPathNode *arr)
{
    for (size_t idx = 0; idx < arr->len; ++idx) {
        printf("%.0f, ", arr->ptr[idx].value);
    }

    printf("\n");
}

int main(void)
{
    ArrayPathNode heap = {0};

    push_heap(&heap, (PathNode) { 3.0 });
    push_heap(&heap, (PathNode) { 2.0 });
    push_heap(&heap, (PathNode) { 4.0 });
    push_heap(&heap, (PathNode) { 1.0 });
    push_heap(&heap, (PathNode) { 5.0 });
    push_heap(&heap, (PathNode) { 9.0 });
    print(&heap);

    pop_heap(&heap);
    print(&heap);

    return 0;
}

__kernel void bitonic_sort_step(__global int* data, int j, int k) {
    unsigned int i = get_global_id(0);
    unsigned int ixj = i ^ j;

    if (ixj > i) {
        if ((i & k) == 0) {
            if (data[i] > data[ixj]) {
                int tmp = data[i];
                data[i] = data[ixj];
                data[ixj] = tmp;
            }
        } else {
            if (data[i] < data[ixj]) {
                int tmp = data[i];
                data[i] = data[ixj];
                data[ixj] = tmp;
            }
        }
    }
}
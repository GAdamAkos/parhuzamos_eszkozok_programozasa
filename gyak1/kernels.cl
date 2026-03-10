// 4. feladat: Vektorok összeadása
__kernel void vector_add(__global const float* A, __global const float* B, __global float* C) {
    int i = get_global_id(0);
    C[i] = A[i] + B[i];
}

// 5. feladat: Hiányzó elemek (negatív értékek) pótlása átlaggal
__kernel void fill_missing(__global float* data) {
    int i = get_global_id(0);
    // Csak a belső elemeket nézzük, mert a szomszédok kellenek
    if (i > 0 && data[i] < 0.0f) { 
        data[i] = (data[i-1] + data[i+1]) / 2.0f;
    }
}

// 6. feladat: Rang számítása (hány kisebb elem van)
__kernel void calculate_rank(__global const int* data, __global int* ranks, int n) {
    int i = get_global_id(0);
    int count = 0;
    for (int j = 0; j < n; j++) {
        if (data[j] < data[i]) count++;
    }
    ranks[i] = count;
}

// 7. feladat: Előfordulások száma és egyediség vizsgálat
__kernel void count_freq(__global const int* data, __global int* counts, __global int* is_unique, int n) {
    int i = get_global_id(0);
    int freq = 0;
    for (int j = 0; j < n; j++) {
        if (data[j] == data[i]) freq++;
    }
    counts[i] = freq;
    if (freq > 1) *is_unique = 0; // Ha bárhol van duplikáció, nem egyedi
}
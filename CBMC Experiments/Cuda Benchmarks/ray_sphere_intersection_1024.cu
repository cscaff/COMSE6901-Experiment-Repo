#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void raySphereKernel(float* ray_origins, float* ray_dirs, int* hits, int N, float* sphere_c, float sphere_r) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    float ox = ray_origins[3*i+0] - sphere_c[0];
    float oy = ray_origins[3*i+1] - sphere_c[1];
    float oz = ray_origins[3*i+2] - sphere_c[2];

    float dx = ray_dirs[3*i+0];
    float dy = ray_dirs[3*i+1];
    float dz = ray_dirs[3*i+2];

    float a = dx*dx + dy*dy + dz*dz;
    float b = 2.0f * (ox*dx + oy*dy + oz*dz);
    float c = ox*ox + oy*oy + oz*oz - sphere_r*sphere_r;

    float disc = b*b - 4*a*c;
    hits[i] = (disc >= 0.0f) ? 1 : 0;
}

int main() {
    static const int N = 1024*1024;
    auto cpu_start = std::chrono::high_resolution_clock::now();

    float* h_ray_origins = new float[3*N];
    float* h_ray_dirs    = new float[3*N];
    int*   h_hits        = new int[N];

    for (int i = 0; i < N; ++i) {
        h_ray_origins[3*i+0] = i % 1000 * 0.001f;
        h_ray_origins[3*i+1] = i % 1000 * 0.001f;
        h_ray_origins[3*i+2] = 0.0f;
        h_ray_dirs[3*i+0] = 0.0f;
        h_ray_dirs[3*i+1] = 0.0f;
        h_ray_dirs[3*i+2] = 1.0f;
        h_hits[i] = 0;
    }

    float *d_ray_origins, *d_ray_dirs, *d_sphere_c;
    int *d_hits;
    float sphere_c[3] = {0.5f,0.5f,5.0f};
    float sphere_r = 1.0f;

    cudaMalloc(&d_ray_origins, 3*N*sizeof(float));
    cudaMalloc(&d_ray_dirs,    3*N*sizeof(float));
    cudaMalloc(&d_hits,        N*sizeof(int));
    cudaMalloc(&d_sphere_c,    3*sizeof(float));

    cudaMemcpy(d_ray_origins, h_ray_origins, 3*N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_ray_dirs,    h_ray_dirs,    3*N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_hits,        h_hits,        N*sizeof(int),     cudaMemcpyHostToDevice);
    cudaMemcpy(d_sphere_c,    sphere_c,      3*sizeof(float),   cudaMemcpyHostToDevice);

    int blockSize = 256;
    int gridSize = (N + blockSize - 1)/blockSize;

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    raySphereKernel<<<gridSize, blockSize>>>(d_ray_origins, d_ray_dirs, d_hits, N, d_sphere_c, sphere_r);
    cudaDeviceSynchronize();

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float gpu_time_ms;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    cudaMemcpy(h_hits, d_hits, N*sizeof(int), cudaMemcpyDeviceToHost);

    int hit_count = 0;
    for (int i = 0; i < N; ++i) hit_count += h_hits[i];

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double,std::milli>(cpu_end - cpu_start).count();

    size_t dynamic_mem = 3* N*sizeof(float) + N*sizeof(int) + 3*sizeof(float); // rays + hits + sphere
    size_t static_mem  = sizeof(N) + sizeof(sphere_r);

    std::cout << hit_count << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    delete[] h_ray_origins;
    delete[] h_ray_dirs;
    delete[] h_hits;
    cudaFree(d_ray_origins);
    cudaFree(d_ray_dirs);
    cudaFree(d_hits);
    cudaFree(d_sphere_c);

    return 0;
}
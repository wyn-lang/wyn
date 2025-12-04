// Wyn GPU Runtime - Metal Backend for macOS/iOS
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

static id<MTLDevice> device = nil;
static id<MTLCommandQueue> commandQueue = nil;

// Initialize Metal
void __wyn_gpu_init() {
    if (!device) {
        device = MTLCreateSystemDefaultDevice();
        if (device) {
            commandQueue = [device newCommandQueue];
        }
    }
}

// Get device count
int __wyn_gpu_device_count() {
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    return (int)[devices count];
}

// Get device name
const char* __wyn_gpu_device_name(int device_id) {
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    if (device_id < [devices count]) {
        return [[devices[device_id] name] UTF8String];
    }
    return "Unknown";
}

// Allocate GPU memory
void* __wyn_gpu_malloc(size_t size) {
    __wyn_gpu_init();
    id<MTLBuffer> buffer = [device newBufferWithLength:size options:MTLResourceStorageModeShared];
    return (__bridge_retained void*)buffer;
}

// Free GPU memory
void __wyn_gpu_free(void* ptr) {
    if (ptr) {
        CFRelease(ptr);
    }
}

// Copy to GPU
void __wyn_gpu_memcpy_to_device(void* dst, const void* src, size_t size) {
    id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)dst;
    memcpy([buffer contents], src, size);
}

// Copy from GPU
void __wyn_gpu_memcpy_from_device(void* dst, const void* src, size_t size) {
    id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)src;
    memcpy(dst, [buffer contents], size);
}

// Execute kernel (simplified)
void __wyn_gpu_launch_kernel(const char* kernel_source, void** buffers, int buffer_count, int grid_size) {
    __wyn_gpu_init();
    
    // Compile kernel
    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:kernel_source]
                                                  options:nil
                                                    error:&error];
    if (!library) return;
    
    id<MTLFunction> function = [library newFunctionWithName:@"compute"];
    if (!function) return;
    
    id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];
    if (!pipeline) return;
    
    // Execute
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    [encoder setComputePipelineState:pipeline];
    for (int i = 0; i < buffer_count; i++) {
        id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)buffers[i];
        [encoder setBuffer:buffer offset:0 atIndex:i];
    }
    
    MTLSize gridSize = MTLSizeMake(grid_size, 1, 1);
    MTLSize threadGroupSize = MTLSizeMake(256, 1, 1);
    [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadGroupSize];
    
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}

// Synchronize
void __wyn_gpu_sync() {
    // Metal operations are synchronous by default with waitUntilCompleted
}
